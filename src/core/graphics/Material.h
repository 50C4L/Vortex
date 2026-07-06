#ifndef _MATERIAL_H
#define _MATERIAL_H

#include <memory>

#include <graphics/VulkanPipeline.h>

namespace eage::graphics
{
	// Represents a material instance
	struct Material
	{
		std::shared_ptr<RenderPipeline> pipeline;		// Pipeline can be shared between instances
	};
}

#endif // _MATERIAL_H