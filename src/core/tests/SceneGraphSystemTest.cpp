#include <doctest/doctest.h>

#include <ecs/ECS.h>
#include <ecs/components/Basics.h>
#include <ecs/systems/SceneGraphSystem.h>

namespace
{
	eage::ecs::Entity make_node( eage::ecs::ECSRegistry& registry, const glm::vec3& position = glm::vec3( 0.f ) )
	{
		const eage::ecs::Entity entity = registry.CreateEntity();
		eage::ecs::TransformComponent transform;
		transform.SetPosition( position );
		transform.SetRotation( glm::quat( 1.f, 0.f, 0.f, 0.f ) );
		transform.SetScale( glm::vec3( 1.f ) );
		registry.AddComponent( entity, std::move( transform ) );
		return entity;
	}

	bool has_child( const eage::ecs::SceneGraphComponent& node, eage::ecs::Entity child )
	{
		for( eage::ecs::Entity candidate : node.children_entities )
		{
			if( candidate == child )
			{
				return true;
			}
		}
		return false;
	}

	glm::vec3 world_position( eage::ecs::ECSRegistry& registry, eage::ecs::Entity entity )
	{
		const glm::mat4 world = registry.GetComponent<eage::ecs::TransformComponent>( entity ).GetWorldMatrix();
		return glm::vec3( world[3] );
	}
}

TEST_CASE( "i_can_add_a_scene_hierachy" )
{
	// GIVEN the ECS registry, a scene graph system, and a root entity
	eage::ecs::ECSRegistry registry;
	eage::ecs::SceneGraphSystem scene_graph( registry );
	const eage::ecs::Entity root = make_node( registry );
	scene_graph.SetSceneRoot( root );

	// WHEN a parent, two children, and a grandchild are added under the root
	const eage::ecs::Entity parent = make_node( registry, glm::vec3( 10.f, 0.f, 0.f ) );
	const eage::ecs::Entity child_a = make_node( registry, glm::vec3( 5.f, 0.f, 0.f ) );
	const eage::ecs::Entity child_b = make_node( registry, glm::vec3( 0.f, 4.f, 0.f ) );
	const eage::ecs::Entity grandchild = make_node( registry, glm::vec3( 1.f, 0.f, 0.f ) );
	scene_graph.AddNodeToParent( parent, root );
	scene_graph.AddNodeToParent( child_a, parent );
	scene_graph.AddNodeToParent( child_b, parent );
	scene_graph.AddNodeToParent( grandchild, child_b );
	scene_graph.Update();

	// THEN each node is linked to its parent
	CHECK( registry.GetComponent<eage::ecs::SceneGraphComponent>( parent ).parent_entity == root );
	CHECK( registry.GetComponent<eage::ecs::SceneGraphComponent>( child_a ).parent_entity == parent );
	CHECK( registry.GetComponent<eage::ecs::SceneGraphComponent>( child_b ).parent_entity == parent );
	CHECK( registry.GetComponent<eage::ecs::SceneGraphComponent>( grandchild ).parent_entity == child_b );

	// AND each parent lists its children
	CHECK( has_child( registry.GetComponent<eage::ecs::SceneGraphComponent>( root ), parent ) );
	CHECK( has_child( registry.GetComponent<eage::ecs::SceneGraphComponent>( parent ), child_a ) );
	CHECK( has_child( registry.GetComponent<eage::ecs::SceneGraphComponent>( parent ), child_b ) );
	CHECK( has_child( registry.GetComponent<eage::ecs::SceneGraphComponent>( child_b ), grandchild ) );

	// AND world transforms compose down the hierarchy
	CHECK( world_position( registry, parent ).x == doctest::Approx( 10.f ) );
	CHECK( world_position( registry, child_a ).x == doctest::Approx( 15.f ) );
	CHECK( world_position( registry, child_b ).y == doctest::Approx( 4.f ) );
	CHECK( world_position( registry, grandchild ).x == doctest::Approx( 11.f ) );
	CHECK( world_position( registry, grandchild ).y == doctest::Approx( 4.f ) );
}

