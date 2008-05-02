// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PIONPOOLALLOCATOR_HEADER__
#define __PION_PIONPOOLALLOCATOR_HEADER__

#include <cstdlib>
#include <boost/array.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/static_assert.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/pool/pool.hpp>
#include <pion/PionConfig.hpp>
#include <pion/PionException.hpp>

/// the following enables use of the lock-free cache
#if defined(PION_HAVE_LOCKFREE) && !defined(_MSC_VER)
#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable: 4800) // forcing value to bool 'true' or 'false' (performance warning)
#endif
	#include <boost/lockfree/tagged_ptr.hpp>
#ifdef _MSC_VER
	#pragma warning(pop)
#endif
	#include <boost/lockfree/atomic_int.hpp>
#endif


namespace pion {	// begin namespace pion


///
/// PionPoolAllocator: a thread-safe, small object allocator that sacrifices 
///                    memory utilization for performance.  It combines a 
///                    collection of fixed-size pooled memory allocators with
///                    lock-free caches to achieve nearly wait-free, constant 
///                    time performance when used for an extended period of time
///
template <std::size_t MinSize = 16, std::size_t MaxSize = 256>
class PionPoolAllocator
	: private boost::noncopyable
{
public:

	/// virtual destructor
	virtual ~PionPoolAllocator()
	{}
	
	/// default constructor
	PionPoolAllocator(void)
	{
		for (std::size_t n = 0; n < NumberOfAllocs; ++n) {
			m_pools[n].reset(new FixedSizeAlloc((n+1) * MinSize));
		}
	}

	/**
	 * allocates a block of memory
	 *
	 * @param n minimum size of the new memory block, in bytes
	 *
	 * @return void * raw pointer to the new memory block
	 */
	inline void *malloc(std::size_t n)
	{
		// check for size greater than MaxSize
		if (n > MaxSize)
			return ::malloc(n);
		FixedSizeAlloc *pool_ptr = getPool(n);

#if defined(PION_HAVE_LOCKFREE) && !defined(_MSC_VER)
		while (true) {
			// get copy of free list pointer
			FreeListPtr old_free_ptr(pool_ptr->m_free_ptr);
			if (! old_free_ptr)
				break;	// use pool alloc if free list is empty
			
			// prepare a new free list pointer
			FreeListPtr new_free_ptr(old_free_ptr->next);
			new_free_ptr.set_tag(old_free_ptr.get_tag() + 1);
			
			// use CAS operation to swap the free list pointer
			if (pool_ptr->m_free_ptr.CAS(old_free_ptr, new_free_ptr))
				return reinterpret_cast<void*>(old_free_ptr.get_ptr());
		}
#endif

		boost::unique_lock<boost::mutex> pool_lock(pool_ptr->m_mutex);
		return pool_ptr->m_pool.malloc();
	}

	/**
	 * deallocates a block of memory
	 *
	 * @param ptr raw pointer to the block of memory
	 * @param n requested size of the memory block, in bytes (actual size may be larger)
	 */
	inline void free(void *ptr, std::size_t n)
	{
		// check for size greater than MaxSize
		if (n > MaxSize) {
			::free(ptr);
			return;
		}
		FixedSizeAlloc *pool_ptr = getPool(n);
#if defined(PION_HAVE_LOCKFREE) && !defined(_MSC_VER)
		while (true) {
			// get copy of free list pointer
			FreeListPtr old_free_ptr(pool_ptr->m_free_ptr);
			
			// cast memory being released to a free list node
			// and point its next pointer to the current free list
			FreeListNode *node_ptr = reinterpret_cast<FreeListNode*>(ptr);
			node_ptr->next.set_ptr(old_free_ptr.get_ptr());
			
			// create a temporary new pointer for the CAS operation
			FreeListPtr new_free_ptr(node_ptr, old_free_ptr.get_tag() + 1);

			// use CAS operation to swap the free list pointer
			if (pool_ptr->m_free_ptr.CAS(old_free_ptr, new_free_ptr))
				break;
		}
#else
		boost::unique_lock<boost::mutex> pool_lock(pool_ptr->m_mutex);
		return pool_ptr->m_pool.free(ptr);
#endif
	}
	
	/**
	 * releases every memory block that does not have any allocated chunks
	 *
	 * @param bool true if at least one block of memory was released
	 */
	inline bool release_memory(void)
	{
		bool result = false;
		for (std::size_t n = 0; n < NumberOfAllocs; ++n) {
			boost::unique_lock<boost::mutex> pool_lock(m_pools[n]->m_mutex);
			if (m_pools[n]->m_pool.release_memory())
				result = true;
		}
		return result;
	}
	

protected:

#if defined(PION_HAVE_LOCKFREE) && !defined(_MSC_VER)
	/// data structure used to represent a free node
	struct FreeListNode {
		boost::lockfree::tagged_ptr<struct FreeListNode>	next;
	};
	
	/// data type for a tagged free-list pointer
	typedef boost::lockfree::tagged_ptr<struct FreeListNode>	FreeListPtr;
#else
	typedef void *	FreeListPtr;
#endif
	
	/// ensure that:
	///   a) MaxSize >= MinSize
	///   b) MaxSize is a multiple of MinSize
	///   c) MinSize >= sizeof(FreeNodePtr)  [usually 16]
	BOOST_STATIC_ASSERT(MaxSize >= MinSize);
	BOOST_STATIC_ASSERT(MaxSize % MinSize == 0);
#if defined(PION_HAVE_LOCKFREE) && !defined(_MSC_VER)
	BOOST_STATIC_ASSERT(MinSize >= sizeof(FreeListNode));
#endif
	
	/// constant representing the number of fixed-size pool allocators
	enum { NumberOfAllocs = ((MaxSize-1) / MinSize) + 1 };

	/**
	 * data structure used to represent a pooled memory
	 * allocator for blocks of a specific size
	 */
	struct FixedSizeAlloc
	{
		/**
		 * constructs a new fixed-size pool allocator
		 *
		 * @param size size of memory blocks managed by this allocator, in bytes
		 */
		FixedSizeAlloc(std::size_t size)
			: m_size(size), m_pool(size), m_free_ptr(NULL)
		{}
		
		/// used to protect access to the memory pool
		boost::mutex		m_mutex;

		/// size of memory blocks managed by this allocator, in bytes
		std::size_t			m_size;
		
		/// underlying pool allocator used for memory management
		boost::pool<>		m_pool;

		/// pointer to a list of free nodes (for lock-free cache)
		FreeListPtr			m_free_ptr;		
	};
	

	/**
	 * gets an appropriate fixed-size pool allocator
	 *
	 * @param n the number of bytes to be (de)allocated
	 *
	 * @return FixedSizeAlloc* pointer to the appropriate fixed-size allocator
	 */
	inline FixedSizeAlloc* getPool(const std::size_t n)
	{
		PION_ASSERT(n > 0);
		PION_ASSERT(n <= MaxSize);
		return m_pools[ (n-1) / MinSize ].get();
	}


private:

	/// a collection of fixed-size pool allocators
	boost::array<boost::scoped_ptr<FixedSizeAlloc>, NumberOfAllocs>	m_pools;
};

	
}	// end namespace pion

#endif
