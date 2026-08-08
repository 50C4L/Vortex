#ifndef _EAGE_SYSTEMS_SCENE_GRAPH_SYSTEM_H_
#define _EAGE_SYSTEMS_SCENE_GRAPH_SYSTEM_H_

#include <cstdint>
#include <glm/glm.hpp>

#include <ecs/ECS.h>

namespace eage::ecs
{
	///
	/// SceneGraphSystem: Updates world transforms based on parent-child relationships
	///
	class SceneGraphSystem
	{
	public:
		SceneGraphSystem( ECSRegistry& ecs_registry );
		~SceneGraphSystem();

		///
		/// Set the root entity of the scene graph
		///
		void SetSceneRoot( Entity entity );

		///
		/// Link entity under parent. Creates SceneGraphComponent on either side if missing.
		/// Reparents if entity already has a different parent. No-op if already under parent.
		///
		void AddNodeToParent( Entity entity, Entity parent );

		///
		/// Unlink entity from its parent. Clears parent_entity; does not touch children.
		/// Nodes detached from the root tree are simply skipped by Update().
		///
		void RemoveNodeFromParent( Entity entity );

		void Update();

	private:
		void UpdateChildrenRecursive( Entity entity, const glm::mat4& parent_world_matrix );

		ECSRegistry& mECSRegistry;
		Entity mSceneRootEntity = 0;
	};
}

#endif // _EAGE_SYSTEMS_SCENE_GRAPH_SYSTEM_H_
