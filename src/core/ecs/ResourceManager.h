#ifndef _EAGE_RESOURCE_MANAGER_H_
#define _EAGE_RESOURCE_MANAGER_H_

#include <unordered_map>
#include <memory>
#include <string>
#include <cstdint>

namespace eage::ecs
{
	using ResourceID = uint32_t;
	static constexpr ResourceID INVALID_ID = 0;

	template<typename T>
	class ResourceManager
	{
	public:
		ResourceManager() = default;
		~ResourceManager() = default;

		// Non-copyable, movable
		ResourceManager(const ResourceManager&) = delete;
		ResourceManager& operator=(const ResourceManager&) = delete;
		ResourceManager(ResourceManager&&) = default;
		ResourceManager& operator=(ResourceManager&&) = default;

		// Store a resource and return its ID
		template<typename... Args>
		ResourceID Create(Args&&... args)
		{
			auto resource = std::make_shared<T>(std::forward<Args>(args)...);
			ResourceID id = mNextID++;
			mResources[id] = resource;
			mReferenceCounts[id] = 1;
			return id;
		}

		// Store an existing resource
		ResourceID Store(std::unique_ptr<T> resource)
		{
			if (!resource) return INVALID_ID;
			
			ResourceID id = mNextID++;
			mResources[id] = std::move( resource );
			mReferenceCounts[id] = 1;
			return id;
		}

		// Get resource by ID
		T* Get(ResourceID id) const
		{
			auto it = mResources.find(id);
			return (it != mResources.end()) ? it->second.get() : nullptr;
		}

		// Add reference to resource
		void AddReference(ResourceID id)
		{
			auto it = mReferenceCounts.find(id);
			if (it != mReferenceCounts.end()) {
				++it->second;
			}
		}

		// Remove reference, delete if no more references
		void RemoveReference(ResourceID id)
		{
			auto it = mReferenceCounts.find(id);
			if (it != mReferenceCounts.end()) {
				--it->second;
				if (it->second <= 0) {
					mResources.erase(id);
					mReferenceCounts.erase(id);
				}
			}
		}

		// Check if resource exists
		bool Exists(ResourceID id) const
		{
			return mResources.find(id) != mResources.end();
		}

		// Get reference count
		int GetReferenceCount(ResourceID id) const
		{
			auto it = mReferenceCounts.find(id);
			return (it != mReferenceCounts.end()) ? it->second : 0;
		}

		// Clear all resources
		void Clear()
		{
			mResources.clear();
			mReferenceCounts.clear();
		}

		// Get total resource count
		size_t Size() const
		{
			return mResources.size();
		}

	private:
		ResourceID mNextID = 1; // Start from 1, reserve 0 for INVALID_ID
		std::unordered_map<ResourceID, std::unique_ptr<T>> mResources;
		std::unordered_map<ResourceID, int> mReferenceCounts;
	};
}

#endif // _EAGE_RESOURCE_MANAGER_H_