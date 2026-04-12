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
#include <graphics/ShaderReflection.h>
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
	Impl( eage::graphics::Renderer& renderer, eage::graphics::SceneRenderPass& scene_pass, ECSRegistry& ecs_registry )
		: mRenderer( renderer )
		, mScenePass( scene_pass )
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

	ResourceId CreateMeshBuffer( const std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices,
								 uint32_t first_index, uint32_t index_count, uint32_t vertex_offset )
	{
		auto mesh = mRenderer.UploadMesh( indices, vertices );
		mesh->first_index = first_index;
		mesh->index_count = index_count;
		mesh->vertex_offset = vertex_offset;
		return mMeshBuffers.Store( std::move( mesh ) );
	}

	ResourceId CreateMaterial( const MaterialProperty& material_property )
	{
		// Load shaders and retain SPIR-V bytecode for reflection
		auto vert_asset = load_shader_from_file( mRenderer.GetDevice(), material_property.vertex_shader_path );
		auto frag_asset = load_shader_from_file( mRenderer.GetDevice(), material_property.fragment_shader_path );

		if( !vert_asset || !frag_asset )
		{
			LOG_ERROR( "Failed to load shaders for material" );
			return INVALID_ID;
		}

		// Reflect descriptor bindings from SPIR-V
		auto reflection = merge_reflection(
			reflect_shader( vert_asset->spirv_code ),
			reflect_shader( frag_asset->spirv_code ) );

		// Build material-specific descriptor set layouts (skip built-in sets 0 and 1)
		auto material_layouts = build_descriptor_set_layouts(
			mRenderer.GetDevice(), reflection,
			vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
			{ 0, 1 } );

		// Assemble all layouts for the pipeline (set 0, set 1, then reflected sets)
		std::vector<vk::DescriptorSetLayout> all_layouts = {
			mRenderer.GetBuiltInDescriptorSetLayouts().global.get(),
			mRenderer.GetBuiltInDescriptorSetLayouts().per_object.get()
		};

		// Track material-set layouts that we own (first reflected set is used for the material descriptor)
		vk::DescriptorSetLayout material_set_layout = nullptr;
		for( auto& [set_number, layout] : material_layouts )
		{
			if( !material_set_layout )
			{
				material_set_layout = layout.get();
			}
			all_layouts.push_back( layout.get() );
		}

		auto pipeline = CreateOrGetPipeline( material_property, all_layouts, *vert_asset, *frag_asset );

		auto material = std::make_unique<Material>();
		material->pipeline = pipeline;

		if( material_set_layout )
		{
			material->descriptor = std::make_unique<StaticDescriptor>( mRenderer, material_set_layout );
		}

		// Build a name-to-binding lookup from the reflected bindings for the material set(s)
		std::unordered_map<std::string, const DescriptorBindingInfo*> name_to_binding;
		std::vector<const DescriptorBindingInfo*> sampler_bindings_ordered;
		for( const auto& b : reflection.bindings )
		{
			if( b.set <= 1 )
			{
				continue;
			}
			if( !b.name.empty() )
			{
				name_to_binding[b.name] = &b;
			}
			if( b.descriptor_type == vk::DescriptorType::eCombinedImageSampler )
			{
				sampler_bindings_ordered.push_back( &b );
			}
		}

		// Sort sampler bindings by (set, binding) for order-based fallback matching
		std::sort( sampler_bindings_ordered.begin(), sampler_bindings_ordered.end(),
			[]( const DescriptorBindingInfo* a, const DescriptorBindingInfo* b )
			{
				if( a->set != b->set ) return a->set < b->set;
				return a->binding < b->binding;
			} );

		// Write textures: match by name first, then fall back to order-based matching
		size_t order_index = 0;
		for( const auto& texture_binding : material_property.textures )
		{
			const DescriptorBindingInfo* reflected = nullptr;

			// Try name-based match
			if( !texture_binding.name.empty() )
			{
				auto it = name_to_binding.find( texture_binding.name );
				if( it != name_to_binding.end() )
				{
					reflected = it->second;
				}
			}

			// Fall back to order-based match
			if( !reflected && order_index < sampler_bindings_ordered.size() )
			{
				reflected = sampler_bindings_ordered[order_index];
			}
			++order_index;

			if( !reflected )
			{
				LOG_ERROR() << "No matching shader binding for texture: " << texture_binding.texture_path;
				continue;
			}

			auto img_it = mImagePathToIdMap.find( texture_binding.texture_path );
			if( img_it == mImagePathToIdMap.end() )
			{
				LOG_ERROR() << "Failed to find image buffer for texture path: " << texture_binding.texture_path;
				continue;
			}

			auto texture = mImages.Get( img_it->second );
			if( !texture )
			{
				LOG_ERROR() << "Invalid image buffer for texture path: " << texture_binding.texture_path;
				continue;
			}

			ResourceId sampler_id = CreateSampler(
				texture_binding.min_filter,
				texture_binding.mag_filter );

			vk::Sampler sampler = GetSampler( sampler_id );

			material->descriptor->WriteImage(
				reflected->binding,
				vk::DescriptorType::eCombinedImageSampler,
				texture->image_view.get(),
				vk::ImageLayout::eShaderReadOnlyOptimal,
				sampler );
		}

		// Write uniforms: match by name first, then by order among reflected uniform bindings
		std::vector<const DescriptorBindingInfo*> uniform_bindings_ordered;
		for( const auto& b : reflection.bindings )
		{
			if( b.set <= 1 ) continue;
			if( b.descriptor_type == vk::DescriptorType::eUniformBuffer ||
				b.descriptor_type == vk::DescriptorType::eStorageBuffer )
			{
				uniform_bindings_ordered.push_back( &b );
			}
		}
		std::sort( uniform_bindings_ordered.begin(), uniform_bindings_ordered.end(),
			[]( const DescriptorBindingInfo* a, const DescriptorBindingInfo* b )
			{
				if( a->set != b->set ) return a->set < b->set;
				return a->binding < b->binding;
			} );

		size_t uniform_order = 0;
		for( const auto& uniform_binding : material_property.uniforms )
		{
			const DescriptorBindingInfo* reflected = nullptr;

			if( !uniform_binding.name.empty() )
			{
				auto it = name_to_binding.find( uniform_binding.name );
				if( it != name_to_binding.end() )
				{
					reflected = it->second;
				}
			}

			if( !reflected && uniform_order < uniform_bindings_ordered.size() )
			{
				reflected = uniform_bindings_ordered[uniform_order];
			}
			++uniform_order;

			if( !reflected || !uniform_binding.data )
			{
				continue;
			}

			auto uniform_buffer = ManagedBuffer::Create(
				*mRenderer.GetMemoryAllocator().allocator.get(),
				uniform_binding.size,
				vk::BufferUsageFlagBits::eUniformBuffer,
				VMA_MEMORY_USAGE_CPU_TO_GPU );

			uniform_buffer->Update( uniform_binding.data, uniform_binding.size, 0 );

			material->descriptor->WriteBuffer(
				reflected->binding,
				reflected->descriptor_type,
				uniform_buffer->buffer,
				uniform_binding.size );
		}

		// Keep material layouts alive alongside the material
		for( auto& [set_number, layout] : material_layouts )
		{
			mOwnedDescriptorSetLayouts.push_back( std::move( layout ) );
		}

		return mMaterials.Store( std::move( material ) );
	}

	ResourceId CreateImageBuffer( const std::string& file_path )
	{
		if( auto it = mImagePathToIdMap.find( file_path ); it != mImagePathToIdMap.end() )
		{
			return it->second;
		}

		assets::ImageLoader image_loader;
		auto image = image_loader.LoadImage( file_path );

		auto texture = mRenderer.UploadImage(
			image.data.data(), sizeof( unsigned char ) * image.data.size(),
			image.width, image.height, vk::Format::eR8G8B8A8Srgb,
			vk::ImageUsageFlagBits::eSampled, vk::ImageAspectFlagBits::eColor, 1 );

		mImagePathToIdMap[file_path] = mImages.Store( std::move( texture ) );
		return mImagePathToIdMap[file_path];
	}

	ResourceId CreateSpriteMesh( float width, float height, glm::vec2 uv_min, glm::vec2 uv_max )
	{
		auto rect = made_rect_vertices( { 0, 0, 0 }, width, height );
		rect.vertices[0].uv_x = uv_max.x; rect.vertices[0].uv_y = uv_min.y;
		rect.vertices[1].uv_x = uv_max.x; rect.vertices[1].uv_y = uv_max.y;
		rect.vertices[2].uv_x = uv_min.x; rect.vertices[2].uv_y = uv_min.y;
		rect.vertices[3].uv_x = uv_min.x; rect.vertices[3].uv_y = uv_max.y;
		return CreateMeshBuffer( rect.indices, rect.vertices, 0, 6, 0 );
	}

	void AttachRenderable( Entity entity, ResourceId mesh_id, ResourceId material_id, bool visible )
	{
		auto ubo_id = CreateDynamicUniformBuffer( sizeof( MeshUniformData ) );
		auto descriptor_id = CreateDynamicDescriptorSet( mRenderer.GetBuiltInDescriptorSetLayouts().per_object.get() );
		GetDescriptorSet( descriptor_id )->WriteBuffer(
			PER_OBJECT_MESH_DATA_BINDING,
			vk::DescriptorType::eUniformBufferDynamic,
			GetUniformBuffer( ubo_id )->buffer,
			sizeof( MeshUniformData ) );
		mECSRegistry.AddComponent( entity, RenderComponent{
			mesh_id, material_id, ubo_id, descriptor_id, visible } );
	}

	void AttachSprite( Entity entity, ResourceId material_id, float width, float height, glm::vec2 uv_min, glm::vec2 uv_max, bool visible )
	{
		auto mesh_id = CreateSpriteMesh( width, height, uv_min, uv_max );
		AttachRenderable( entity, mesh_id, material_id, visible );
	}

	void SetCamera( const AbstractCamera& camera )
	{
		auto current_frame = mRenderer.GetCurrentFrameIndex();
		SceneGlobalData scene_global_data;
		scene_global_data.view = camera.GetViewMatrix();
		scene_global_data.proj = camera.GetProjectionMatrix();
		scene_global_data.view_proj = scene_global_data.proj * scene_global_data.view;
		GetGlobalUniformBuffer()->Update( &scene_global_data, sizeof( SceneGlobalData ), sizeof( SceneGlobalData ) * current_frame );
	}

	void Update()
	{
		auto& renderable_entities = mECSRegistry.GetComponentMap<RenderComponent>();
		for( const auto& [entity, render_cmp] : renderable_entities )
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

			mScenePass.AddRenderInfo( std::move( render_info ) );
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
	eage::graphics::SceneRenderPass& mScenePass;
	ECSRegistry& mECSRegistry;

	ResourceId mGlobalDescriptorSetId;
	ResourceId mGlobalUniformBufferId;

	ResourceManager<std::unique_ptr<GPUMeshBuffers>> mMeshBuffers;
	ResourceManager<std::unique_ptr<Material>> mMaterials;
	ResourceManager<ManagedBuffer::Ptr> mUniformBuffers;
	ResourceManager<ManagedImage::Ptr> mImages;
	ResourceManager<std::unique_ptr<AbstractUniformDescriptor>> mDescriptorSets;
	ResourceManager<std::unique_ptr<vk::UniqueSampler>> mSamplers;

	std::unordered_map<size_t, std::shared_ptr<RenderPipeline>> mPipelineCache;
	std::unordered_map<std::string, ResourceId> mImagePathToIdMap;
	std::unordered_map<size_t, ResourceId> mSamplerCache;

	std::vector<vk::UniqueDescriptorSetLayout> mOwnedDescriptorSetLayouts;
};

