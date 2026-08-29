#include <doctest/doctest.h>

#include <memory>
#include <vector>

#include <ecs/ECS.h>

namespace
{
	struct MockTagComponent
	{
	};

	struct MockDataComponent
	{
		int value = 0;
	};

	struct MockTrackedComponent
	{
		std::shared_ptr<int> token;
	};

	struct MockExtraComponent
	{
		std::shared_ptr<int> token;
	};

	class MockQuerySystem
	{
	public:
		explicit MockQuerySystem( eage::ecs::ECSRegistry& registry )
			: mRegistry( registry )
		{
		}

		int QueryValue( eage::ecs::Entity entity ) const
		{
			for( auto [candidate, component] : mRegistry.GetComponentMap<MockDataComponent>() )
			{
				if( candidate == entity )
				{
					return component.value;
				}
			}

			return -1;
		}

		size_t Count() const
		{
			return mRegistry.GetComponentMap<MockDataComponent>().Size();
		}

	private:
		eage::ecs::ECSRegistry& mRegistry;
	};

	class MockDestroyObserver : public eage::ecs::ECSRegistry::Observer
	{
	public:
		explicit MockDestroyObserver( eage::ecs::ECSRegistry& registry )
			: mRegistry( registry )
		{
			mRegistry.Subscribe( this );
		}

		~MockDestroyObserver()
		{
			mRegistry.Unsubscribe( this );
		}

		void OnEntityDestroying( eage::ecs::Entity entity ) override
		{
			mDestroyedEntity = entity;
			mSawTag = mRegistry.HasComponent<MockTagComponent>( entity );
			mSawData = mRegistry.HasComponent<MockDataComponent>( entity );
			++mNotifyCount;
		}

		eage::ecs::Entity mDestroyedEntity = 0;
		bool mSawTag = false;
		bool mSawData = false;
		int mNotifyCount = 0;

	private:
		eage::ecs::ECSRegistry& mRegistry;
	};

	class MockNestedDestroyObserver : public eage::ecs::ECSRegistry::Observer
	{
	public:
		MockNestedDestroyObserver(
			eage::ecs::ECSRegistry& registry,
			eage::ecs::Entity parent,
			eage::ecs::Entity child )
			: mRegistry( registry )
			, mParent( parent )
			, mChild( child )
		{
			mRegistry.Subscribe( this );
		}

		~MockNestedDestroyObserver()
		{
			mRegistry.Unsubscribe( this );
		}

		void OnEntityDestroying( eage::ecs::Entity entity ) override
		{
			mDestroyedEntities.push_back( entity );
			if( entity == mParent )
			{
				mRegistry.QueueDestroyEntity( mChild );
			}
		}

		std::vector<eage::ecs::Entity> mDestroyedEntities;

	private:
		eage::ecs::ECSRegistry& mRegistry;
		eage::ecs::Entity mParent = 0;
		eage::ecs::Entity mChild = 0;
	};
}

TEST_CASE( "i_can_add_and_remove_entity" )
{
	// GIVEN the ECS registry
	eage::ecs::ECSRegistry registry;

	// WHEN an entity is added
	const eage::ecs::Entity entity = registry.CreateEntity();
	registry.AddComponent( entity, MockTagComponent{} );

	// THEN the entity exists
	REQUIRE( entity != 0 );
	REQUIRE( registry.HasComponent<MockTagComponent>( entity ) );

	// WHEN the entity is removed
	registry.DestroyEntity( entity );

	// THEN the entity is gone
	CHECK_FALSE( registry.HasComponent<MockTagComponent>( entity ) );

	// AND a new entity can still be created
	const eage::ecs::Entity next_entity = registry.CreateEntity();
	CHECK( next_entity != 0 );
	CHECK( next_entity != entity );
}

TEST_CASE( "i_can_add_component_to_an_entity_and_query_it_from_a_system" )
{
	// GIVEN the ECS registry, a query system, and an entity
	eage::ecs::ECSRegistry registry;
	MockQuerySystem system( registry );
	const eage::ecs::Entity entity = registry.CreateEntity();

	// WHEN a component is added to the entity
	registry.AddComponent( entity, MockDataComponent{ 42 } );

	// THEN the system can query the component
	CHECK( system.QueryValue( entity ) == 42 );

	// AND the system sees one matching entity
	CHECK( system.Count() == 1 );
}

TEST_CASE( "i_can_remove_a_component_from_an_entity" )
{
	// GIVEN an entity with a component
	eage::ecs::ECSRegistry registry;
	MockQuerySystem system( registry );
	const eage::ecs::Entity entity = registry.CreateEntity();
	registry.AddComponent( entity, MockDataComponent{ 7 } );
	REQUIRE( registry.HasComponent<MockDataComponent>( entity ) );
	REQUIRE( system.QueryValue( entity ) == 7 );

	// WHEN the component is removed
	registry.RemoveComponent<MockDataComponent>( entity );

	// THEN the entity no longer has the component
	CHECK_FALSE( registry.HasComponent<MockDataComponent>( entity ) );

	// AND the system no longer finds it
	CHECK( system.QueryValue( entity ) == -1 );
	CHECK( system.Count() == 0 );
}

