#include "RenderSystem.h"

#include <assets/ImageLoader.h>
#include <ecs/components/Basics.h>
#include <ecs/components/Render.h>
#include <graphics/BuiltInMeshes.h>
#include <graphics/BuiltInUniforms.h>
#include <graphics/Camera.h>
#include <graphics/Material.h>
#include <graphics/MaterialProperty.h>
#include <graphics/ManagedVulkanResources.h>
#include <graphics/Renderer.h>
#include <graphics/RenderInfo.h>
#include <graphics/SceneRenderPass.h>
#include <graphics/VulkanDescriptor.h>
#include <graphics/VulkanMesh.h>
#include <graphics/VulkanShader.h>
#include <graphics/VulkanTypeMapping.h>
#include <graphics/VMAWrapper.h>
#include <utility/Logger.h>

using namespace eage::ecs;
using namespace eage::graphics;
using namespace utility;

namespace
{
	constexpr uint32_t GLOBAL_SCENE_DATA_BINDING = 0;
	constexpr uint32_t PER_OBJECT_MESH_DATA_BINDING = 0;
}

// ---------------------------------------------------------------------------
// Impl
// ---------------------------------------------------------------------------

struct RenderSystem::Impl
{
	enum class PendingKind
	{
		UniformBuffer,
		DescriptorSet
	};

	struct PendingDelete
	{
		PendingKind kind = PendingKind::UniformBuffer;
		ResourceId id = INVALID_ID;
		int frames_remaining = 0;
	};

	Impl( eage::graphics::Renderer& renderer, ECSRegistry& ecs_registry )
		: mRenderer( renderer )
		, mECSRegistry( ecs_registry )
	{
		mGlobalDescriptorSetId = CreateDynamicDescriptorSet( mRenderer.GetBuiltInDescriptorSetLayouts().global.get() );
		mGlobalUniformBufferId = CreateDynamicUniformBuffer( sizeof( SceneGlobalData ) );
		GetDescriptorSet( mGlobalDescriptorSetId )->WriteBuffer(
			GLOBAL_SCENE_DATA_BINDING,
			vk::DescriptorType::eUniformBufferDynamic,
			GetGlobalUniformBuffer()->buffer,
			sizeof( SceneGlobalData ) );
	}

	// ----- Resource creation -----

	ResourceHandle CreateMeshBuffer( const std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices,
								 uint32_t first_index, uint32_t index_count, uint32_t vertex_offset )
	{
		auto mesh = mRenderer.UploadMesh( indices, vertices );
		mesh->first_index = first_index;
		mesh->index_count = index_count;
		mesh->vertex_offset = vertex_offset;
		ResourceId id = mMeshBuffers.Store( std::move( mesh ) );
		return ResourceHandle::Adopt( mMeshBuffers, id );
	}

	ResourceHandle CreateMaterial( const MaterialProperty& material_property )
	{
		auto vert_asset = load_shader_from_file( mRenderer.GetDevice(), material_property.vertex_shader_path );
		auto frag_asset = load_shader_from_file( mRenderer.GetDevice(), material_property.fragment_shader_path );

		if( !vert_asset || !frag_asset )
		{
			LOG_ERROR( "Failed to load shaders for material" );
			return ResourceHandle{};
		}

		std::vector<vk::DescriptorSetLayout> all_layouts = {
			mRenderer.GetBuiltInDescriptorSetLayouts().global.get(),
			mRenderer.GetBuiltInDescriptorSetLayouts().per_object.get(),
			mRenderer.GetBuiltInDescriptorSetLayouts().bindless.get()
		};

		auto pipeline = CreateOrGetPipeline( material_property, all_layouts, *vert_asset, *frag_asset );

		auto material = std::make_unique<Material>();
		material->pipeline = pipeline;
		ResourceId id = mMaterials.Store( std::move( material ) );
		return ResourceHandle::Adopt( mMaterials, id );
	}

	uint32_t CreateTexture( const std::string& file_path )
	{
		if( auto it = mTexturePathToBindlessIndex.find( file_path ); it != mTexturePathToBindlessIndex.end() )
		{
			return it->second;
		}

		assets::ImageLoader image_loader;
		auto image = image_loader.LoadImage( file_path );
		if( image.data.empty() || image.width <= 0 || image.height <= 0 )
		{
			LOG_ERROR( "Invalid image data for: " + file_path );
			throw std::runtime_error( "Failed to load image: " + file_path );
		}

		auto texture = mRenderer.UploadImage(
			image.data.data(), sizeof( unsigned char ) * image.data.size(),
			image.width, image.height, vk::Format::eR8G8B8A8Srgb,
			vk::ImageUsageFlagBits::eSampled, vk::ImageAspectFlagBits::eColor, 1 );

		const uint32_t bindless_index = mRenderer.RegisterBindlessTexture(
			texture->image_view.get(),
			mRenderer.GetDefaultSampler() );

		mImages.Store( std::move( texture ) );
		mTexturePathToBindlessIndex[file_path] = bindless_index;
		return bindless_index;
	}

