#include "RenderSystem.h"

#include <assets/ImageLoader.h>
#include <ecs/components/Basics.h>
#include <ecs/components/Render.h>
#include <graphics/BuiltInMeshes.h>
#include <graphics/BuiltInUniforms.h>
#include <graphics/Renderer.h>
#include <graphics/VulkanMesh.h>
#include <graphics/VulkanDescriptor.h>
#include <graphics/VulkanShader.h>
#include <graphics/VMAWrapper.h>
#include <graphics/ManagedVulkanResources.h>
#include <utility/Logger.h>

using namespace eage::ecs;
using namespace utility;

namespace
{
	constexpr uint32_t GLOBAL_SCENE_DATA_BINDING = 0;
	constexpr uint32_t PER_OBJECT_MESH_DATA_BINDING = 0;
}

RenderSystem::RenderSystem( eage::graphics::Renderer& renderer, ECSRegistry& ecs_registry )
	: mRenderer(renderer)
	, mECSRegistry(ecs_registry)
{
	// Create global descriptor set and uniform buffer
	mGlobalDescriptorSetId = CreateDynamicDescriptorSet( mRenderer.GetBuiltInDescriptorSetLayouts().global.get() );
	mGlobalUniformBufferId = CreateDynamicUniformBuffer( sizeof( eage::graphics::SceneGlobalData ) );
	GetDescriptorSet( mGlobalDescriptorSetId )->WriteBuffer(
		GLOBAL_SCENE_DATA_BINDING,
		vk::DescriptorType::eUniformBufferDynamic,
		GetGlobalUniformBuffer()->buffer,
		sizeof( eage::graphics::SceneGlobalData ) );
}

RenderSystem::~RenderSystem()
{
}

ResourceId
RenderSystem::CreateMeshBuffer( const std::vector<uint32_t>& indices, const std::vector<eage::graphics::Vertex>& vertices, 
								uint32_t first_index, uint32_t index_count, uint32_t vertex_offset )
{
	auto mesh = mRenderer.UploadMesh( indices, vertices );
	mesh->first_index = first_index;
	mesh->index_count = index_count;
	mesh->vertex_offset = vertex_offset;

	return mMeshBuffers.Store( std::move(mesh) );
}

