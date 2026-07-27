#include "UIRenderInterface.h"

#include <algorithm>
#include <cstring>

#include <stb/stb_image.h>

#include <graphics/CommandBuffer.h>
#include <graphics/Renderer.h>
#include <graphics/VMAWrapper.h>
#include <graphics/VulkanPipeline.h>
#include <graphics/VulkanShader.h>
#include <utility/Logger.h>

using namespace eage::ui;
using namespace eage::graphics;
using namespace utility;

namespace
{
	constexpr const char* UI_VERT_PATH = "./src/core/ui/shaders/compiled/ui.vert.spv";
	constexpr const char* UI_FRAG_PATH = "./src/core/ui/shaders/compiled/ui.frag.spv";
}

UIRenderInterface::UIRenderInterface( Renderer& renderer )
	: mRenderer( renderer )
{
}

UIRenderInterface::~UIRenderInterface()
{
	mRenderer.WaitForIdle();
	mPendingReleases.clear();
	mGeometries.clear();
	mTextures.clear();
	mNamedTextures.clear();
	mPipeline.reset();
	mPipelineLayout.reset();
	mVertModule.reset();
	mFragModule.reset();
}

void
UIRenderInterface::BeginFrame( CommandBuffer& cmd, uint32_t target_width, uint32_t target_height )
{
	EnsurePipeline();
	mActiveCmd = vk::CommandBuffer( static_cast<VkCommandBuffer>( cmd.GetNativeHandle() ) );
	mHasActiveCmd = true;
	mTargetWidth = target_width > 0 ? target_width : 1;
	mTargetHeight = target_height > 0 ? target_height : 1;
	mScissorEnabled = false;
	mScissor = vk::Rect2D{ { 0, 0 }, { mTargetWidth, mTargetHeight } };

	vk::Viewport viewport{};
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.width = static_cast<float>( mTargetWidth );
	viewport.height = static_cast<float>( mTargetHeight );
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;
	mActiveCmd.setViewport( 0, 1, &viewport );
	mActiveCmd.setScissor( 0, 1, &mScissor );
}

void
UIRenderInterface::EndFrame()
{
	mHasActiveCmd = false;
	mActiveCmd = vk::CommandBuffer{};
}

void
UIRenderInterface::AdvanceFrame()
{
	++mFrameNumber;
	DrainDeferredReleases();
}

void
UIRenderInterface::BindExternalImage( const std::string& name, ManagedImage& image )
{
	const uint32_t bindless_index = mRenderer.RegisterBindlessTexture(
		image.image_view.get(),
		mRenderer.GetDefaultSampler() );

	const Rml::TextureHandle handle = AllocateTextureHandle();
	TextureRecord record;
	record.bindless_index = bindless_index;
	record.external = true;
	record.width = image.extent.width;
	record.height = image.extent.height;
	mTextures.emplace( handle, std::move( record ) );
	mNamedTextures[ name ] = handle;
}