	ResourceHandle CreateSpriteMesh( float width, float height )
	{
		auto rect = made_rect_vertices( { 0, 0, 0 }, width, height );
		rect.vertices[0].uv_x = 1.f; rect.vertices[0].uv_y = 0.f;
		rect.vertices[1].uv_x = 1.f; rect.vertices[1].uv_y = 1.f;
		rect.vertices[2].uv_x = 0.f; rect.vertices[2].uv_y = 0.f;
		rect.vertices[3].uv_x = 0.f; rect.vertices[3].uv_y = 1.f;
		return CreateMeshBuffer( rect.indices, rect.vertices, 0, 6, 0 );
	}

	void AttachRenderable( Entity entity, ResourceId mesh_id, ResourceId material_id, uint32_t texture_index, bool visible )
	{
		if( mECSRegistry.HasComponent<RenderComponent>( entity ) )
		{
			DetachRenderable( entity );
		}

		if( mesh_id != INVALID_ID )
		{
			mMeshBuffers.AddReference( mesh_id );
		}
		if( material_id != INVALID_ID )
		{
			mMaterials.AddReference( material_id );
		}

		auto ubo_id = CreateDynamicUniformBuffer( sizeof( MeshUniformData ) );
		auto descriptor_id = CreateDynamicDescriptorSet( mRenderer.GetBuiltInDescriptorSetLayouts().per_object.get() );
		GetDescriptorSet( descriptor_id )->WriteBuffer(
			PER_OBJECT_MESH_DATA_BINDING,
			vk::DescriptorType::eUniformBufferDynamic,
			GetUniformBuffer( ubo_id )->buffer,
			sizeof( MeshUniformData ) );
		mECSRegistry.AddComponent( entity, RenderComponent{
			.mesh_buffer_id = mesh_id,
			.material_id = material_id,
			.mesh_uniform_data_dynamic_id = ubo_id,
			.mesh_descriptor_id = descriptor_id,
			.texture_index = texture_index,
			.visible = visible } );
	}

	void AttachSprite( Entity entity, ResourceId material_id, float width, float height, uint32_t texture_index, bool visible )
	{
		ResourceHandle mesh = CreateSpriteMesh( width, height );
		AttachRenderable( entity, mesh.Get(), material_id, texture_index, visible );
		// Drop creator hold so the entity is the sole mesh owner.
		mesh.Reset();
	}

	void DetachRenderable( Entity entity )
	{
		if( !mECSRegistry.HasComponent<RenderComponent>( entity ) )
		{
			return;
		}

		auto& render = mECSRegistry.GetComponent<RenderComponent>( entity );

		if( render.mesh_buffer_id != INVALID_ID )
		{
			mMeshBuffers.RemoveReference( render.mesh_buffer_id );
			render.mesh_buffer_id = INVALID_ID;
		}
		if( render.material_id != INVALID_ID )
		{
			mMaterials.RemoveReference( render.material_id );
			render.material_id = INVALID_ID;
		}

		if( render.mesh_uniform_data_dynamic_id != INVALID_ID )
		{
			mPendingDeletes.push_back( {
				PendingKind::UniformBuffer,
				render.mesh_uniform_data_dynamic_id,
				Renderer::MAX_FRAMES_IN_FLIGHT } );
			render.mesh_uniform_data_dynamic_id = INVALID_ID;
		}
		if( render.mesh_descriptor_id != INVALID_ID )
		{
			mPendingDeletes.push_back( {
				PendingKind::DescriptorSet,
				render.mesh_descriptor_id,
				Renderer::MAX_FRAMES_IN_FLIGHT } );
			render.mesh_descriptor_id = INVALID_ID;
		}

		mECSRegistry.RemoveComponent<RenderComponent>( entity );
	}

	void DrainPendingDeletes()
	{
		for( size_t i = 0; i < mPendingDeletes.size(); )
		{
			auto& pending = mPendingDeletes[i];
			--pending.frames_remaining;
			if( pending.frames_remaining > 0 )
			{
				++i;
				continue;
			}

			FreePendingResource( pending );
			mPendingDeletes[i] = mPendingDeletes.back();
			mPendingDeletes.pop_back();
		}
	}