TEST_CASE( "when_an_entity_is_removed_its_components_are_released" )
{
	// GIVEN an entity with a tracked component
	eage::ecs::ECSRegistry registry;
	auto token = std::make_shared<int>( 1 );
	const eage::ecs::Entity entity = registry.CreateEntity();
	registry.AddComponent( entity, MockTrackedComponent{ token } );
	REQUIRE( registry.HasComponent<MockTrackedComponent>( entity ) );
	REQUIRE( token.use_count() == 2 );

	// WHEN the entity is removed
	registry.DestroyEntity( entity );

	// THEN the component is gone from the registry
	CHECK_FALSE( registry.HasComponent<MockTrackedComponent>( entity ) );

	// AND the component resource is released
	CHECK( token.use_count() == 1 );
}

TEST_CASE( "observers_see_components_while_an_entity_is_destroying" )
{
	// GIVEN an entity with components and a subscribed observer
	eage::ecs::ECSRegistry registry;
	MockDestroyObserver observer( registry );
	const eage::ecs::Entity entity = registry.CreateEntity();
	registry.AddComponent( entity, MockTagComponent{} );
	registry.AddComponent( entity, MockDataComponent{ 11 } );

	// WHEN the entity is destroyed
	registry.DestroyEntity( entity );

	// THEN the observer is notified for that entity
	CHECK( observer.mNotifyCount == 1 );
	CHECK( observer.mDestroyedEntity == entity );

	// AND the observer can still see the components
	CHECK( observer.mSawTag );
	CHECK( observer.mSawData );
}

TEST_CASE( "flush_destroy_queue_drains_nested_queues" )
{
	// GIVEN a parent entity, a child entity, and an observer that queues the child
	eage::ecs::ECSRegistry registry;
	const eage::ecs::Entity parent = registry.CreateEntity();
	const eage::ecs::Entity child = registry.CreateEntity();
	registry.AddComponent( parent, MockTagComponent{} );
	registry.AddComponent( child, MockTagComponent{} );
	MockNestedDestroyObserver observer( registry, parent, child );

	// WHEN the parent is queued and the destroy queue is flushed
	registry.QueueDestroyEntity( parent );
	registry.FlushDestroyQueue();

	// THEN both entities are destroyed
	CHECK_FALSE( registry.HasComponent<MockTagComponent>( parent ) );
	CHECK_FALSE( registry.HasComponent<MockTagComponent>( child ) );

	// AND the observer is notified for parent then child
	REQUIRE( observer.mDestroyedEntities.size() == 2 );
	CHECK( observer.mDestroyedEntities[0] == parent );
	CHECK( observer.mDestroyedEntities[1] == child );
}

TEST_CASE( "destroy_entity_is_a_noop_for_invalid_or_empty_entities" )
{
	// GIVEN the ECS registry with a subscribed observer
	eage::ecs::ECSRegistry registry;
	MockDestroyObserver observer( registry );
	const eage::ecs::Entity empty_entity = registry.CreateEntity();

	// WHEN entity zero is destroyed
	registry.DestroyEntity( 0 );

	// THEN the observer is not notified
	CHECK( observer.mNotifyCount == 0 );

	// WHEN an entity with no components is destroyed
	registry.DestroyEntity( empty_entity );

	// THEN the observer is still not notified
	CHECK( observer.mNotifyCount == 0 );

	// AND flushing a queued zero entity does nothing
	registry.QueueDestroyEntity( 0 );
	registry.FlushDestroyQueue();
	CHECK( observer.mNotifyCount == 0 );
}

TEST_CASE( "destroying_an_entity_removes_every_component_type" )
{
	// GIVEN an entity with multiple component types
	eage::ecs::ECSRegistry registry;
	auto token_a = std::make_shared<int>( 1 );
	auto token_b = std::make_shared<int>( 1 );
	const eage::ecs::Entity entity = registry.CreateEntity();
	registry.AddComponent( entity, MockTagComponent{} );
	registry.AddComponent( entity, MockDataComponent{ 3 } );
	registry.AddComponent( entity, MockTrackedComponent{ token_a } );
	registry.AddComponent( entity, MockExtraComponent{ token_b } );
	REQUIRE( registry.HasComponent<MockTagComponent>( entity ) );
	REQUIRE( registry.HasComponent<MockDataComponent>( entity ) );
	REQUIRE( token_a.use_count() == 2 );
	REQUIRE( token_b.use_count() == 2 );

	// WHEN the entity is destroyed
	registry.DestroyEntity( entity );

	// THEN none of the component types remain
	CHECK_FALSE( registry.HasComponent<MockTagComponent>( entity ) );
	CHECK_FALSE( registry.HasComponent<MockDataComponent>( entity ) );
	CHECK_FALSE( registry.HasComponent<MockTrackedComponent>( entity ) );
	CHECK_FALSE( registry.HasComponent<MockExtraComponent>( entity ) );

	// AND every tracked component resource is released
	CHECK( token_a.use_count() == 1 );
	CHECK( token_b.use_count() == 1 );
}