TEST_CASE( "disable_the_parent_node_in_the_scene_will_disable_its_decendents" )
{
	// GIVEN a scene with two branches under the root
	eage::ecs::ECSRegistry registry;
	eage::ecs::SceneGraphSystem scene_graph( registry );
	const eage::ecs::Entity root = make_node( registry );
	const eage::ecs::Entity parent = make_node( registry, glm::vec3( 10.f, 0.f, 0.f ) );
	const eage::ecs::Entity child = make_node( registry, glm::vec3( 5.f, 0.f, 0.f ) );
	const eage::ecs::Entity grandchild = make_node( registry, glm::vec3( 1.f, 0.f, 0.f ) );
	const eage::ecs::Entity other = make_node( registry, glm::vec3( 0.f, 8.f, 0.f ) );
	scene_graph.SetSceneRoot( root );
	scene_graph.AddNodeToParent( parent, root );
	scene_graph.AddNodeToParent( child, parent );
	scene_graph.AddNodeToParent( grandchild, child );
	scene_graph.AddNodeToParent( other, root );
	scene_graph.Update();
	REQUIRE( world_position( registry, child ).x == doctest::Approx( 15.f ) );
	REQUIRE( world_position( registry, grandchild ).x == doctest::Approx( 16.f ) );
	REQUIRE( registry.GetComponent<eage::ecs::SceneGraphComponent>( child ).world_enabled );
	REQUIRE( registry.GetComponent<eage::ecs::SceneGraphComponent>( other ).world_enabled );

	// WHEN the parent node is disabled
	scene_graph.SetNodeEnabled( parent, false );

	// THEN the parent and its descendants are disabled
	CHECK_FALSE( registry.GetComponent<eage::ecs::SceneGraphComponent>( parent ).world_enabled );
	CHECK_FALSE( registry.GetComponent<eage::ecs::SceneGraphComponent>( child ).world_enabled );
	CHECK_FALSE( registry.GetComponent<eage::ecs::SceneGraphComponent>( grandchild ).world_enabled );

	// AND the other branch stays enabled
	CHECK( registry.GetComponent<eage::ecs::SceneGraphComponent>( root ).world_enabled );
	CHECK( registry.GetComponent<eage::ecs::SceneGraphComponent>( other ).world_enabled );

	// WHEN the parent moves and the graph is updated
	registry.GetComponent<eage::ecs::TransformComponent>( parent ).SetPosition( glm::vec3( 100.f, 0.f, 0.f ) );
	registry.GetComponent<eage::ecs::TransformComponent>( other ).SetPosition( glm::vec3( 0.f, 20.f, 0.f ) );
	scene_graph.Update();

	// THEN disabled descendants keep their last world transform
	CHECK( world_position( registry, child ).x == doctest::Approx( 15.f ) );
	CHECK( world_position( registry, grandchild ).x == doctest::Approx( 16.f ) );

	// AND the enabled sibling still updates
	CHECK( world_position( registry, other ).y == doctest::Approx( 20.f ) );
}