bool
UIRenderInterface::EnsurePipeline()
{
	if( mPipelineReady )
	{
		return true;
	}

	auto vert = load_shader_from_file( mRenderer.GetDevice(), UI_VERT_PATH );
	auto frag = load_shader_from_file( mRenderer.GetDevice(), UI_FRAG_PATH );
	if( !vert || !frag )
	{
		LOG_ERROR( "UIRenderInterface: failed to load UI shaders" );
		return false;
	}

	mVertModule = std::move( vert->module );
	mFragModule = std::move( frag->module );

	vk::PushConstantRange push_range{};
	push_range.stageFlags = vk::ShaderStageFlagBits::eVertex;
	push_range.offset = 0;
	push_range.size = sizeof( UIPushConstants );

	vk::DescriptorSetLayout set_layouts[] = {
		mRenderer.GetBuiltInDescriptorSetLayouts().bindless.get()
	};

	vk::PipelineLayoutCreateInfo layout_info{};
	layout_info.setLayoutCount = 1;
	layout_info.pSetLayouts = set_layouts;
	layout_info.pushConstantRangeCount = 1;
	layout_info.pPushConstantRanges = &push_range;
	mPipelineLayout = mRenderer.GetDevice().createPipelineLayoutUnique( layout_info );

	VulkanPipelineBuilder builder;
	mPipeline = builder
		.SetShaders( mVertModule.get(), mFragModule.get() )
		.SetPipelineLayout( mPipelineLayout.get() )
		.SetInputTopology( vk::PrimitiveTopology::eTriangleList )
		.SetPolygonMode( vk::PolygonMode::eFill )
		.SetCullMode( vk::CullModeFlagBits::eNone, vk::FrontFace::eClockwise )
		.SetMultisampling()
		.SetBlendMode(
			VK_TRUE,
			vk::BlendFactor::eOne,
			vk::BlendFactor::eOneMinusSrcAlpha,
			vk::BlendOp::eAdd,
			vk::BlendFactor::eOne,
			vk::BlendFactor::eOneMinusSrcAlpha )
		.SetDepthTest( VK_FALSE, VK_FALSE, vk::CompareOp::eAlways )
		.SetColorAttachmentFormat( mRenderer.GetColorFormat() )
		.Build( mRenderer.GetDevice() );

	if( !mPipeline )
	{
		LOG_ERROR( "UIRenderInterface: failed to create UI pipeline" );
		return false;
	}

	mPipelineReady = true;
	return true;
}

Rml::CompiledGeometryHandle
UIRenderInterface::CompileGeometry( Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices )
{
	if( vertices.size() == 0 || indices.size() == 0 )
	{
		return 0;
	}

	std::vector<UIVertex> packed;
	packed.reserve( vertices.size() );
	for( const Rml::Vertex& src : vertices )
	{
		UIVertex dst{};
		dst.position = { src.position.x, src.position.y };
		dst.tex_coord = { src.tex_coord.x, src.tex_coord.y };
		dst.colour = {
			src.colour.red / 255.f,
			src.colour.green / 255.f,
			src.colour.blue / 255.f,
			src.colour.alpha / 255.f
		};
		packed.push_back( dst );
	}

	std::vector<uint32_t> index_data;
	index_data.reserve( indices.size() );
	for( int index : indices )
	{
		index_data.push_back( static_cast<uint32_t>( index ) );
	}

	const size_t vertex_bytes = packed.size() * sizeof( UIVertex );
	const size_t index_bytes = index_data.size() * sizeof( uint32_t );

	GeometryRecord record;
	record.vertex_buffer = ManagedBuffer::Create(
		*mRenderer.GetMemoryAllocator().allocator.get(),
		vertex_bytes,
		vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
		VMA_MEMORY_USAGE_CPU_TO_GPU );
	record.index_buffer = ManagedBuffer::Create(
		*mRenderer.GetMemoryAllocator().allocator.get(),
		index_bytes,
		vk::BufferUsageFlagBits::eIndexBuffer,
		VMA_MEMORY_USAGE_CPU_TO_GPU );
	record.vertex_buffer->Update( packed.data(), vertex_bytes );
	record.index_buffer->Update( index_data.data(), index_bytes );
	record.index_count = static_cast<uint32_t>( index_data.size() );

	vk::BufferDeviceAddressInfo address_info{};
	address_info.buffer = record.vertex_buffer->buffer;
	record.vertex_address = mRenderer.GetDevice().getBufferAddress( address_info );

	const Rml::CompiledGeometryHandle handle = mNextGeometryHandle++;
	mGeometries.emplace( handle, std::move( record ) );
	return handle;
}

