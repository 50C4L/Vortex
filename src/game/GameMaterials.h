#ifndef _VORTEX_GAME_MATERIALS_H
#define _VORTEX_GAME_MATERIALS_H

#include <vulkan/vulkan.hpp>

#include <memory>

#include <graphics/ManagedVulkanResources.h>

namespace eage::graphics
{
	struct RenderPipeline;
	class Renderer;
	struct Material;
}

namespace vortex
{
	struct SingleTextureSpriteMaterial
	{
		std::shared_ptr<eage::graphics::RenderPipeline> pipeline;
		vk::UniqueDescriptorSetLayout material_layout;

		void build_pipeline( eage::graphics::Renderer& renderer, const std::vector<vk::DescriptorSetLayout>& descriptor_set_layouts );

		struct Resources
		{
			eage::graphics::ManagedImage::Ptr color_texture;
			vk::UniqueSampler color_texture_sampler;
		};
		std::unique_ptr<eage::graphics::Material> Instantiate( eage::graphics::Renderer& renderer, const Resources& resources );
	};
}

#endif // _VORTEX_GAME_MATERIALS_H