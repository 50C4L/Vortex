#ifndef _VULKAN_PIPELINE_H_
#define _VULKAN_PIPELINE_H_

#include <vulkan/vulkan.hpp>

#include <vector>
#include <memory>

namespace eage::graphics
{
	class AbstractUniformDescriptor;

	class VulkanPipelineBuilder
	{
	public:
		vk::UniquePipeline Build( vk::Device device );

		void Clear();

		VulkanPipelineBuilder& SetShaders( vk::ShaderModule& vertex_shader, vk::ShaderModule& fragment_shader );
		VulkanPipelineBuilder& SetInputTopology( vk::PrimitiveTopology topology );
		VulkanPipelineBuilder& SetPolygonMode( vk::PolygonMode mode );
		VulkanPipelineBuilder& SetCullMode( vk::CullModeFlags mode, vk::FrontFace front_face );
		VulkanPipelineBuilder& SetColorAttachmentFormat( vk::Format format );
		VulkanPipelineBuilder& SetDepthFormat( vk::Format format );
		VulkanPipelineBuilder& SetPipelineLayout( vk::PipelineLayout layout );

		// The following features are disabled for now
		VulkanPipelineBuilder& SetMultisampling( vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1 );
		VulkanPipelineBuilder& SetBlendMode( vk::Bool32 enable, vk::BlendFactor src_factor, vk::BlendFactor dst_factor, vk::BlendOp blend_op );
		VulkanPipelineBuilder& SetDepthTest( vk::Bool32 enable, vk::Bool32 write_enable, vk::CompareOp compare_op );
	
		vk::PipelineInputAssemblyStateCreateInfo input_assembly;
		vk::PipelineRasterizationStateCreateInfo rasterizer;
		vk::PipelineColorBlendAttachmentState color_blend_attachement;
		vk::PipelineMultisampleStateCreateInfo multisampling;
		vk::PipelineLayout pipeline_layout;
		vk::PipelineDepthStencilStateCreateInfo depth_stencil;
		vk::PipelineRenderingCreateInfo rendering_info;
		vk::Format color_attachment_format;

		std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;
	};

	struct RenderPipeline
	{
		vk::UniquePipeline pipeline;
		vk::UniquePipelineLayout layout;
		AbstractUniformDescriptor* global_descriptor;
	};
}

#endif // _VULKAN_PIPELINE_H_