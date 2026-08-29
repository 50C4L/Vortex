#ifndef _EAGE_SYSTEMS_SCENE_GRAPH_SYSTEM_H_
#define _EAGE_SYSTEMS_SCENE_GRAPH_SYSTEM_H_

#include <cstdint>
#include <vector>
#include <glm/glm.hpp>

#include <ecs/ECS.h>

namespace eage::ecs
{
	///
	/// SceneGraphSystem: Updates world transforms based on parent-child relationships
	///
	class SceneGraphSystem : public ECSRegistry::Observer
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

		///
		/// Set the local enabled flag. Disabled nodes and their descendants
		/// have world_enabled = false and are skipped by transform Update().
		///
		void SetNodeEnabled( Entity entity, bool enabled );

		///
		/// Queue destroy for entity and all descendants (depth-first collect, then queue).
		///
		void QueueDestroySubtree( Entity entity );

		void Update();

		// ECSRegistry::Observer
		void OnEntityDestroying( Entity entity ) override;

	private:
		void UpdateChildrenRecursive( Entity entity, const glm::mat4& parent_world_matrix, bool parent_world_enabled );
		void RefreshWorldEnabled( Entity entity, bool parent_world_enabled );
		void CollectSubtree( Entity entity, std::vector<Entity>& out ) const;

		ECSRegistry& mECSRegistry;
		Entity mSceneRootEntity = 0;
	};
}

#endif // _EAGE_SYSTEMS_SCENE_GRAPH_SYSTEM_H_