TEST_CASE( "i_can_add_and_remove_nodes_at_runtime" )
{
	// GIVEN a live scene with a parent already under the root
	eage::ecs::ECSRegistry registry;
	eage::ecs::SceneGraphSystem scene_graph( registry );
	const eage::ecs::Entity root = make_node( registry );
	const eage::ecs::Entity parent = make_node( registry, glm::vec3( 10.f, 0.f, 0.f ) );
	const eage::ecs::Entity other_parent = make_node( registry, glm::vec3( 0.f, 3.f, 0.f ) );
	scene_graph.SetSceneRoot( root );
	scene_graph.AddNodeToParent( parent, root );
	scene_graph.AddNodeToParent( other_parent, root );
	scene_graph.Update();

	// WHEN a node is added at runtime
	const eage::ecs::Entity child = make_node( registry, glm::vec3( 5.f, 0.f, 0.f ) );
	scene_graph.AddNodeToParent( child, parent );
	scene_graph.Update();

	// THEN it is linked under the parent and receives a world transform
	CHECK( registry.GetComponent<eage::ecs::SceneGraphComponent>( child ).parent_entity == parent );
	CHECK( has_child( registry.GetComponent<eage::ecs::SceneGraphComponent>( parent ), child ) );
	CHECK( world_position( registry, child ).x == doctest::Approx( 15.f ) );

	// WHEN the node is removed from its parent
	scene_graph.RemoveNodeFromParent( child );

	// THEN it is unlinked from the parent
	CHECK( registry.GetComponent<eage::ecs::SceneGraphComponent>( child ).parent_entity == 0 );
	CHECK_FALSE( has_child( registry.GetComponent<eage::ecs::SceneGraphComponent>( parent ), child ) );

	// AND it can be reparented onto another live node
	scene_graph.AddNodeToParent( child, other_parent );
	scene_graph.Update();
	CHECK( registry.GetComponent<eage::ecs::SceneGraphComponent>( child ).parent_entity == other_parent );
	CHECK( has_child( registry.GetComponent<eage::ecs::SceneGraphComponent>( other_parent ), child ) );
	CHECK_FALSE( has_child( registry.GetComponent<eage::ecs::SceneGraphComponent>( parent ), child ) );
	CHECK( world_position( registry, child ).x == doctest::Approx( 5.f ) );
	CHECK( world_position( registry, child ).y == doctest::Approx( 3.f ) );
}

TEST_CASE( "removed_entity_automatically_remove_its_scene_node" )
{
	// GIVEN a hierarchy of root, parent, and child
	eage::ecs::ECSRegistry registry;
	eage::ecs::SceneGraphSystem scene_graph( registry );
	const eage::ecs::Entity root = make_node( registry );
	const eage::ecs::Entity parent = make_node( registry );
	const eage::ecs::Entity child = make_node( registry );
	const eage::ecs::Entity sibling = make_node( registry );
	scene_graph.SetSceneRoot( root );
	scene_graph.AddNodeToParent( parent, root );
	scene_graph.AddNodeToParent( child, parent );
	scene_graph.AddNodeToParent( sibling, parent );
	REQUIRE( has_child( registry.GetComponent<eage::ecs::SceneGraphComponent>( parent ), child ) );

	// WHEN the child entity is destroyed
	registry.DestroyEntity( child );

	// THEN its scene node is gone
	CHECK_FALSE( registry.HasComponent<eage::ecs::SceneGraphComponent>( child ) );
	CHECK_FALSE( registry.HasComponent<eage::ecs::TransformComponent>( child ) );

	// AND the parent no longer lists it as a child
	CHECK_FALSE( has_child( registry.GetComponent<eage::ecs::SceneGraphComponent>( parent ), child ) );
	CHECK( has_child( registry.GetComponent<eage::ecs::SceneGraphComponent>( parent ), sibling ) );

	// WHEN the parent entity is destroyed
	registry.DestroyEntity( parent );

	// THEN the root no longer lists it, and Update still succeeds
	CHECK_FALSE( registry.HasComponent<eage::ecs::SceneGraphComponent>( parent ) );
	CHECK_FALSE( has_child( registry.GetComponent<eage::ecs::SceneGraphComponent>( root ), parent ) );
	scene_graph.Update();
}

