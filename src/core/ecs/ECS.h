#ifndef _EAGE_ECS_H_
#define _EAGE_ECS_H_

#include <cstdint>
#include <unordered_map>
#include <typeindex>
#include <any>

namespace eage
{
namespace ecs
{
using Entity = uint64_t;

///
/// ECSRegistry: Manages entities and their components
///
class ECSRegistry
{
public:
	ECSRegistry() = default;

	ECSRegistry(const ECSRegistry&) = delete;
	ECSRegistry& operator=(const ECSRegistry&) = delete;

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
	void AddComponent(Entity e, T&& component )
	{
		auto& map = GetComponentMap<T>();
		map.emplace( e, std::move(component) );
	}

	///
	/// Check if an entity has a specific component
	///
	template<typename T>
	bool HasComponent(Entity e) const
	{
		auto& map = GetComponentMap<T>();
		return map.find(e) != map.end();
	}

	///
	/// Get a component of an entity
	///
	template<typename T>
	T& GetComponent(Entity e)
	{
		return GetComponentMap<T>().at(e);
	}

	///
	/// Get a map of all entities and their components of a specific type
	///
	template<typename T>
	const std::unordered_map<Entity, T>& GetComponentMap() const
	{
		auto type = std::type_index( typeid(T) );
		auto it = mComponents.find( type );
		
		if( it == mComponents.end() )
		{
			// Return empty map if component type doesn't exist
			static const std::unordered_map<Entity, T> empty_map;
			return empty_map;
		}
		return std::any_cast<const std::unordered_map<Entity, T>&>( it->second );
	}

	///
	/// Get a map of all entities and their components of a specific type (non-const)
	///
	template<typename T>
	std::unordered_map<Entity, T>& GetComponentMap() 
	{
		auto type = std::type_index(typeid(T));
		if( mComponents.find(type) == mComponents.end() )
		{
			mComponents.emplace( type, std::unordered_map<Entity, T>{} ); // Use emplace to avoid copying
		}
		return std::any_cast<std::unordered_map<Entity, T>&>(mComponents[type]);
	}

private:
	Entity mNextEntity = 1;
	std::unordered_map<std::type_index, std::any> mComponents;
};

}
}

#endif // _EAGE_ECS_H_