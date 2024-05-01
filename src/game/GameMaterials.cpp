#include "GameMaterials.h"

#include <graphics/Renderer.h>
#include <graphics/VulkanPipeline.h>
#include <graphics/VulkanDescriptor.h>
#include <graphics/VulkanShader.h>
#include <graphics/Material.h>

#include <utility/Logger.h>

using namespace vortex;
using namespace utility;

void 
SingleTextureSpriteMaterial::build_pipeline( graphics::Renderer& renderer, const std::vector<vk::DescriptorSetLayout>& descriptor_set_layouts )
{
	this->pipeline = std::make_shared<graphics::RenderPipeline>();

	std::vector<vk::DescriptorSetLayout> temp_layouts = descriptor_set_layouts;
	temp_layouts.push_back( this->material_layout.get() );
	vk::PipelineLayoutCreateInfo pipeline_layout_info{};
	pipeline_layout_info.setLayoutCount = static_cast<uint32_t>( temp_layouts.size() );
	pipeline_layout_info.pSetLayouts = temp_layouts.data();
	this->pipeline->layout = renderer.GetDevice().createPipelineLayoutUnique( pipeline_layout_info );

	auto vertex_shader = graphics::create_shader_module_from_file( renderer.GetDevice(), "./src/shaders/compiled/colored_triangle_mesh.vert.spv" );
	if( !vertex_shader.has_value() )
	{
		LOG_ERROR( "Failed to create vertex shader module." );
		return;
	}

	auto fragment_shader = graphics::create_shader_module_from_file( renderer.GetDevice(), "./src/shaders/compiled/colored_triangle.frag.spv" );
	if( !fragment_shader.has_value() )
	{
		LOG_ERROR( "Failed to create fragment shader module." );
		return;
	}

	graphics::VulkanPipelineBuilder pipeline_builder;
	this->pipeline->pipeline = pipeline_builder
		.SetPipelineLayout( this->pipeline->layout.get() )
		.SetShaders( vertex_shader.value().get(), fragment_shader.value().get() )
		.SetInputTopology( vk::PrimitiveTopology::eTriangleList )
		.SetPolygonMode( vk::PolygonMode::eFill )
		.SetCullMode( vk::CullModeFlagBits::eNone, vk::FrontFace::eClockwise )
		.SetMultisampling()
		.SetBlendMode( vk::Bool32( VK_TRUE ), vk::BlendFactor::eOne, vk::BlendFactor::eDstAlpha, vk::BlendOp::eAdd )
		.SetColorAttachmentFormat( renderer.GetColorFormat() )
		.SetDepthTest( vk::Bool32( VK_TRUE ), vk::Bool32( VK_TRUE ), vk::CompareOp::eGreaterOrEqual )
		.SetDepthFormat( renderer.GetDepthFormat() )
		.Build( renderer.GetDevice() );
}

std::unique_ptr<graphics::Material>
SingleTextureSpriteMaterial::Instantiate( graphics::Renderer& renderer, const Resources& resources )
{
	auto material = std::make_unique<graphics::Material>();

	material->pipeline = this->pipeline;
	material->descriptor = std::make_unique<graphics::UniformDescriptor>( renderer, this->material_layout.get() );
	material->descriptor->WriteImage(
		0, 
		vk::DescriptorType::eCombinedImageSampler,
		resources.color_texture->image_view.get(),
		vk::ImageLayout::eShaderReadOnlyOptimal,
		resources.color_texture_sampler.get() );

	return material;
}