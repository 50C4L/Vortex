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

class ECSRegistry
{
public:
	ECSRegistry() = default;

	ECSRegistry(const ECSRegistry&) = delete;
	ECSRegistry& operator=(const ECSRegistry&) = delete;

	Entity CreateEntity()
	{
		return mNextEntity++;
	}

	template<typename T>
	void AddComponent(Entity e, T&& component )
	{
		auto& map = GetComponentMap<T>();
		map.emplace( e, std::move(component) );
	}

	template<typename T>
	bool HasComponent(Entity e) const
	{
		auto& map = GetComponentMap<T>();
		return map.find(e) != map.end();
	}

	template<typename T>
	T& GetComponent(Entity e)
	{
		return GetComponentMap<T>().at(e);
	}

	template<typename T>
	const std::unordered_map<Entity, T>& GetComponentMap() const
	{
		auto type = std::type_index(typeid(T));
		return std::any_cast<const std::unordered_map<Entity, T>&>(components.at(type));
	}

	template<typename T>
	std::unordered_map<Entity, T>& GetComponentMap() 
	{
		auto type = std::type_index(typeid(T));
		if( components.find(type) == components.end() )
		{
			components.emplace( type, std::unordered_map<Entity, T>{} ); // Use emplace to avoid copying
		}
		return std::any_cast<std::unordered_map<Entity, T>&>(components[type]);
	}

private:
	Entity mNextEntity = 1;
	std::unordered_map<std::type_index, std::any> components;
};

}
}

#endif // _EAGE_ECS_H_