#ifndef _EAGE_RENDER_COMPONENTS_H_
#define _EAGE_RENDER_COMPONENTS_H_

#include <memory>
#include <vulkan/vulkan.hpp>
#include <graphics/RenderInfo.h>
#include <ecs/ResourceManager.h>

namespace eage::ecs
{
	struct RenderComponent
	{
		ResourceID mesh_buffer_id = INVALID_ID;
		ResourceID material_id = INVALID_ID;
		ResourceID mesh_uniform_data_dynamic_id = INVALID_ID;
		ResourceID mesh_descriptor_id = INVALID_ID;
		
		bool visible = true;
	};
}

#endif // _ECS_RENDER_COMPONENTS_H_