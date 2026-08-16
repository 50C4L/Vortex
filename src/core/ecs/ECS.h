#ifndef _EAGE_ECS_H_
#define _EAGE_ECS_H_

#include <algorithm>
#include <cstdint>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include <ecs/ComponentPool.h>

namespace eage
{
namespace ecs
{
using Entity = uint64_t;

class AbstractComponentPool
{
public:
	virtual ~AbstractComponentPool() = default;
	virtual void Remove( Entity entity ) = 0;
	virtual bool Has( Entity entity ) const = 0;
};

template<typename T>
class TypedComponentPool : public AbstractComponentPool
{
public:
	void Remove( Entity entity ) override
	{
		pool.Remove( entity );
	}

	bool Has( Entity entity ) const override
	{
		return pool.Has( entity );
	}

	ComponentPool<T> pool;
};

///
/// ECSRegistry: Manages entities and their components
///
class ECSRegistry
{
public:
	class Observer
	{
	public:
		virtual ~Observer() = default;
		virtual void OnEntityDestroying( Entity entity ) = 0;
	};

	ECSRegistry() = default;

	ECSRegistry( const ECSRegistry& ) = delete;
	ECSRegistry& operator=( const ECSRegistry& ) = delete;

	///
	/// Create a new entity
	///
	/// @return
	///   New entity ID
	///
	Entity CreateEntity()
	{
		return mNextEntity++;
	}

	void Subscribe( Observer* observer )
	{
		if( observer == nullptr )
		{
			return;
		}
		mObservers.push_back( observer );
	}

	void Unsubscribe( Observer* observer )
	{
		mObservers.erase(
			std::remove( mObservers.begin(), mObservers.end(), observer ),
			mObservers.end() );
	}

	///
	/// Immediately destroy an entity: notify observers, then remove all components.
	/// No-op if the entity has no components. Observers must not call DestroyEntity
	/// re-entrantly; use QueueDestroyEntity for additional entities.
	///
	void DestroyEntity( Entity entity )
	{
		if( entity == 0 || !HasAnyComponents( entity ) )
		{
			return;
		}

		for( Observer* observer : mObservers )
		{
			observer->OnEntityDestroying( entity );
		}

		for( auto& [type, pool] : mComponents )
		{
			pool->Remove( entity );
		}
	}

	void QueueDestroyEntity( Entity entity )
	{
		if( entity == 0 )
		{
			return;
		}
		mDestroyQueue.push_back( entity );
	}

	///
	/// Process all queued destructions. Drains until empty so entities queued
	/// during destroy (e.g. subtree children) are processed in the same flush.
	///
	void FlushDestroyQueue()
	{
		while( !mDestroyQueue.empty() )
		{
			Entity entity = mDestroyQueue.front();
			mDestroyQueue.erase( mDestroyQueue.begin() );
			DestroyEntity( entity );
		}
	}

	///
	/// Add a component to an entity
	///
	template<typename T>
	void AddComponent( Entity e, T&& component )
	{
		GetComponentMap<T>().Add( e, std::forward<T>( component ) );
	}

	///
	/// Check if an entity has a specific component
	///
	template<typename T>
	bool HasComponent( Entity e ) const
	{
		return GetComponentMap<T>().Has( e );
	}

	///
	/// Get a component of an entity
	///
	template<typename T>
	T& GetComponent( Entity e )
	{
		return GetComponentMap<T>().Get( e );
	}

	///
	/// Get a component of an entity (const)
	///
	template<typename T>
	const T& GetComponent( Entity e ) const
	{
		return GetComponentMap<T>().Get( e );
	}

	///
	/// Remove a component from an entity
	///
	template<typename T>
	void RemoveComponent( Entity e )
	{
		GetComponentMap<T>().Remove( e );
	}

	///
	/// Get the dense component pool for a specific type
	///
	template<typename T>
	const ComponentPool<T>& GetComponentMap() const
	{
		auto type = std::type_index( typeid( T ) );
		auto it = mComponents.find( type );

		if( it == mComponents.end() )
		{
			static const ComponentPool<T> empty_pool;
			return empty_pool;
		}

		return static_cast<const TypedComponentPool<T>*>( it->second.get() )->pool;
	}

	///
	/// Get the dense component pool for a specific type (non-const)
	///
	template<typename T>
	ComponentPool<T>& GetComponentMap()
	{
		auto type = std::type_index( typeid( T ) );
		auto it = mComponents.find( type );

		if( it == mComponents.end() )
		{
			auto typed_pool = std::make_unique<TypedComponentPool<T>>();
			auto* pool = &typed_pool->pool;
			mComponents.emplace( type, std::move( typed_pool ) );
			return *pool;
		}

		return static_cast<TypedComponentPool<T>*>( it->second.get() )->pool;
	}

private:
	bool HasAnyComponents( Entity entity ) const
	{
		for( const auto& [type, pool] : mComponents )
		{
			if( pool->Has( entity ) )
			{
				return true;
			}
		}
		return false;
	}

	Entity mNextEntity = 1;
	std::unordered_map<std::type_index, std::unique_ptr<AbstractComponentPool>> mComponents;
	std::vector<Observer*> mObservers;
	std::vector<Entity> mDestroyQueue;
};

}
}

#endif // _EAGE_ECS_H_
