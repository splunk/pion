// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PIONLOCKFREEQUEUE_HEADER__
#define __PION_PIONLOCKFREEQUEUE_HEADER__

#ifndef PION_HAVE_LOCKFREE
	#error "PionLockFreeQueue requires the boost::lockfree library!"
#endif
#ifdef _MSC_VER
	#include <iso646.h>
	#pragma warning(push)
	#pragma warning(disable: 4800) // forcing value to bool 'true' or 'false' (performance warning)
#endif
#include <boost/lockfree/tagged_ptr.hpp>
#ifdef _MSC_VER
	#pragma warning(pop)
#endif
#include <boost/lockfree/cas.hpp>
#include <boost/lockfree/freelist.hpp>
#include <boost/lockfree/branch_hints.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/thread.hpp>
#include <pion/PionConfig.hpp>
//#include <boost/array.hpp>
//#include <boost/cstdint.hpp>
//#include <boost/static_assert.hpp>


// NOTE: the data structures contained in this file are based upon algorithms
// published in the paper "Simple, Fast, and Practical Non-Blocking and Blocking
// Concurrent Queue Algorithms" (1996, Maged M. Michael and Michael L. Scott,
// Department of Computer Science, University of Rochester).
// See http://www.cs.rochester.edu/u/scott/papers/1996_PODC_queues.pdf