TEST_CASE( "queue_destroy_subtree_destroys_the_node_and_every_descendant" )
{
	// GIVEN a parent with descendants, and a sibling branch under the root
	eage::ecs::ECSRegistry registry;
	eage::ecs::SceneGraphSystem scene_graph( registry );
	const eage::ecs::Entity root = make_node( registry );
	const eage::ecs::Entity parent = make_node( registry );
	const eage::ecs::Entity child = make_node( registry );
	const eage::ecs::Entity grandchild = make_node( registry );
	const eage::ecs::Entity sibling = make_node( registry );
	scene_graph.SetSceneRoot( root );
	scene_graph.AddNodeToParent( parent, root );
	scene_graph.AddNodeToParent( child, parent );
	scene_graph.AddNodeToParent( grandchild, child );
	scene_graph.AddNodeToParent( sibling, root );

	// WHEN the parent subtree is queued and the destroy queue is flushed
	scene_graph.QueueDestroySubtree( parent );
	registry.FlushDestroyQueue();

	// THEN the parent and every descendant are gone
	CHECK_FALSE( registry.HasComponent<eage::ecs::SceneGraphComponent>( parent ) );
	CHECK_FALSE( registry.HasComponent<eage::ecs::SceneGraphComponent>( child ) );
	CHECK_FALSE( registry.HasComponent<eage::ecs::SceneGraphComponent>( grandchild ) );

	// AND the sibling branch is untouched
	CHECK( registry.HasComponent<eage::ecs::SceneGraphComponent>( sibling ) );
	CHECK( has_child( registry.GetComponent<eage::ecs::SceneGraphComponent>( root ), sibling ) );
	CHECK_FALSE( has_child( registry.GetComponent<eage::ecs::SceneGraphComponent>( root ), parent ) );

	// AND Update still succeeds
	scene_graph.Update();
}

TEST_CASE( "re_enabling_a_parent_restores_descendants_that_were_not_locally_disabled" )
{
	// GIVEN a parent with descendants that are all locally enabled
	eage::ecs::ECSRegistry registry;
	eage::ecs::SceneGraphSystem scene_graph( registry );
	const eage::ecs::Entity root = make_node( registry );
	const eage::ecs::Entity parent = make_node( registry, glm::vec3( 10.f, 0.f, 0.f ) );
	const eage::ecs::Entity child = make_node( registry, glm::vec3( 5.f, 0.f, 0.f ) );
	const eage::ecs::Entity grandchild = make_node( registry, glm::vec3( 1.f, 0.f, 0.f ) );
	scene_graph.SetSceneRoot( root );
	scene_graph.AddNodeToParent( parent, root );
	scene_graph.AddNodeToParent( child, parent );
	scene_graph.AddNodeToParent( grandchild, child );
	scene_graph.Update();
	REQUIRE( world_position( registry, child ).x == doctest::Approx( 15.f ) );
	REQUIRE( world_position( registry, grandchild ).x == doctest::Approx( 16.f ) );

	// WHEN the parent is disabled, moved, then re-enabled
	scene_graph.SetNodeEnabled( parent, false );
	registry.GetComponent<eage::ecs::TransformComponent>( parent ).SetPosition( glm::vec3( 100.f, 0.f, 0.f ) );
	scene_graph.Update();
	REQUIRE( world_position( registry, child ).x == doctest::Approx( 15.f ) );
	scene_graph.SetNodeEnabled( parent, true );

	// THEN descendants are world-enabled again and still locally enabled
	CHECK( registry.GetComponent<eage::ecs::SceneGraphComponent>( parent ).world_enabled );
	CHECK( registry.GetComponent<eage::ecs::SceneGraphComponent>( child ).enabled );
	CHECK( registry.GetComponent<eage::ecs::SceneGraphComponent>( child ).world_enabled );
	CHECK( registry.GetComponent<eage::ecs::SceneGraphComponent>( grandchild ).enabled );
	CHECK( registry.GetComponent<eage::ecs::SceneGraphComponent>( grandchild ).world_enabled );

	// AND they receive world transforms from the parent's new pose
	scene_graph.Update();
	CHECK( world_position( registry, child ).x == doctest::Approx( 105.f ) );
	CHECK( world_position( registry, grandchild ).x == doctest::Approx( 106.f ) );
}

