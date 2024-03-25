#include "VulkanPipeline.h"

#include <utility/Logger.h>

using namespace graphics;
using namespace utility;

namespace
{
	const uint32_t VIEWPORT_COUNT = 1;
	const uint32_t SCISSOR_COUNT = 1;
	const float LINE_WIDTH = 1.0f;
}

vk::UniquePipeline
VulkanPipelineBuilder::Build( vk::Device& device )
{
	vk::PipelineViewportStateCreateInfo viewport_state{};
	viewport_state.viewportCount = VIEWPORT_COUNT;
	viewport_state.scissorCount = SCISSOR_COUNT;

	// No blending for now
	vk::PipelineColorBlendStateCreateInfo color_blend_state{};
	color_blend_state.attachmentCount = 1;
	color_blend_state.pAttachments = &color_blend_attachement;
	color_blend_state.logicOpEnable = VK_FALSE;
	color_blend_state.logicOp = vk::LogicOp::eCopy;

	vk::PipelineVertexInputStateCreateInfo vertex_input_info{};

	vk::GraphicsPipelineCreateInfo pipeline_info{};
	pipeline_info.pNext = &rendering_info;
	pipeline_info.stageCount = static_cast<uint32_t>( shader_stages.size() );
	pipeline_info.pStages = shader_stages.data();
	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly;
	pipeline_info.pViewportState = &viewport_state;
	pipeline_info.pRasterizationState = &rasterizer;
	pipeline_info.pMultisampleState = &multisampling;
	pipeline_info.pColorBlendState = &color_blend_state;
	pipeline_info.pDepthStencilState = &depth_stencil;
	pipeline_info.layout = pipeline_layout;

	vk::DynamicState dynamic_states[] = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
	vk::PipelineDynamicStateCreateInfo dynamic_state{};
	dynamic_state.dynamicStateCount = 2;
	dynamic_state.pDynamicStates = dynamic_states;
	pipeline_info.pDynamicState = &dynamic_state;

	auto result = device.createGraphicsPipelineUnique( nullptr, pipeline_info );
	if( result.result != vk::Result::eSuccess )
	{
		LOG_ERROR( "Failed to create graphics pipeline" );
		return vk::UniquePipeline{};
	}

	return std::move( result.value );
}

void
VulkanPipelineBuilder::Clear()
{
	shader_stages.clear();
	input_assembly = vk::PipelineInputAssemblyStateCreateInfo{};
	rasterizer = vk::PipelineRasterizationStateCreateInfo{};
	color_blend_attachement = vk::PipelineColorBlendAttachmentState{};
	multisampling = vk::PipelineMultisampleStateCreateInfo{};
	pipeline_layout = vk::PipelineLayout{};
	rendering_info = vk::PipelineRenderingCreateInfo{};
	color_attachment_format = vk::Format::eUndefined;
}

VulkanPipelineBuilder&
VulkanPipelineBuilder::SetShaders( vk::ShaderModule& vertex_shader, vk::ShaderModule& fragment_shader )
{
	vk::PipelineShaderStageCreateInfo vertex_stage{};
	vertex_stage.stage = vk::ShaderStageFlagBits::eVertex;
	vertex_stage.module = vertex_shader;
	vertex_stage.pName = "main";

	vk::PipelineShaderStageCreateInfo fragment_stage{};
	fragment_stage.stage = vk::ShaderStageFlagBits::eFragment;
	fragment_stage.module = fragment_shader;
	fragment_stage.pName = "main";

	shader_stages.push_back( vertex_stage );
	shader_stages.push_back( fragment_stage );

	return *this;
}

VulkanPipelineBuilder&
VulkanPipelineBuilder::SetInputTopology( vk::PrimitiveTopology topology )
{
	input_assembly.topology = topology;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	return *this;
}

VulkanPipelineBuilder&
VulkanPipelineBuilder::SetPolygonMode( vk::PolygonMode mode )
{
	rasterizer.polygonMode = mode;
	rasterizer.lineWidth = LINE_WIDTH;

	return *this;
}

VulkanPipelineBuilder&
VulkanPipelineBuilder::SetCullMode( vk::CullModeFlags mode, vk::FrontFace front_face )
{
	rasterizer.cullMode = mode;
	rasterizer.frontFace = front_face;

	return *this;
}

VulkanPipelineBuilder&
VulkanPipelineBuilder::SetMultisampling( vk::SampleCountFlagBits sample_count )
{
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = sample_count;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	return *this;
}

VulkanPipelineBuilder&
VulkanPipelineBuilder::SetBlendMode( /*vk::Bool32 enable, vk::BlendFactor src_factor, vk::BlendFactor dst_factor, vk::BlendOp blend_op*/ )
{
	color_blend_attachement.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	color_blend_attachement.blendEnable = VK_FALSE;
	//color_blend_attachement.srcColorBlendFactor = src_factor;
	//color_blend_attachement.dstColorBlendFactor = dst_factor;
	//color_blend_attachement.colorBlendOp = blend_op;
	//color_blend_attachement.srcAlphaBlendFactor = src_factor;
	//color_blend_attachement.dstAlphaBlendFactor = dst_factor;
	//color_blend_attachement.alphaBlendOp = blend_op;

	return *this;
}

VulkanPipelineBuilder&
VulkanPipelineBuilder::SetColorAttachmentFormat( vk::Format format )
{
	color_attachment_format = format;
	rendering_info.colorAttachmentCount = 1;
	rendering_info.pColorAttachmentFormats = &color_attachment_format;

	return *this;
}

VulkanPipelineBuilder&
VulkanPipelineBuilder::SetDepthFormat( vk::Format format )
{
	rendering_info.depthAttachmentFormat = format;

	return *this;
}

VulkanPipelineBuilder&
VulkanPipelineBuilder::SetDepthTest( /*vk::Bool32 enable, vk::Bool32 write_enable, vk::CompareOp compare_op*/ )
{
	depth_stencil.depthTestEnable = VK_TRUE;
	depth_stencil.depthWriteEnable = VK_TRUE;
	depth_stencil.depthCompareOp = vk::CompareOp::eNever;
	depth_stencil.depthBoundsTestEnable = VK_FALSE;
	depth_stencil.stencilTestEnable = VK_FALSE;
	depth_stencil.minDepthBounds = 0.0f;
	depth_stencil.maxDepthBounds = 1.0f;

	return *this;
}

VulkanPipelineBuilder&
VulkanPipelineBuilder::SetPipelineLayout( vk::PipelineLayout layout )
{
	pipeline_layout = std::move( layout );

	return *this;
}