	void FlushPendingDeletes()
	{
		for( const auto& pending : mPendingDeletes )
		{
			FreePendingResource( pending );
		}
		mPendingDeletes.clear();
	}

	void FreePendingResource( const PendingDelete& pending )
	{
		switch( pending.kind )
		{
			case PendingKind::UniformBuffer:
				mUniformBuffers.RemoveReference( pending.id );
				break;
			case PendingKind::DescriptorSet:
				mDescriptorSets.RemoveReference( pending.id );
				break;
		}
	}

	void SetCamera( const AbstractCamera& camera, glm::vec2 virtual_resolution )
	{
		auto current_frame = mRenderer.GetCurrentFrameIndex();
		SceneGlobalData scene_global_data;
		scene_global_data.view = camera.GetViewMatrix();
		scene_global_data.proj = camera.GetProjectionMatrix();
		scene_global_data.view_proj = scene_global_data.proj * scene_global_data.view;
		scene_global_data.virtual_resolution = virtual_resolution;
		GetGlobalUniformBuffer()->Update( &scene_global_data, sizeof( SceneGlobalData ), sizeof( SceneGlobalData ) * current_frame );
	}

	void SetScenePass( eage::graphics::SceneRenderPass* scene_pass )
	{
		mScenePass = scene_pass;
	}

	void Update()
	{
		DrainPendingDeletes();

		if( mScenePass == nullptr )
		{
			return;
		}

		auto& renderable_entities = mECSRegistry.GetComponentMap<RenderComponent>();
		for( auto [entity, render_cmp] : renderable_entities )
		{
			if( !render_cmp.visible )
			{
				continue;
			}

			RenderInfo render_info{};

			if( auto material = mMaterials.Get( render_cmp.material_id ) )
			{
				render_info.material = material;
			}

			if( auto mesh_buffer = mMeshBuffers.Get( render_cmp.mesh_buffer_id ) )
			{
				render_info.mesh_buffer = mesh_buffer;
				render_info.first_index   = mesh_buffer->first_index;
				render_info.index_count   = mesh_buffer->index_count;
				render_info.vertex_offset = mesh_buffer->vertex_offset;
			}
			else
			{
				LOG_ERROR( "Mesh buffer not found for entity: " + std::to_string( entity ) );
				continue;
			}

			if( auto mesh_descriptor = mDescriptorSets.Get( render_cmp.mesh_descriptor_id ) )
			{
				render_info.mesh_descriptor = mesh_descriptor;
			}
			else
			{
				LOG_ERROR( "Mesh descriptor not found for entity: " + std::to_string( entity ) );
				continue;
			}

			if( auto mesh_uniform_data = mUniformBuffers.Get( render_cmp.mesh_uniform_data_dynamic_id ) )
			{
				render_info.mesh_uniform_data_dynamic = mesh_uniform_data;
			}
			else
			{
				LOG_ERROR( "Mesh uniform data not found for entity: " + std::to_string( entity ) );
				continue;
			}

			if( mECSRegistry.HasComponent<TransformComponent>( entity ) )
			{
				auto& transform = mECSRegistry.GetComponent<TransformComponent>( entity );
				render_info.model_matrix = transform.GetWorldMatrix();
			}
			else
			{
				render_info.model_matrix = glm::mat4( 1.0f );
			}

			render_info.uv_rect = render_cmp.uv_rect;
			render_info.texture_index = render_cmp.texture_index;

			mScenePass->AddRenderInfo( std::move( render_info ) );
		}
	}

	// ----- Internal helpers -----

	ResourceId CreateUniformBuffer( size_t data_size )
	{
		auto buffer = ManagedBuffer::Create(
			*mRenderer.GetMemoryAllocator().allocator.get(),
			data_size,
			vk::BufferUsageFlagBits::eUniformBuffer,
			VMA_MEMORY_USAGE_CPU_TO_GPU );
		return mUniformBuffers.Store( std::move( buffer ) );
	}

	ResourceId CreateDynamicUniformBuffer( size_t data_size )
	{
		size_t buffer_size = data_size * mRenderer.GetFrames().size();
		auto buffer = ManagedBuffer::Create(
			*mRenderer.GetMemoryAllocator().allocator.get(),
			buffer_size,
			vk::BufferUsageFlagBits::eUniformBuffer,
			VMA_MEMORY_USAGE_CPU_TO_GPU );
		return mUniformBuffers.Store( std::move( buffer ) );
	}

