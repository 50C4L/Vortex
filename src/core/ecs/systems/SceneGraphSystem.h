#ifndef _EAGE_SYSTEMS_SCENE_GRAPH_SYSTEM_H_
#define _EAGE_SYSTEMS_SCENE_GRAPH_SYSTEM_H_

#include <cstdint>
#include <glm/glm.hpp>

namespace eage::ecs
{
	class ECSRegistry;

	class SceneGraphSystem
	{
	public:
		SceneGraphSystem( ECSRegistry& ecs_registry );
		~SceneGraphSystem();

		void SetSceneRoot( uint64_t entity );

		void Update();

	private:
		void UpdateChildrenRecursive( uint64_t entity, const glm::mat4& parent_world_matrix );

		ECSRegistry& mECSRegistry;
		uint64_t mSceneRootEntity = 0;
	};
}

#endif // _EAGE_SYSTEMS_SCENE_GRAPH_SYSTEM_H_