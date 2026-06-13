#ifndef _EAGE_COMPONENT_POOL_H_
#define _EAGE_COMPONENT_POOL_H_

#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace eage
{
namespace ecs
{

///
/// Dense sparse-set storage for a single component type.
/// Iteration walks mDense directly for cache-friendly access.
///
template<typename T>
class ComponentPool
{
public:
	struct ComponentRef
	{
		uint64_t entity = 0;
		T& component;
	};

	struct ConstComponentRef
	{
		uint64_t entity = 0;
		const T& component;
	};

	class Iterator
	{
	public:
		Iterator( ComponentPool* pool, size_t index )
			: mPool( pool )
			, mIndex( index )
		{
		}

		ComponentRef operator*() const
		{
			return { mPool->mEntities[mIndex], mPool->mDense[mIndex] };
		}

		Iterator& operator++()
		{
			++mIndex;
			return *this;
		}

		bool operator!=( const Iterator& other ) const
		{
			return mIndex != other.mIndex;
		}

	private:
		ComponentPool* mPool = nullptr;
		size_t mIndex = 0;
	};

	class ConstIterator
	{
	public:
		ConstIterator( const ComponentPool* pool, size_t index )
			: mPool( pool )
			, mIndex( index )
		{
		}

		ConstComponentRef operator*() const
		{
			return { mPool->mEntities[mIndex], mPool->mDense[mIndex] };
		}

		ConstIterator& operator++()
		{
			++mIndex;
			return *this;
		}

		bool operator!=( const ConstIterator& other ) const
		{
			return mIndex != other.mIndex;
		}

	private:
		const ComponentPool* mPool = nullptr;
		size_t mIndex = 0;
	};

	void Add( uint64_t entity, T&& component )
	{
		if( Has( entity ) )
		{
			return;
		}

		EnsureSparseCapacity( entity );

		const uint32_t dense_index = static_cast<uint32_t>( mDense.size() );
		mSparse[entity] = dense_index;
		mEntities.push_back( entity );
		mDense.push_back( std::move( component ) );
	}

	bool Has( uint64_t entity ) const
	{
		if( entity >= mSparse.size() )
		{
			return false;
		}

		return mSparse[entity] != kInvalidIndex;
	}

	T& Get( uint64_t entity )
	{
		return mDense[mSparse[entity]];
	}

	const T& Get( uint64_t entity ) const
	{
		return mDense[mSparse[entity]];
	}

	void Remove( uint64_t entity )
	{
		if( !Has( entity ) )
		{
			return;
		}

		const uint32_t dense_index = mSparse[entity];
		const uint32_t last_dense_index = static_cast<uint32_t>( mDense.size() - 1 );

		if( dense_index != last_dense_index )
		{
			const uint64_t moved_entity = mEntities[last_dense_index];
			mDense[dense_index] = std::move( mDense[last_dense_index] );
			mEntities[dense_index] = moved_entity;
			mSparse[moved_entity] = dense_index;
		}

		mDense.pop_back();
		mEntities.pop_back();
		mSparse[entity] = kInvalidIndex;
	}

	size_t Size() const
	{
		return mDense.size();
	}

	bool Empty() const
	{
		return mDense.empty();
	}

	Iterator begin()
	{
		return Iterator( this, 0 );
	}

	Iterator end()
	{
		return Iterator( this, mDense.size() );
	}

	ConstIterator begin() const
	{
		return ConstIterator( this, 0 );
	}

	ConstIterator end() const
	{
		return ConstIterator( this, mDense.size() );
	}

private:
	static constexpr uint32_t kInvalidIndex = std::numeric_limits<uint32_t>::max();

	void EnsureSparseCapacity( uint64_t entity )
	{
		if( entity >= mSparse.size() )
		{
			mSparse.resize( entity + 1, kInvalidIndex );
		}
	}

	std::vector<T> mDense;
	std::vector<uint64_t> mEntities;
	std::vector<uint32_t> mSparse;
};

}
}

#endif // _EAGE_COMPONENT_POOL_H_