	ResourceId CreateDescriptorSet( vk::DescriptorSetLayout layout )
	{
		auto descriptor = std::make_unique<StaticDescriptor>( mRenderer, layout );
		return mDescriptorSets.Store( std::move( descriptor ) );
	}

	ResourceId CreateDynamicDescriptorSet( vk::DescriptorSetLayout layout )
	{
		auto descriptor = std::make_unique<DynamicDescriptor>( mRenderer, layout );
		return mDescriptorSets.Store( std::move( descriptor ) );
	}

	ResourceId CreateSampler( TextureFilter min_filter, TextureFilter mag_filter )
	{
		size_t hash = std::hash<int>{}( static_cast<int>( min_filter ) );
		hash ^= std::hash<int>{}( static_cast<int>( mag_filter ) ) + 0x9e3779b9 + ( hash << 6 ) + ( hash >> 2 );

		auto it = mSamplerCache.find( hash );
		if( it != mSamplerCache.end() )
		{
			return it->second;
		}

		auto sampler = mRenderer.CreateSampler( ToVulkan( min_filter ), ToVulkan( mag_filter ) );
		auto sampler_ptr = std::make_unique<vk::UniqueSampler>( std::move( sampler ) );
		ResourceId sampler_id = mSamplers.Store( std::move( sampler_ptr ) );
		mSamplerCache[hash] = sampler_id;
		return sampler_id;
	}

	ManagedBuffer* GetGlobalUniformBuffer()
	{
		return mUniformBuffers.Get( mGlobalUniformBufferId );
	}

	ManagedImage* GetImageBuffer( ResourceId id )
	{
		return mImages.Get( id );
	}

	ManagedBuffer* GetUniformBuffer( ResourceId id )
	{
		return mUniformBuffers.Get( id );
	}

	AbstractUniformDescriptor* GetDescriptorSet( ResourceId id )
	{
		return mDescriptorSets.Get( id );
	}

	vk::Sampler GetSampler( ResourceId id )
	{
		auto sampler_ptr = mSamplers.Get( id );
		return sampler_ptr ? sampler_ptr->get() : vk::Sampler{};
	}

	std::shared_ptr<RenderPipeline> CreateOrGetPipeline(
		const MaterialProperty& property,
		const std::vector<vk::DescriptorSetLayout>& all_layouts,
		ShaderAsset& vert_asset,
		ShaderAsset& frag_asset )
	{
		size_t hash = HashMaterialProperty( property );

		auto it = mPipelineCache.find( hash );
		if( it != mPipelineCache.end() )
		{
			return it->second;
		}

		auto pipeline = std::make_shared<RenderPipeline>();

		vk::PipelineLayoutCreateInfo layout_info;
		layout_info.setLayoutCount = static_cast<uint32_t>( all_layouts.size() );
		layout_info.pSetLayouts = all_layouts.data();
		layout_info.pushConstantRangeCount = 0;
		layout_info.pPushConstantRanges = nullptr;

		auto pipeline_layout = mRenderer.GetDevice().createPipelineLayoutUnique( layout_info );

		VulkanPipelineBuilder pipeline_builder;
		pipeline->pipeline = pipeline_builder
			.SetShaders( vert_asset.module.get(), frag_asset.module.get() )
			.SetPipelineLayout( pipeline_layout.get() )
			.SetInputTopology( ToVulkan( property.topology ) )
			.SetPolygonMode( ToVulkan( property.polygon_mode ) )
			.SetCullMode( ToVulkan( property.cull_mode ), ToVulkan( property.front_face ) )
			.SetMultisampling()
			.SetBlendMode(
				vk::Bool32( property.blend_enable ),
				ToVulkan( property.src_color_blend ),
				ToVulkan( property.dst_color_blend ),
				ToVulkan( property.color_blend_op ),
				ToVulkan( property.src_alpha_blend ),
				ToVulkan( property.dst_alpha_blend ) )
			.SetColorAttachmentFormat( mRenderer.GetColorFormat() )
			.SetDepthTest(
				vk::Bool32( property.depth_test ),
				vk::Bool32( property.depth_write ),
				ToVulkan( property.depth_compare ) )
			.SetDepthFormat( mRenderer.GetDepthFormat() )
			.Build( mRenderer.GetDevice() );

		pipeline->layout = std::move( pipeline_layout );
		pipeline->global_descriptor = GetDescriptorSet( mGlobalDescriptorSetId );

		mPipelineCache[hash] = pipeline;
		return pipeline;
	}

