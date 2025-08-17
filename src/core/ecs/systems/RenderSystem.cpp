#include "RenderSystem.h"

#include <assets/ImageLoader.h>
#include <ecs/components/Basics.h>
#include <ecs/components/Render.h>
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

RenderSystem::RenderSystem( eage::graphics::Renderer& renderer, ECSRegistry& ecs_registry )
	: mRenderer(renderer)
	, mECSRegistry(ecs_registry)
{
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
		// Load texture
		// @todo: Should texture loading be handled here or in a separate system?
		assets::ImageLoader image_loader;
		auto image = image_loader.LoadImage(texture_binding.texture_path);
		
		auto texture = mRenderer.UploadImage(
			image.data.data(), sizeof(unsigned char) * image.data.size(),
			image.width, image.height, vk::Format::eR8G8B8A8Srgb,
			vk::ImageUsageFlagBits::eSampled, vk::ImageAspectFlagBits::eColor, 1);
		
		auto sampler = mRenderer.CreateSampler(
			texture_binding.min_filter, 
			texture_binding.mag_filter );
		
		material->descriptor->WriteImage(
			texture_binding.binding,
			vk::DescriptorType::eCombinedImageSampler,
			texture->image_view.get(),
			vk::ImageLayout::eShaderReadOnlyOptimal,
			sampler.get());
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
RenderSystem::CreateUniformBuffer( size_t data_size, bool dynamic )
{
	size_t buffer_size = dynamic ? data_size * mRenderer.GetFrames().size() : data_size;
	auto buffer = eage::graphics::ManagedBuffer::Create(
		*mRenderer.GetMemoryAllocator().allocator.get(),
		buffer_size,
		vk::BufferUsageFlagBits::eUniformBuffer,
		VMA_MEMORY_USAGE_CPU_TO_GPU );

	return mUniformBuffers.Store(std::move(buffer));
}

ResourceId
RenderSystem::CreateDescriptorSet( vk::DescriptorSetLayout layout )
{
	auto descriptor = std::make_unique<eage::graphics::UniformDescriptor>( mRenderer, layout );
	return mDescriptorSets.Store( std::move(descriptor) );
}

eage::graphics::ManagedBuffer*
RenderSystem::GetUniformBuffer(ResourceId id)
{
	return mUniformBuffers.Get(id);
}


void RenderSystem::Update()
{
	// Update uniform buffers with transform data
	// This replaces the old RenderComponent::Transform logic
}

void RenderSystem::Render()
{
	// Iterate through all entities with both Transform and Render components
	// This replaces the old RenderComponent::CreateRenderInfo logic
	
	// For now, you'd need to iterate manually or implement a query system
	// Example pseudo-code:
	/*
	for (auto entity : entities_with_render_and_transform_components)
	{
		auto& transform = mECSRegistry.GetComponent<TransformComponent>(entity);
		auto& render = mECSRegistry.GetComponent<RenderComponent>(entity);
		
		if (render.visible)
		{
			// Update uniforms with transform.ToMatrix()
			// Create RenderInfo and add to render queue
			mRenderer.AddToRenderQueue(CreateRenderInfo(entity));
		}
	}
	*/
}

eage::graphics::RenderInfo RenderSystem::CreateRenderInfo(Entity entity)
{
	auto& render = mECSRegistry.GetComponent<RenderComponent>(entity);
	auto& transform = mECSRegistry.GetComponent<TransformComponent>(entity);
	
	// Update uniform buffer with transform matrix
	// Return RenderInfo similar to old implementation
	
	eage::graphics::RenderInfo info{};
	// Fill info with data from render component
	return info;
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