namespace pion {	// begin namespace pion


///
/// PionLockFreeQueue: a FIFO queue that is thread-safe and lock-free
/// 
template <typename T>
class PionLockFreeQueue :
	private boost::noncopyable
{
protected:
	
	/// data structure used to wrap each item in the queue
	struct QueueNode {
		/// default constructor
		QueueNode(void) : next(NULL) {}
		
		/// constructs QueueNode with a data value
		QueueNode(const T& d) : next(NULL), data(d) {}
		
		/// points to the next node in the queue
		boost::lockfree::tagged_ptr<QueueNode>	next;

		/// data wrapped by the node item
		T	data;
	};
	
	/// data type for an atomic QueueNode pointer
	typedef boost::lockfree::tagged_ptr<QueueNode>	QueueNodePtr;
	
	/// returns a new queue node item for use in the queue
	inline QueueNode *createNode(void) {
		QueueNode *node_ptr = m_free_list.allocate();
		new(node_ptr) QueueNode();
		return node_ptr;
	}
	
	/// frees memory for an existing queue node item
	inline void destroyNode(QueueNode *node_ptr) {
		node_ptr->~QueueNode();
		m_free_list.deallocate(node_ptr);
	}
	
	
public:
	
	/// constructs a new PionLockFreeQueue
	PionLockFreeQueue(void) {
		// initialize with a dummy node since m_head_ptr is always 
		// pointing to the item before the head of the list
		QueueNode *dummy_ptr = createNode();
		m_head_ptr.set_ptr(dummy_ptr);
		m_tail_ptr.set_ptr(dummy_ptr);
	}
	
	/// virtual destructor
	virtual ~PionLockFreeQueue() {
		clear();
		destroyNode(m_head_ptr.get_ptr());
	}
	
	/// returns true if the queue is empty; false if it is not
	inline bool empty(void) const {
		return (m_head_ptr.get_ptr() == m_tail_ptr.get_ptr());
	}
	
	/// clears the queue by removing all remaining items
	/// WARNING: this is NOT thread-safe!
	volatile void clear(void) {
		while (! empty()) {
			QueueNodePtr node_ptr(m_head_ptr);
			m_head_ptr = m_head_ptr->next;
			destroyNode(node_ptr.get_ptr());
		}
	}
	
	/**
	 * pushes a new item into the back of the queue
	 *
	 * @param t the item to add to the back of the queue
	 */
	inline void push(const T& t) {
		// create a new list node for the queue item
		QueueNode *node_ptr = createNode();
		node_ptr->data = t;
		
		while (true) {
			// get copy of tail pointer
			QueueNodePtr tail_ptr(m_tail_ptr);
			//boost::lockfree::memory_barrier();

			// get copy of tail's next pointer
			QueueNodePtr next_ptr(tail_ptr->next);
			boost::lockfree::memory_barrier();
			
			// make sure that the tail pointer has not changed since reading next
			if (boost::lockfree::likely(tail_ptr == m_tail_ptr)) {
				// check if tail was pointing to the last node
				if (next_ptr.get_ptr() == NULL) {
					// try to link the new node at the end of the list
					if (tail_ptr->next.CAS(next_ptr, node_ptr)) {
						// done with enqueue; try to swing tail to the inserted node
						m_tail_ptr.CAS(tail_ptr, node_ptr);
						break;
					}
				} else {
					// try to swing tail to the next node
					m_tail_ptr.CAS(tail_ptr, next_ptr.get_ptr());
				}
			}
		}
	}	
	
	/**
	 * pops the next item from the top of the queue
	 *
	 * @param t assigned to the item at the top of the queue, if it is not empty
	 *
	 * @return true if an item was retrieved, false if the queue is empty
	 */
	inline bool pop(T& t) {
		while (true) {
			// get copy of head pointer
			QueueNodePtr head_ptr(m_head_ptr);
			//boost::lockfree::memory_barrier();

			// get copy of tail pointer
			QueueNodePtr tail_ptr(m_tail_ptr);
			QueueNode *next_ptr = head_ptr->next.get_ptr();
			boost::lockfree::memory_barrier();
			
			// check consistency of head pointer
			if (boost::lockfree::likely(head_ptr == m_head_ptr)) {

				// check if queue is empty, or tail is falling behind
				if (head_ptr.get_ptr() == tail_ptr.get_ptr()) {
					// is queue empty?
					if (next_ptr == NULL)
						return false;	// queue is empty
					
					// not empty; try to advance tail to catch it up
					m_tail_ptr.CAS(tail_ptr, next_ptr);

				} else {
					// tail is OK
					// read value before CAS, otherwise another dequeue
					//   might free the next node
					t = next_ptr->data;
					
					// try to swing head to the next node
					if (m_head_ptr.CAS(head_ptr, next_ptr)) {
						// success -> nuke the old head item
						destroyNode(head_ptr.get_ptr());
						break;	// exit loop
					}
				}
			}
		}
		
		// item successfully retrieved
		return true;
	}

	
private:
	
	/// data type for a caching free list of queue nodes
	typedef boost::lockfree::caching_freelist<QueueNode>	NodeFreeList;

	
	/// a caching free list of queue nodes used to reduce memory operations
	NodeFreeList		m_free_list;
	
	/// pointer to the first item in the list
	QueueNodePtr		m_head_ptr;
	
	/// pointer to the last item in the list
#ifdef _MSC_VER
	#pragma pack(8) /* force head_ and tail_ to different cache lines! */
	QueueNodePtr		m_tail_ptr;
#else
	QueueNodePtr		m_tail_ptr __attribute__((aligned(64))); /* force head_ and tail_ to different cache lines! */
#endif
};



	
#if 0
///
/// PionLockFreeQueue: a FIFO queue that is thread-safe, lock-free and uses fixed memory.
///                    WARNING: T::operator=() must be thread safe!
/// 
template <typename T,
	boost::uint16_t MaxSize = 50000,
	boost::uint32_t SleepMilliSec = 10 >
class PionLockFreeQueue :
	private boost::noncopyable
{
protected:

	/// make sure that the type used for CAS is at least as large as our structure
	BOOST_STATIC_ASSERT(sizeof(boost::uint32_t) >= (sizeof(boost::uint16_t) * 2));

	/// an object used to point to a QueueNode
	union QueueNodePtr {
		/// the actual data contained within the QueueNodePtr object
		struct {
			/// index for the QueueNode object that is pointed to
			boost::uint16_t		index;
			/// used to check compare and swap operations & protect against ABA problems
			boost::uint16_t		counter;
		} data;
		/// the 32-bit value of the QueueNode object, used for CAS operations
		boost::int32_t		value;
	};	

	/// an object used to wrap each item in the queue
	struct QueueNode {
		/// default constructor used for base QueueNode
		QueueNode(void) { m_next.value = 0; }
		/// the data wrapped by the node object
		T							m_data;
		/// points to the next node in the list
		volatile QueueNodePtr		m_next;
	};
	
	/**
	 * returns a reference to a QueueNode based on the index of a QueueNodePtr
	 *
	 * @param node_ptr pointer to the the QueueNode object
	 *
	 * @return QueueNode& reference to the QueueNode object
	 */
	inline QueueNode& getQueueNode(QueueNodePtr node_ptr) {
		return m_nodes[node_ptr.data.index];
	}
	
	/**
	 * changes the QueueNode pointed to using an atomic compare-and-swap operation
	 *
	 * @param cur_ptr reference to the QueueNode pointer that will be changed
	 * @param old_ptr the old (existing) pointer value, used to check cas operation
	 * @param new_index the new index position for the object pointed to
	 *
	 * @return bool if the cas operation was successful, or false if not changed
	 */
	static inline bool cas(volatile QueueNodePtr& cur_ptr, QueueNodePtr old_ptr,
		boost::uint16_t new_index)
	{
		QueueNodePtr new_ptr;
		new_ptr.data.index = new_index;
		new_ptr.data.counter = old_ptr.data.counter + 1;
		return boost::lockfree::CAS(&(cur_ptr.value), old_ptr.value, new_ptr.value);
	}
	
	/// returns the index position for a QueueNode that is available for use (may block)
	inline boost::uint16_t acquireNode(void) {
		QueueNodePtr	current_free_ptr;
		boost::uint16_t	new_free_index;
		boost::uint16_t	avail_index;

		while (true) {
			while (true) {
				// get current free_ptr value
				current_free_ptr.value = m_free_ptr.value;
				// check if current free_ptr value == 0
				if (current_free_ptr.data.index > 0)
					break;
				// sleep while MaxSize is exceeded
				boost::system_time wakeup_time = boost::get_system_time()
					+ boost::posix_time::millisec(SleepMilliSec);
				boost::thread::sleep(wakeup_time);
			}

			// prepare what will become the new free_ptr index value
			new_free_index = current_free_ptr.data.index - 1;
			
			// optimisticly get the next available node index
			avail_index = m_free_nodes[new_free_index];

			// use cas operation to update free_ptr value
			if (avail_index != 0
				&& cas(m_free_ptr, current_free_ptr, new_free_index))
			{
				m_free_nodes[new_free_index] = 0;
				break;	// cas successful - all done!
			}
		}
		
		return avail_index;
	}

	/// releases a QueueNode that is no longer in use
	inline void releaseNode(const boost::uint16_t node_index) {
		QueueNodePtr	current_free_ptr;
		boost::uint16_t	new_free_index;

		while (true) {
			// get current free_ptr value
			current_free_ptr.value = m_free_ptr.value;

			// prepare what will become the new free_ptr index value
			new_free_index = current_free_ptr.data.index + 1;

			// use cas operation to update free_ptr value
			if (m_free_nodes[current_free_ptr.data.index] == 0
				&& cas(m_free_ptr, current_free_ptr, new_free_index))
			{
				// push the available index value into the next free position
				m_free_nodes[current_free_ptr.data.index] = node_index;
				
				// all done!
				break;
			}
		}
	}
	

public:

	/// virtual destructor
	virtual ~PionLockFreeQueue() {}

	/// constructs a new PionLockFreeQueue
	PionLockFreeQueue(void)
	{
		// point head and tail to the node at index 1 (0 is reserved for NULL)
		m_head_ptr.data.index = m_tail_ptr.data.index = 1;
		m_head_ptr.data.counter = m_tail_ptr.data.counter = 0;
		// initialize free_ptr to zero
		m_free_ptr.value = 0;
		// initialize free_nodes to zero
		for (boost::uint16_t n = 0; n < MaxSize; ++n)
			m_free_nodes[n] = 0;
		// initialize next values to zero
		for (boost::uint16_t n = 0; n < MaxSize+2; ++n)
			m_nodes[n].m_next.value = 0;
		// push everything but the first two nodes into the available stack
		for (boost::uint16_t n = 2; n < MaxSize+2; ++n)
			releaseNode(n);
	}
	
	/// returns true if the queue is empty; false if it is not
	inline bool empty(void) const { return m_free_ptr.data.index == 0; }

	/// returns the number of items that are currently in the queue
	inline boost::uint16_t size(void) const { return m_free_ptr.data.index; }
	
	/**
	 * pushes a new item into the back of the queue
	 *
	 * @param t the item to add to the back of the queue
	 */
	inline void push(const T& t) {
		// retrieve a new list node for the queue item
		const boost::uint16_t node_index(acquireNode());

		// prepare it to be added to the list
		QueueNode& node_ref = m_nodes[node_index];
		node_ref.m_data = t;
		node_ref.m_next.data.index = 0;

		// append node to the end of the list
		QueueNodePtr tail_ptr;
		QueueNodePtr next_ptr;
		while (true) {
			tail_ptr.value = m_tail_ptr.value;
			next_ptr.value = getQueueNode(tail_ptr).m_next.value;
			// make sure that the tail pointer has not changed since reading next
			if (tail_ptr.value == m_tail_ptr.value) {
				// check if tail was pointing to the last node
				if (next_ptr.data.index == 0) {
					// try to link the new node at the end of the list
					if (cas(getQueueNode(tail_ptr).m_next, next_ptr, node_index))
						break;
				} else {
					// try to swing tail to the next node
					cas(m_tail_ptr, tail_ptr, next_ptr.data.index);
				}
			}
		}
		
		// done with enqueue; try to swing tail to the inserted node
		cas(m_tail_ptr, tail_ptr, node_index);
	}
	
	/**
	 * pops the next item from the top of the queue
	 *
	 * @param t assigned to the item at the top of the queue, if it is not empty
	 *
	 * @return true if an item was retrieved, false if the queue is empty
	 */
	inline bool pop(T& t) {
		QueueNodePtr head_ptr;
		QueueNodePtr tail_ptr;
		QueueNodePtr next_ptr;

		while (true) {
			// read current pointer values
			head_ptr.value = m_head_ptr.value;
			tail_ptr.value = m_tail_ptr.value;
			next_ptr.value = getQueueNode(head_ptr).m_next.value;
			// check consistency
			if (head_ptr.value == m_head_ptr.value) {
				// check if queue is empty, or tail is falling behind
				if (head_ptr.data.index == tail_ptr.data.index) {
					// is queue empty?
					if (next_ptr.data.index == 0)
						return false;
					// not empty; try to advance tail to catch it up
					cas(m_tail_ptr, tail_ptr, next_ptr.data.index);
				} else {
					// tail is OK
					// read value before CAS, otherwise another dequeue might
					// free the next node
					t = getQueueNode(next_ptr).m_data;
					// try to swing head to the next node
					if (cas(m_head_ptr, head_ptr, next_ptr.data.index))
						break;	// success -> exit loop
				}
			}
		}

		// item successfully retrieved
		releaseNode(const_cast<boost::uint16_t&>(head_ptr.data.index));
		return true;
	}


private:
	
	/// lookup table that maps uint16 index numbers to QueueNode pointers
	boost::array<QueueNode, MaxSize+2>		m_nodes;
	
	/// keeps track of all the QueueNode objects that are available for use
	boost::array<volatile boost::uint16_t, MaxSize>	m_free_nodes;
	
	/// pointer to the first item in the list
	volatile QueueNodePtr					m_head_ptr;

	/// pointer to the last item in the list
	volatile QueueNodePtr					m_tail_ptr;

	/// index pointer to the next QueueNode object that is available for use
	volatile QueueNodePtr					m_free_ptr;
};
#endif

}	// end namespace pion

#endif