ResourceId
RenderSystem::CreateMaterial( const eage::graphics::MaterialProperty& material_property )
{
	// Create descriptor layout for material-specific bindings
	eage::graphics::DescriptorLayoutBuilder layout_builder;
	
	for( const auto& texture : material_property.textures )
	{
		layout_builder.AddBinding(texture.binding, vk::DescriptorType::eCombinedImageSampler);
	}
	
	for( const auto& uniform : material_property.uniforms )
	{
		layout_builder.AddBinding(uniform.binding, uniform.type);
	}
	
	auto material_layout = layout_builder.Build( mRenderer.GetDevice(),
		vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
	
	// Get global descriptor layouts
	std::vector<vk::DescriptorSetLayout> global_layouts = {
		// Add your global layouts here (scene data, render component data, etc.)
		mRenderer.GetBuiltInDescriptorSetLayouts().global.get(),
		mRenderer.GetBuiltInDescriptorSetLayouts().per_object.get()
	};
	
	// Add material layout to the global layouts
	std::vector<vk::DescriptorSetLayout> all_layouts = global_layouts;
	all_layouts.push_back(material_layout.get());
	
	// Create or get cached pipeline
	auto pipeline = CreateOrGetPipeline(material_property, all_layouts);
	
	// Create material instance
	auto material = std::make_unique<eage::graphics::Material>();
	material->pipeline = pipeline;
	material->descriptor = std::make_unique<eage::graphics::StaticDescriptor>(
		mRenderer, material_layout.get());
	
	// Bind textures
	for( const auto& texture_binding : material_property.textures )
	{
		auto it = mImagePathToIdMap.find(texture_binding.texture_path);
		if( it == mImagePathToIdMap.end() )
		{
			LOG_ERROR() << "Failed to find image buffer for texture path: " << texture_binding.texture_path;
			continue;
		}

		auto texture = mImages.Get( it->second );
		if( !texture )
		{
			LOG_ERROR() << "Invalid image buffer for texture path: " << texture_binding.texture_path;
			continue;
		}
		
		// Get or create cached sampler
		ResourceId sampler_id = CreateSampler(
			texture_binding.min_filter, 
			texture_binding.mag_filter );
		
		vk::Sampler sampler = GetSampler(sampler_id);
		
		material->descriptor->WriteImage(
			texture_binding.binding,
			vk::DescriptorType::eCombinedImageSampler,
			texture->image_view.get(),
			vk::ImageLayout::eShaderReadOnlyOptimal,
			sampler);
	}
	
	// Bind uniform buffers
	for( const auto& uniform_binding : material_property.uniforms )
	{
		if( uniform_binding.data )
		{
			auto uniform_buffer = eage::graphics::ManagedBuffer::Create(
				*mRenderer.GetMemoryAllocator().allocator.get(),
				uniform_binding.size,
				vk::BufferUsageFlagBits::eUniformBuffer,
				VMA_MEMORY_USAGE_CPU_TO_GPU);
			
			uniform_buffer->Update( uniform_binding.data, uniform_binding.size, 0 );
			
			material->descriptor->WriteBuffer(
				uniform_binding.binding,
				uniform_binding.type,
				uniform_buffer->buffer,
				uniform_binding.size);
		}
	}
	
	return mMaterials.Store( std::move( material ) );
}

ResourceId
RenderSystem::CreateUniformBuffer( size_t data_size )
{
	auto buffer = eage::graphics::ManagedBuffer::Create(
		*mRenderer.GetMemoryAllocator().allocator.get(),
		data_size,
		vk::BufferUsageFlagBits::eUniformBuffer,
		VMA_MEMORY_USAGE_CPU_TO_GPU );

	return mUniformBuffers.Store( std::move(buffer) );
}

ResourceId
RenderSystem::CreateImageBuffer( const std::string& file_path )
{
	if( auto it = mImagePathToIdMap.find(file_path); it != mImagePathToIdMap.end() )
	{
		return it->second;
	}

	assets::ImageLoader image_loader;
		auto image = image_loader.LoadImage( file_path );

	// @todo: Expose image flags and format
	auto texture = mRenderer.UploadImage(
		image.data.data(), sizeof(unsigned char) * image.data.size(),
		image.width, image.height, vk::Format::eR8G8B8A8Srgb,
		vk::ImageUsageFlagBits::eSampled, vk::ImageAspectFlagBits::eColor, 1);

	mImagePathToIdMap[file_path] = mImages.Store( std::move( texture ) );
	return mImagePathToIdMap[file_path];
}

ResourceId
RenderSystem::CreateDynamicUniformBuffer( size_t data_size )
{
	size_t buffer_size = data_size * mRenderer.GetFrames().size();
	auto buffer = eage::graphics::ManagedBuffer::Create(
		*mRenderer.GetMemoryAllocator().allocator.get(),
		buffer_size,
		vk::BufferUsageFlagBits::eUniformBuffer,
		VMA_MEMORY_USAGE_CPU_TO_GPU );

	return mUniformBuffers.Store( std::move(buffer) );
}

ResourceId
RenderSystem::CreateDescriptorSet( vk::DescriptorSetLayout layout )
{
	auto descriptor = std::make_unique<eage::graphics::StaticDescriptor>( mRenderer, layout );
	return mDescriptorSets.Store( std::move(descriptor) );
}

ResourceId
RenderSystem::CreateDynamicDescriptorSet( vk::DescriptorSetLayout layout )
{
	auto descriptor = std::make_unique<eage::graphics::DynamicDescriptor>( mRenderer, layout );
	return mDescriptorSets.Store( std::move(descriptor) );
}

ResourceId
RenderSystem::CreateSampler( vk::Filter min_filter, vk::Filter mag_filter )
{
	// Create a hash from the filter parameters
	size_t hash = std::hash<int>{}(static_cast<int>(min_filter));
	hash ^= std::hash<int>{}(static_cast<int>(mag_filter)) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	
	// Check if sampler with these parameters already exists
	auto it = mSamplerCache.find(hash);
	if (it != mSamplerCache.end())
	{
		return it->second;
	}
	
	// Create new sampler
	auto sampler = mRenderer.CreateSampler(min_filter, mag_filter);
	auto sampler_ptr = std::make_unique<vk::UniqueSampler>(std::move(sampler));
	ResourceId sampler_id = mSamplers.Store(std::move(sampler_ptr));
	
	// Cache the sampler ID
	mSamplerCache[hash] = sampler_id;
	
	return sampler_id;
}

ResourceId
RenderSystem::CreateSpriteMesh( float width, float height, glm::vec2 uv_min, glm::vec2 uv_max )
{
	auto rect = eage::graphics::made_rect_vertices( { 0, 0, 0 }, width, height );
	rect.vertices[0].uv_x = uv_max.x; rect.vertices[0].uv_y = uv_min.y;
	rect.vertices[1].uv_x = uv_max.x; rect.vertices[1].uv_y = uv_max.y;
	rect.vertices[2].uv_x = uv_min.x; rect.vertices[2].uv_y = uv_min.y;
	rect.vertices[3].uv_x = uv_min.x; rect.vertices[3].uv_y = uv_max.y;
	return CreateMeshBuffer( rect.indices, rect.vertices, 0, 6, 0 );
}

void
RenderSystem::AttachRenderable( eage::ecs::Entity entity, ResourceId mesh_id, ResourceId material_id, bool visible )
{
	auto ubo_id = CreateDynamicUniformBuffer( sizeof( eage::graphics::MeshUniformData ) );
	auto descriptor_id = CreateDynamicDescriptorSet( mRenderer.GetBuiltInDescriptorSetLayouts().per_object.get() );
	GetDescriptorSet( descriptor_id )->WriteBuffer(
		PER_OBJECT_MESH_DATA_BINDING,
		vk::DescriptorType::eUniformBufferDynamic,
		GetUniformBuffer( ubo_id )->buffer,
		sizeof( eage::graphics::MeshUniformData ) );
	mECSRegistry.AddComponent( entity, eage::ecs::RenderComponent{
		mesh_id, material_id, ubo_id, descriptor_id, visible } );
}

void
RenderSystem::AttachSprite( eage::ecs::Entity entity, ResourceId material_id, float width, float height, glm::vec2 uv_min, glm::vec2 uv_max, bool visible )
{
	auto mesh_id = CreateSpriteMesh( width, height, uv_min, uv_max );
	AttachRenderable( entity, mesh_id, material_id, visible );
}

eage::graphics::ManagedBuffer*
RenderSystem::GetGlobalUniformBuffer()
{
	return mUniformBuffers.Get( mGlobalUniformBufferId );
}

eage::graphics::ManagedImage*
RenderSystem::GetImageBuffer( ResourceId id )
{
	return mImages.Get(id);
}

eage::graphics::ManagedBuffer*
RenderSystem::GetUniformBuffer( ResourceId id )
{
	return mUniformBuffers.Get(id);
}

eage::graphics::AbstractUniformDescriptor*
RenderSystem::GetDescriptorSet( ResourceId id )
{
	return mDescriptorSets.Get(id);
}

vk::Sampler
RenderSystem::GetSampler( ResourceId id )
{
	auto sampler_ptr = mSamplers.Get(id);
	return sampler_ptr ? sampler_ptr->get() : vk::Sampler{};
}

void RenderSystem::Update()
{
	auto& renderable_entities = mECSRegistry.GetComponentMap<RenderComponent>();
	for( const auto& [entity, render_cmp] : renderable_entities )
	{
		if( !render_cmp.visible )
		{
			continue;
		}

		eage::graphics::RenderInfo render_info{};

		// Material
		if( auto material = mMaterials.Get( render_cmp.material_id ) )
		{
			render_info.material = material;
		}

		// Mesh buffer
		if( auto mesh_buffer = mMeshBuffers.Get( render_cmp.mesh_buffer_id ) )
		{
			render_info.mesh_buffer = mesh_buffer;
			render_info.first_index   = mesh_buffer->first_index;
			render_info.index_count   = mesh_buffer->index_count;
			render_info.vertex_offset = mesh_buffer->vertex_offset;
		}
		else
		{
			LOG_ERROR( "Mesh buffer not found for entity: " + std::to_string(entity) );
			continue; // Skip rendering this entity if mesh buffer is not found
		}

		// Mesh descriptor
		if( auto mesh_descriptor = mDescriptorSets.Get( render_cmp.mesh_descriptor_id ) )
		{
			render_info.mesh_descriptor = mesh_descriptor;
		}
		else
		{
			LOG_ERROR( "Mesh descriptor not found for entity: " + std::to_string(entity) );
			continue; // Skip rendering this entity if mesh descriptor is not found
		}

		// Mesh uniform data
		if( auto mesh_uniform_data = mUniformBuffers.Get( render_cmp.mesh_uniform_data_dynamic_id ) )
		{
			render_info.mesh_uniform_data_dynamic = mesh_uniform_data;
			// Note: Descriptor binding should be set up once during entity creation, not here
		}
		else
		{
			LOG_ERROR( "Mesh uniform data not found for entity: " + std::to_string(entity) );
			continue; // Skip rendering this entity if mesh uniform data is not found
		}

		// Model matrix
		if( mECSRegistry.HasComponent<TransformComponent>(entity) )
		{
			auto& transform = mECSRegistry.GetComponent<TransformComponent>(entity);
			render_info.model_matrix = transform.GetWorldMatrix();
		}
		else
		{
			render_info.model_matrix = glm::mat4( 1.0f ); // Identity matrix if no transform
		}

		mRenderer.AddToRenderQueue( std::move( render_info ) );
	}
}

std::shared_ptr<eage::graphics::RenderPipeline> 
RenderSystem::CreateOrGetPipeline( const eage::graphics::MaterialProperty& property,
								   const std::vector<vk::DescriptorSetLayout>& global_layouts)
{
	size_t hash = HashMaterialProperty(property);
	
	auto it = mPipelineCache.find(hash);
	if (it != mPipelineCache.end())
	{
		return it->second;
	}
	
	// Create new pipeline
	auto pipeline = std::make_shared<eage::graphics::RenderPipeline>();
	
	// Load shaders
	auto vertex_shader = eage::graphics::create_shader_module_from_file(
		mRenderer.GetDevice(), property.vertex_shader_path);
	auto fragment_shader = eage::graphics::create_shader_module_from_file(
		mRenderer.GetDevice(), property.fragment_shader_path);
	
	if( !vertex_shader || !fragment_shader )
	{
		LOG_ERROR("Failed to load shaders for material");
		return nullptr;
	}
	
	// Create pipeline layout (global layouts + material layout)
	std::vector<vk::DescriptorSetLayout> all_layouts = global_layouts;
	
	vk::PipelineLayoutCreateInfo layout_info;
	layout_info.setLayoutCount = static_cast<uint32_t>(all_layouts.size());
	layout_info.pSetLayouts = all_layouts.data();
	layout_info.pushConstantRangeCount = 0;
	layout_info.pPushConstantRanges = nullptr;
	
	auto pipeline_layout = mRenderer.GetDevice().createPipelineLayoutUnique(layout_info);
	
	eage::graphics::VulkanPipelineBuilder pipeline_builder;
	pipeline->pipeline = pipeline_builder
		.SetShaders(vertex_shader->get(), fragment_shader->get())
		.SetPipelineLayout(pipeline_layout.get())
		.SetInputTopology(property.topology)
		.SetPolygonMode(property.polygon_mode)
		.SetCullMode(property.cull_mode, property.front_face)
		.SetMultisampling()
		.SetBlendMode(
			vk::Bool32(property.blend_enable),
			property.src_color_blend,
			property.dst_color_blend,
			property.color_blend_op)
		.SetColorAttachmentFormat(mRenderer.GetColorFormat())
		.SetDepthTest(
			vk::Bool32(property.depth_test),
			vk::Bool32(property.depth_write),
			property.depth_compare)
		.SetDepthFormat(mRenderer.GetDepthFormat())
		.Build(mRenderer.GetDevice());
	
	// Store the pipeline layout in the RenderPipeline
	pipeline->layout = std::move(pipeline_layout);
	pipeline->global_descriptor = GetDescriptorSet( mGlobalDescriptorSetId );
	
	mPipelineCache[hash] = pipeline;
	return pipeline;
}

size_t
RenderSystem::HashMaterialProperty( const eage::graphics::MaterialProperty& property )
{
	// Simple hash combining shader paths and pipeline state
	size_t hash = std::hash<std::string>{}(property.vertex_shader_path);
	hash ^= std::hash<std::string>{}(property.fragment_shader_path) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	hash ^= static_cast<size_t>(property.topology) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	hash ^= static_cast<size_t>(property.polygon_mode) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	// Add more fields as needed
	return hash;
}
