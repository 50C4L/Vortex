#include "SceneGraphSystem.h"

#include <ecs/components/Basics.h>

using namespace eage::ecs;

SceneGraphSystem::SceneGraphSystem( ECSRegistry& ecs_registry )
	: mECSRegistry( ecs_registry )
{
	mECSRegistry.Subscribe( this );
}

SceneGraphSystem::~SceneGraphSystem()
{
	mECSRegistry.Unsubscribe( this );
}

void
SceneGraphSystem::SetSceneRoot( Entity entity )
{
	mSceneRootEntity = entity;
}

void
SceneGraphSystem::AddNodeToParent( Entity entity, Entity parent )
{
	if( entity == 0 || parent == 0 || entity == parent )
	{
		return;
	}

	if( !mECSRegistry.HasComponent<SceneGraphComponent>( entity ) )
	{
		mECSRegistry.AddComponent( entity, SceneGraphComponent{} );
	}

	if( !mECSRegistry.HasComponent<SceneGraphComponent>( parent ) )
	{
		mECSRegistry.AddComponent( parent, SceneGraphComponent{} );
	}

	auto& node = mECSRegistry.GetComponent<SceneGraphComponent>( entity );
	if( node.parent_entity == parent )
	{
		return;
	}

	if( node.parent_entity != 0 )
	{
		RemoveNodeFromParent( entity );
	}

	node.parent_entity = parent;
	auto& parent_node = mECSRegistry.GetComponent<SceneGraphComponent>( parent );
	parent_node.children_entities.push_back( entity );
}

void
SceneGraphSystem::RemoveNodeFromParent( Entity entity )
{
	if( !mECSRegistry.HasComponent<SceneGraphComponent>( entity ) )
	{
		return;
	}

	auto& node = mECSRegistry.GetComponent<SceneGraphComponent>( entity );
	if( node.parent_entity == 0 )
	{
		return;
	}

	const Entity parent = node.parent_entity;
	if( mECSRegistry.HasComponent<SceneGraphComponent>( parent ) )
	{
		auto& children = mECSRegistry.GetComponent<SceneGraphComponent>( parent ).children_entities;
		for( size_t i = 0; i < children.size(); ++i )
		{
			if( children[i] == entity )
			{
				children[i] = children.back();
				children.pop_back();
				break;
			}
		}
	}

	node.parent_entity = 0;
}

void
SceneGraphSystem::CollectSubtree( Entity entity, std::vector<Entity>& out ) const
{
	out.push_back( entity );

	if( !mECSRegistry.HasComponent<SceneGraphComponent>( entity ) )
	{
		return;
	}

	const auto& relationship = mECSRegistry.GetComponent<SceneGraphComponent>( entity );
	for( Entity child : relationship.children_entities )
	{
		CollectSubtree( child, out );
	}
}

void
SceneGraphSystem::QueueDestroySubtree( Entity entity )
{
	if( entity == 0 )
	{
		return;
	}

	std::vector<Entity> nodes;
	CollectSubtree( entity, nodes );
	for( Entity node : nodes )
	{
		mECSRegistry.QueueDestroyEntity( node );
	}
}

void
SceneGraphSystem::OnEntityDestroying( Entity entity )
{
	RemoveNodeFromParent( entity );
}

void
SceneGraphSystem::Update()
{
	if( mSceneRootEntity == 0 )
	{
		return; // No scene root set
	}

	auto& root_relationships = mECSRegistry.GetComponent<SceneGraphComponent>( mSceneRootEntity );
	for( auto& child : root_relationships.children_entities )
	{
		UpdateChildrenRecursive( child, glm::mat4(1.0f) ); // Identity matrix as parent for root's children
	}
}

void
SceneGraphSystem::UpdateChildrenRecursive( Entity entity, const glm::mat4& parent_world_matrix )
{
	if( !mECSRegistry.HasComponent<TransformComponent>( entity ) )
	{
		return; // Entity must have a Transform component
	}

	auto& transform = mECSRegistry.GetComponent<TransformComponent>( entity );

	// Update world matrix
	glm::mat4 local_matrix = transform.GetLocalMatrix();
	glm::mat4 world_matrix = parent_world_matrix * local_matrix;
	transform.SetWorldMatrix( world_matrix );

	if( !mECSRegistry.HasComponent<SceneGraphComponent>( entity ) )
	{
		return; // No children to update
	}
	auto& relationship = mECSRegistry.GetComponent<SceneGraphComponent>( entity );

	// Recursively update children
	for( auto& child : relationship.children_entities )
	{
		if( mECSRegistry.HasComponent<TransformComponent>( child ) )
		{
			UpdateChildrenRecursive( child, world_matrix );
		}
	}
}