// ---------------------------------------------------------------------------
// RenderSystem forwarding
// ---------------------------------------------------------------------------

RenderSystem::RenderSystem( eage::graphics::Renderer& renderer, eage::graphics::SceneRenderPass& scene_pass, ECSRegistry& ecs_registry )
	: mImpl( std::make_unique<Impl>( renderer, scene_pass, ecs_registry ) )
{
}

RenderSystem::~RenderSystem() = default;

ResourceId
RenderSystem::CreateMeshBuffer( const std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices,
								uint32_t first_index, uint32_t index_count, uint32_t vertex_offset )
{
	return mImpl->CreateMeshBuffer( indices, vertices, first_index, index_count, vertex_offset );
}

ResourceId
RenderSystem::CreateMaterial( const MaterialProperty& material_property )
{
	return mImpl->CreateMaterial( material_property );
}

ResourceId
RenderSystem::CreateImageBuffer( const std::string& file_path )
{
	return mImpl->CreateImageBuffer( file_path );
}

ResourceId
RenderSystem::CreateSpriteMesh( float width, float height, glm::vec2 uv_min, glm::vec2 uv_max )
{
	return mImpl->CreateSpriteMesh( width, height, uv_min, uv_max );
}

void
RenderSystem::AttachRenderable( Entity entity, ResourceId mesh_id, ResourceId material_id, bool visible )
{
	mImpl->AttachRenderable( entity, mesh_id, material_id, visible );
}

void
RenderSystem::AttachSprite( Entity entity, ResourceId material_id, float width, float height, glm::vec2 uv_min, glm::vec2 uv_max, bool visible )
{
	mImpl->AttachSprite( entity, material_id, width, height, uv_min, uv_max, visible );
}

void
RenderSystem::SetCamera( const AbstractCamera& camera )
{
	mImpl->SetCamera( camera );
}

void
RenderSystem::Update()
{
	mImpl->Update();
}
