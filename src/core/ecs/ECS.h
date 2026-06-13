#ifndef _EAGE_ECS_H_
#define _EAGE_ECS_H_

#include <cstdint>
#include <memory>
#include <typeindex>
#include <unordered_map>

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
};

template<typename T>
class TypedComponentPool : public AbstractComponentPool
{
public:
	ComponentPool<T> pool;
};

///
/// ECSRegistry: Manages entities and their components
///
class ECSRegistry
{
public:
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
	Entity mNextEntity = 1;
	std::unordered_map<std::type_index, std::unique_ptr<AbstractComponentPool>> mComponents;
};

}
}

#endif // _EAGE_ECS_H_
