#ifndef _EAGE_ECS_COMPONENTS_PHYSICS_H_
#define _EAGE_ECS_COMPONENTS_PHYSICS_H_

#include <ecs/ResourceManager.h>
#include <glm/glm.hpp>

namespace eage::ecs
{
	struct PhysicsComponent 
	{
		eage::ecs::ResourceId body_id = 0;
		
		enum class BodyType
		{
			STATIC,
			DYNAMIC,
			KINEMATIC
		} body_type = BodyType::STATIC;

		bool is_sensor = false;

		// If true, the TransformComponent will be used to update the physics body position
		// This is useful for static bodies that need to be moved infrequently
		// For dynamic bodies, setting this to true may lead to unexpected behavior
		bool sync_transform_to_body = false;

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