TEST_CASE( "locally_disabled_child_stays_off_when_the_parent_is_re_enabled" )
{
	// GIVEN a parent with one locally disabled child and one enabled sibling
	eage::ecs::ECSRegistry registry;
	eage::ecs::SceneGraphSystem scene_graph( registry );
	const eage::ecs::Entity root = make_node( registry );
	const eage::ecs::Entity parent = make_node( registry, glm::vec3( 10.f, 0.f, 0.f ) );
	const eage::ecs::Entity disabled_child = make_node( registry, glm::vec3( 5.f, 0.f, 0.f ) );
	const eage::ecs::Entity grandchild = make_node( registry, glm::vec3( 1.f, 0.f, 0.f ) );
	const eage::ecs::Entity enabled_child = make_node( registry, glm::vec3( 0.f, 4.f, 0.f ) );
	scene_graph.SetSceneRoot( root );
	scene_graph.AddNodeToParent( parent, root );
	scene_graph.AddNodeToParent( disabled_child, parent );
	scene_graph.AddNodeToParent( grandchild, disabled_child );
	scene_graph.AddNodeToParent( enabled_child, parent );
	scene_graph.Update();
	scene_graph.SetNodeEnabled( disabled_child, false );
	REQUIRE_FALSE( registry.GetComponent<eage::ecs::SceneGraphComponent>( disabled_child ).world_enabled );
	REQUIRE_FALSE( registry.GetComponent<eage::ecs::SceneGraphComponent>( grandchild ).world_enabled );

	// WHEN the parent is disabled and then re-enabled
	scene_graph.SetNodeEnabled( parent, false );
	scene_graph.SetNodeEnabled( parent, true );

	// THEN the locally disabled child and its descendants stay off
	CHECK_FALSE( registry.GetComponent<eage::ecs::SceneGraphComponent>( disabled_child ).enabled );
	CHECK_FALSE( registry.GetComponent<eage::ecs::SceneGraphComponent>( disabled_child ).world_enabled );
	CHECK_FALSE( registry.GetComponent<eage::ecs::SceneGraphComponent>( grandchild ).world_enabled );

	// AND the sibling that was not locally disabled comes back
	CHECK( registry.GetComponent<eage::ecs::SceneGraphComponent>( enabled_child ).enabled );
	CHECK( registry.GetComponent<eage::ecs::SceneGraphComponent>( enabled_child ).world_enabled );

	// AND only the enabled sibling receives a new world transform
	registry.GetComponent<eage::ecs::TransformComponent>( parent ).SetPosition( glm::vec3( 100.f, 0.f, 0.f ) );
	scene_graph.Update();
	CHECK( world_position( registry, disabled_child ).x == doctest::Approx( 15.f ) );
	CHECK( world_position( registry, grandchild ).x == doctest::Approx( 16.f ) );
	CHECK( world_position( registry, enabled_child ).x == doctest::Approx( 100.f ) );
	CHECK( world_position( registry, enabled_child ).y == doctest::Approx( 4.f ) );
}

TEST_CASE( "node_added_under_a_disabled_parent_starts_world_disabled" )
{
	// GIVEN a disabled parent already in the scene
	eage::ecs::ECSRegistry registry;
	eage::ecs::SceneGraphSystem scene_graph( registry );
	const eage::ecs::Entity root = make_node( registry );
	const eage::ecs::Entity parent = make_node( registry, glm::vec3( 10.f, 0.f, 0.f ) );
	scene_graph.SetSceneRoot( root );
	scene_graph.AddNodeToParent( parent, root );
	scene_graph.SetNodeEnabled( parent, false );
	REQUIRE_FALSE( registry.GetComponent<eage::ecs::SceneGraphComponent>( parent ).world_enabled );

	// WHEN a node is added under that parent
	const eage::ecs::Entity child = make_node( registry, glm::vec3( 5.f, 0.f, 0.f ) );
	scene_graph.AddNodeToParent( child, parent );

	// THEN the child is locally enabled but world-disabled
	CHECK( registry.GetComponent<eage::ecs::SceneGraphComponent>( child ).enabled );
	CHECK_FALSE( registry.GetComponent<eage::ecs::SceneGraphComponent>( child ).world_enabled );
	CHECK( has_child( registry.GetComponent<eage::ecs::SceneGraphComponent>( parent ), child ) );

	// AND Update does not compose a world transform from the disabled parent
	registry.GetComponent<eage::ecs::TransformComponent>( parent ).SetPosition( glm::vec3( 100.f, 0.f, 0.f ) );
	scene_graph.Update();
	CHECK( world_position( registry, child ).x == doctest::Approx( 0.f ) );
}
