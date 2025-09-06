#ifndef _EAGE_ECS_COMPONENTS_PHYSICS_H_
#define _EAGE_ECS_COMPONENTS_PHYSICS_H_

#include <ecs/ResourceManager.h>
#include <glm/glm.hpp>

namespace eage::ecs
{
	struct CollisionComponent 
	{
		eage::ecs::ResourceId body_id = 0;
		
		// Pure configuration data
		bool is_sensor = false;
		bool is_static = false;
		uint16_t category_bits = 0x0001;
		uint16_t mask_bits = 0xFFFF;
		int16_t group_index = 0;
	};

	struct BoxColliderComponent 
	{
		float width = 1.0f;
		float height = 1.0f;
		glm::vec2 offset{ 0.0f, 0.0f };
	};

	struct CircleColliderComponent 
	{
		float radius = 0.5f;
		glm::vec2 offset{ 0.0f, 0.0f };
	};
}

#endif // _EAGE_ECS_COMPONENTS_PHYSICS_H_