#ifndef _VORTEX_GAME_MATERIALS_H
#define _VORTEX_GAME_MATERIALS_H

#include <vulkan/vulkan.hpp>

#include <memory>

#include <graphics/ManagedVulkanResources.h>

namespace graphics
{
	struct RenderPipeline;
	class Renderer;
	struct Material;
}

namespace vortex
{
	struct SingleTextureSpriteMaterial
	{
		std::shared_ptr<graphics::RenderPipeline> pipeline;
		vk::UniqueDescriptorSetLayout material_layout;

		void build_pipeline( graphics::Renderer& renderer, const std::vector<vk::DescriptorSetLayout>& descriptor_set_layouts );

		struct Resources
		{
			graphics::ManagedImage::Ptr color_texture;
			vk::UniqueSampler color_texture_sampler;
		};
		std::unique_ptr<graphics::Material> Instantiate( graphics::Renderer& renderer, const Resources& resources );
	};
}

#endif // _VORTEX_GAME_MATERIALS_H