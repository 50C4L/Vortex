#include "SceneGraphSystem.h"

#include <ecs/ECS.h>
#include <ecs/components/Basics.h>

using namespace eage::ecs;

SceneGraphSystem::SceneGraphSystem( ECSRegistry& ecs_registry )
	: mECSRegistry( ecs_registry )
{
}

SceneGraphSystem::~SceneGraphSystem()
{
}

void
SceneGraphSystem::SetSceneRoot( uint64_t entity )
{
	// @todo SceneGraphSystem should create the root entity itself, and provide a getter
	mSceneRootEntity = entity;
}

void
SceneGraphSystem::Update()
{
	if( mSceneRootEntity == 0 )
	{
		return; // No scene root set
	}

	auto& root_relationships = mECSRegistry.GetComponent<SceneGraphComponment>( mSceneRootEntity );
	for( auto& child : root_relationships.children_entities )
	{
		UpdateChildrenRecursive( child, glm::mat4(1.0f) ); // Identity matrix as parent for root's children
	}
}

void
SceneGraphSystem::UpdateChildrenRecursive( uint64_t entity, const glm::mat4& parent_world_matrix )
{
	if( !mECSRegistry.HasComponent<TransformComponent>( entity )  )
	{
		return; // Entity must have both Transform and Relationship components
	}

	auto& transform = mECSRegistry.GetComponent<TransformComponent>( entity );
	
	// Update world matrix
	glm::mat4 local_matrix = transform.GetLocalMatrix();
	glm::mat4 world_matrix = parent_world_matrix * local_matrix;
	transform.SetWorldMatrix( world_matrix );

	if( !mECSRegistry.HasComponent<SceneGraphComponment>( entity ) )
	{
		return; // No children to update
	}
	auto& relationship = mECSRegistry.GetComponent<SceneGraphComponment>( entity );

	// Recursively update children
	for( auto& child : relationship.children_entities )
	{
		if( mECSRegistry.HasComponent<TransformComponent>( entity ) )
		{
			UpdateChildrenRecursive( child, world_matrix );
		}
	}
}