void
UIRenderInterface::RenderGeometry( Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture )
{
	if( !mHasActiveCmd || !mPipelineReady )
	{
		return;
	}

	auto geo_it = mGeometries.find( geometry );
	if( geo_it == mGeometries.end() )
	{
		return;
	}

	const GeometryRecord& record = geo_it->second;

	uint32_t texture_index = 0;
	uint32_t has_texture = 0;
	if( texture != 0 )
	{
		auto tex_it = mTextures.find( texture );
		if( tex_it != mTextures.end() )
		{
			texture_index = tex_it->second.bindless_index;
			has_texture = 1;
		}
	}

	UIPushConstants push{};
	push.scale = {
		2.f / static_cast<float>( mTargetWidth ),
		2.f / static_cast<float>( mTargetHeight )
	};
	push.translation = { translation.x, translation.y };
	push.vertex_buffer_address = record.vertex_address;
	push.texture_index = texture_index;
	push.has_texture = has_texture;

	mActiveCmd.bindPipeline( vk::PipelineBindPoint::eGraphics, mPipeline.get() );

	vk::DescriptorSet bindless = mRenderer.GetBindlessDescriptorSet();
	mActiveCmd.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics,
		mPipelineLayout.get(),
		0, 1, &bindless,
		0, nullptr );

	mActiveCmd.pushConstants(
		mPipelineLayout.get(),
		vk::ShaderStageFlagBits::eVertex,
		0,
		sizeof( UIPushConstants ),
		&push );

	if( mScissorEnabled )
	{
		mActiveCmd.setScissor( 0, 1, &mScissor );
	}
	else
	{
		vk::Rect2D full{ { 0, 0 }, { mTargetWidth, mTargetHeight } };
		mActiveCmd.setScissor( 0, 1, &full );
	}

	mActiveCmd.bindIndexBuffer( record.index_buffer->buffer, 0, vk::IndexType::eUint32 );
	mActiveCmd.drawIndexed( record.index_count, 1, 0, 0, 0 );
}

void
UIRenderInterface::ReleaseGeometry( Rml::CompiledGeometryHandle geometry )
{
	auto it = mGeometries.find( geometry );
	if( it == mGeometries.end() )
	{
		return;
	}

	PendingGeometryRelease pending;
	pending.frame_number = mFrameNumber;
	pending.vertex_buffer = std::move( it->second.vertex_buffer );
	pending.index_buffer = std::move( it->second.index_buffer );
	mPendingReleases.push_back( std::move( pending ) );
	mGeometries.erase( it );
}

Rml::TextureHandle
UIRenderInterface::LoadTexture( Rml::Vector2i& texture_dimensions, const Rml::String& source )
{
	const std::string source_str( source.c_str() );
	auto named_it = mNamedTextures.find( source_str );
	if( named_it == mNamedTextures.end() )
	{
		named_it = mNamedTextures.find( basename_of( source_str ) );
	}

	if( named_it != mNamedTextures.end() )
	{
		auto tex_it = mTextures.find( named_it->second );
		if( tex_it != mTextures.end() )
		{
			texture_dimensions.x = static_cast<int>( tex_it->second.width );
			texture_dimensions.y = static_cast<int>( tex_it->second.height );
			return named_it->second;
		}
	}

	stbi_set_flip_vertically_on_load( 0 );
	int width = 0;
	int height = 0;
	int channels = 0;
	unsigned char* pixels = stbi_load( source_str.c_str(), &width, &height, &channels, 4 );
	stbi_set_flip_vertically_on_load( 1 );
	if( !pixels || width <= 0 || height <= 0 )
	{
		LOG_ERROR( "UIRenderInterface: failed to load texture " + source_str );
		if( pixels )
		{
			stbi_image_free( pixels );
		}
		return 0;
	}

	const size_t byte_size = static_cast<size_t>( width ) * static_cast<size_t>( height ) * 4u;
	auto image = mRenderer.UploadImage(
		pixels,
		byte_size,
		static_cast<uint32_t>( width ),
		static_cast<uint32_t>( height ),
		vk::Format::eR8G8B8A8Unorm,
		vk::ImageUsageFlagBits::eSampled,
		vk::ImageAspectFlagBits::eColor,
		1 );
	stbi_image_free( pixels );

	const uint32_t bindless_index = mRenderer.RegisterBindlessTexture(
		image->image_view.get(),
		mRenderer.GetDefaultSampler() );

	const Rml::TextureHandle handle = AllocateTextureHandle();
	TextureRecord record;
	record.bindless_index = bindless_index;
	record.owned_image = std::move( image );
	record.external = false;
	record.width = static_cast<uint32_t>( width );
	record.height = static_cast<uint32_t>( height );
	mTextures.emplace( handle, std::move( record ) );

	texture_dimensions.x = width;
	texture_dimensions.y = height;
	return handle;
}