	size_t HashMaterialProperty( const MaterialProperty& property )
	{
		size_t hash = std::hash<std::string>{}( property.vertex_shader_path );
		hash ^= std::hash<std::string>{}( property.fragment_shader_path ) + 0x9e3779b9 + ( hash << 6 ) + ( hash >> 2 );
		hash ^= static_cast<size_t>( property.topology ) + 0x9e3779b9 + ( hash << 6 ) + ( hash >> 2 );
		hash ^= static_cast<size_t>( property.polygon_mode ) + 0x9e3779b9 + ( hash << 6 ) + ( hash >> 2 );
		return hash;
	}

	// ----- Members -----

	eage::graphics::Renderer& mRenderer;
	eage::graphics::SceneRenderPass* mScenePass = nullptr;
	ECSRegistry& mECSRegistry;

	ResourceId mGlobalDescriptorSetId;
	ResourceId mGlobalUniformBufferId;

	ResourceStore<std::unique_ptr<GPUMeshBuffers>> mMeshBuffers;
	ResourceStore<std::unique_ptr<Material>> mMaterials;
	ResourceStore<ManagedBuffer::Ptr> mUniformBuffers;
	ResourceStore<ManagedImage::Ptr> mImages;
	ResourceStore<std::unique_ptr<AbstractUniformDescriptor>> mDescriptorSets;
	ResourceStore<std::unique_ptr<vk::UniqueSampler>> mSamplers;

	std::unordered_map<size_t, std::shared_ptr<RenderPipeline>> mPipelineCache;
	std::unordered_map<std::string, uint32_t> mTexturePathToBindlessIndex;
	std::unordered_map<size_t, ResourceId> mSamplerCache;
	std::vector<PendingDelete> mPendingDeletes;

};

// ---------------------------------------------------------------------------
// RenderSystem forwarding
// ---------------------------------------------------------------------------

RenderSystem::RenderSystem( eage::graphics::Renderer& renderer, ECSRegistry& ecs_registry )
	: mImpl( std::make_unique<Impl>( renderer, ecs_registry ) )
{
	ecs_registry.Subscribe( this );
}

RenderSystem::~RenderSystem()
{
	mImpl->mECSRegistry.Unsubscribe( this );
	mImpl->FlushPendingDeletes();
}

ResourceHandle
RenderSystem::CreateMeshBuffer( const std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices,
								uint32_t first_index, uint32_t index_count, uint32_t vertex_offset )
{
	return mImpl->CreateMeshBuffer( indices, vertices, first_index, index_count, vertex_offset );
}

ResourceHandle
RenderSystem::CreateMaterial( const MaterialProperty& material_property )
{
	return mImpl->CreateMaterial( material_property );
}

uint32_t
RenderSystem::CreateTexture( const std::string& file_path )
{
	return mImpl->CreateTexture( file_path );
}

ResourceHandle
RenderSystem::CreateSpriteMesh( float width, float height )
{
	return mImpl->CreateSpriteMesh( width, height );
}

void
RenderSystem::AttachRenderable( Entity entity, ResourceId mesh_id, ResourceId material_id, uint32_t texture_index, bool visible )
{
	mImpl->AttachRenderable( entity, mesh_id, material_id, texture_index, visible );
}

void
RenderSystem::AttachSprite( Entity entity, ResourceId material_id, float width, float height, uint32_t texture_index, bool visible )
{
	mImpl->AttachSprite( entity, material_id, width, height, texture_index, visible );
}

void
RenderSystem::DetachRenderable( Entity entity )
{
	mImpl->DetachRenderable( entity );
}

void
RenderSystem::FlushPendingDeletes()
{
	mImpl->FlushPendingDeletes();
}

void
RenderSystem::OnEntityDestroying( Entity entity )
{
	mImpl->DetachRenderable( entity );
}

void
RenderSystem::SetCamera( const AbstractCamera& camera, glm::vec2 virtual_resolution )
{
	mImpl->SetCamera( camera, virtual_resolution );
}

void
RenderSystem::SetScenePass( eage::graphics::SceneRenderPass* scene_pass )
{
	// @todo: Should RenderSystem knows about SceneRenderPass? Will it be better if it just use the active
	// `RenderPass`? But this might need a render graph which is overkill for now.
	mImpl->SetScenePass( scene_pass );
}

void
RenderSystem::Update()
{
	mImpl->Update();
}
