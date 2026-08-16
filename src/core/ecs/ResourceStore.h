#ifndef _EAGE_RESOURCE_STORE_H_
#define _EAGE_RESOURCE_STORE_H_

#include <cstdint>
#include <memory>
#include <unordered_map>

namespace eage::ecs
{
	using ResourceId = uint32_t;
	static constexpr ResourceId INVALID_ID = 0;

	///
	/// Id-keyed, ref-counted store for smart-pointer resources.
	///
	template<typename PtrType>
	class ResourceStore
	{
	public:
		using pointer_type = PtrType;
		using element_type = typename PtrType::element_type;

		ResourceStore() = default;
		~ResourceStore() = default;

		ResourceStore( const ResourceStore& ) = delete;
		ResourceStore& operator=( const ResourceStore& ) = delete;
		ResourceStore( ResourceStore&& ) = default;
		ResourceStore& operator=( ResourceStore&& ) = default;

		/// Store a resource with initial refcount 1. Caller should Adopt into a ResourceHandle
		/// (or treat a component as owning that ref, e.g. physics bodies).
		ResourceId Store( PtrType resource )
		{
			if( !resource )
			{
				return INVALID_ID;
			}

			ResourceId id = mNextID++;
			mResources[id] = std::move( resource );
			mReferenceCounts[id] = 1;
			return id;
		}

		element_type* Get( ResourceId id ) const
		{
			auto it = mResources.find( id );
			return ( it != mResources.end() ) ? it->second.get() : nullptr;
		}

		void AddReference( ResourceId id )
		{
			auto it = mReferenceCounts.find( id );
			if( it != mReferenceCounts.end() )
			{
				++it->second;
			}
		}

		void RemoveReference( ResourceId id )
		{
			auto it = mReferenceCounts.find( id );
			if( it != mReferenceCounts.end() )
			{
				--it->second;
				if( it->second <= 0 )
				{
					mResources.erase( id );
					mReferenceCounts.erase( id );
				}
			}
		}

		bool Exists( ResourceId id ) const
		{
			return mResources.find( id ) != mResources.end();
		}

		int GetReferenceCount( ResourceId id ) const
		{
			auto it = mReferenceCounts.find( id );
			return ( it != mReferenceCounts.end() ) ? it->second : 0;
		}

		void Clear()
		{
			mResources.clear();
			mReferenceCounts.clear();
		}

		size_t Size() const
		{
			return mResources.size();
		}

	private:
		ResourceId mNextID = 1;
		std::unordered_map<ResourceId, PtrType> mResources;
		std::unordered_map<ResourceId, int> mReferenceCounts;
	};

	///
	/// RAII owner of one ResourceStore reference. Adopt after Store(); copy AddReferences;
	/// destroy / Reset RemoveReferences. Type-erased so public APIs need not expose PtrType.
	///
	class ResourceHandle
	{
	public:
		ResourceHandle() = default;

		template<typename PtrType>
		static ResourceHandle Adopt( ResourceStore<PtrType>& store, ResourceId id )
		{
			ResourceHandle handle;
			if( id != INVALID_ID )
			{
				handle.mStore = &store;
				handle.mId = id;
				handle.mAddReference = &AddReferenceThunk<PtrType>;
				handle.mRemoveReference = &RemoveReferenceThunk<PtrType>;
			}
			return handle;
		}

		~ResourceHandle()
		{
			Reset();
		}

		ResourceHandle( const ResourceHandle& other )
			: mStore( other.mStore )
			, mId( other.mId )
			, mAddReference( other.mAddReference )
			, mRemoveReference( other.mRemoveReference )
		{
			if( mStore && mId != INVALID_ID && mAddReference )
			{
				mAddReference( mStore, mId );
			}
		}

		ResourceHandle& operator=( const ResourceHandle& other )
		{
			if( this == &other )
			{
				return *this;
			}
			Reset();
			mStore = other.mStore;
			mId = other.mId;
			mAddReference = other.mAddReference;
			mRemoveReference = other.mRemoveReference;
			if( mStore && mId != INVALID_ID && mAddReference )
			{
				mAddReference( mStore, mId );
			}
			return *this;
		}

		ResourceHandle( ResourceHandle&& other ) noexcept
			: mStore( other.mStore )
			, mId( other.mId )
			, mAddReference( other.mAddReference )
			, mRemoveReference( other.mRemoveReference )
		{
			other.mStore = nullptr;
			other.mId = INVALID_ID;
			other.mAddReference = nullptr;
			other.mRemoveReference = nullptr;
		}

		ResourceHandle& operator=( ResourceHandle&& other ) noexcept
		{
			if( this == &other )
			{
				return *this;
			}
			Reset();
			mStore = other.mStore;
			mId = other.mId;
			mAddReference = other.mAddReference;
			mRemoveReference = other.mRemoveReference;
			other.mStore = nullptr;
			other.mId = INVALID_ID;
			other.mAddReference = nullptr;
			other.mRemoveReference = nullptr;
			return *this;
		}

		ResourceId Get() const
		{
			return mId;
		}

		explicit operator bool() const
		{
			return mId != INVALID_ID;
		}

		void Reset()
		{
			if( mStore && mId != INVALID_ID && mRemoveReference )
			{
				mRemoveReference( mStore, mId );
			}
			mStore = nullptr;
			mId = INVALID_ID;
			mAddReference = nullptr;
			mRemoveReference = nullptr;
		}

	private:
		using RefFn = void ( * )( void* store, ResourceId id );

		template<typename PtrType>
		static void AddReferenceThunk( void* store, ResourceId id )
		{
			static_cast<ResourceStore<PtrType>*>( store )->AddReference( id );
		}

		template<typename PtrType>
		static void RemoveReferenceThunk( void* store, ResourceId id )
		{
			static_cast<ResourceStore<PtrType>*>( store )->RemoveReference( id );
		}

		void* mStore = nullptr;
		ResourceId mId = INVALID_ID;
		RefFn mAddReference = nullptr;
		RefFn mRemoveReference = nullptr;
	};
}

#endif // _EAGE_RESOURCE_STORE_H_