Rml::TextureHandle
UIRenderInterface::GenerateTexture( Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions )
{
	if( source_dimensions.x <= 0 || source_dimensions.y <= 0 || source.size() == 0 )
	{
		return 0;
	}

	const size_t byte_size = static_cast<size_t>( source_dimensions.x ) * static_cast<size_t>( source_dimensions.y ) * 4u;
	if( source.size() < byte_size )
	{
		LOG_ERROR( "UIRenderInterface: GenerateTexture source too small" );
		return 0;
	}

	std::vector<Rml::byte> pixels( source.begin(), source.begin() + static_cast<std::ptrdiff_t>( byte_size ) );
	auto image = mRenderer.UploadImage(
		pixels.data(),
		byte_size,
		static_cast<uint32_t>( source_dimensions.x ),
		static_cast<uint32_t>( source_dimensions.y ),
		vk::Format::eR8G8B8A8Unorm,
		vk::ImageUsageFlagBits::eSampled,
		vk::ImageAspectFlagBits::eColor,
		1 );

	const uint32_t bindless_index = mRenderer.RegisterBindlessTexture(
		image->image_view.get(),
		mRenderer.GetDefaultSampler() );

	const Rml::TextureHandle handle = AllocateTextureHandle();
	TextureRecord record;
	record.bindless_index = bindless_index;
	record.owned_image = std::move( image );
	record.external = false;
	record.width = static_cast<uint32_t>( source_dimensions.x );
	record.height = static_cast<uint32_t>( source_dimensions.y );
	mTextures.emplace( handle, std::move( record ) );
	return handle;
}

void
UIRenderInterface::ReleaseTexture( Rml::TextureHandle texture )
{
	auto it = mTextures.find( texture );
	if( it == mTextures.end() )
	{
		return;
	}

	if( it->second.external )
	{
		// External images (BindImage) are owned by the scene; drop the handle only.
		for( auto named = mNamedTextures.begin(); named != mNamedTextures.end(); )
		{
			if( named->second == texture )
			{
				named = mNamedTextures.erase( named );
			}
			else
			{
				++named;
			}
		}
	}

	mTextures.erase( it );
}

void
UIRenderInterface::EnableScissorRegion( bool enable )
{
	mScissorEnabled = enable;
}

void
UIRenderInterface::SetScissorRegion( Rml::Rectanglei region )
{
	const int x = std::max( region.Left(), 0 );
	const int y = std::max( region.Top(), 0 );
	const int right = std::min( region.Right(), static_cast<int>( mTargetWidth ) );
	const int bottom = std::min( region.Bottom(), static_cast<int>( mTargetHeight ) );
	const int width = std::max( right - x, 0 );
	const int height = std::max( bottom - y, 0 );

	mScissor.offset = vk::Offset2D{ x, y };
	mScissor.extent = vk::Extent2D{
		static_cast<uint32_t>( width ),
		static_cast<uint32_t>( height )
	};
}

Rml::TextureHandle
UIRenderInterface::AllocateTextureHandle()
{
	return mNextTextureHandle++;
}

void
UIRenderInterface::DrainDeferredReleases()
{
	while( !mPendingReleases.empty() )
	{
		const PendingGeometryRelease& pending = mPendingReleases.front();
		if( mFrameNumber < pending.frame_number + static_cast<uint64_t>( Renderer::MAX_FRAMES_IN_FLIGHT ) )
		{
			break;
		}
		mPendingReleases.pop_front();
	}
}

std::string
UIRenderInterface::basename_of( const std::string& path )
{
	const size_t slash = path.find_last_of( "/\\" );
	if( slash == std::string::npos )
	{
		return path;
	}
	return path.substr( slash + 1 );
}
