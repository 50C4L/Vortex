#ifndef _EAGE_RENDER_COMPONENTS_H_
#define _EAGE_RENDER_COMPONENTS_H_

#include <ecs/ResourceManager.h>

namespace eage::ecs
{
	struct RenderComponent
	{
		ResourceId mesh_buffer_id = INVALID_ID;
		ResourceId material_id = INVALID_ID;
		ResourceId mesh_uniform_data_dynamic_id = INVALID_ID;
		ResourceId mesh_descriptor_id = INVALID_ID;
		
		bool visible = true;
	};
}

#endif // _ECS_RENDER_COMPONENTS_H_