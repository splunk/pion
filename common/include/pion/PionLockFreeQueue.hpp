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

#include <boost/array.hpp>
#include <boost/cstdint.hpp>
#include <boost/noncopyable.hpp>
#include <pion/PionConfig.hpp>
#include <pion/PionScheduler.hpp>
#include <apr-1/apr_atomic.h>


// NOTE: the data structures contained in this file are based upon algorithms
// published in the paper "Simple, Fast, and Practical Non-Blocking and Blocking
// Concurrent Queue Algorithms" (1996, Maged M. Michael and Michael L. Scott,
// Department of Computer Science, University of Rochester).
// See http://www.cs.rochester.edu/u/scott/papers/1996_PODC_queues.pdf


namespace pion {	// begin namespace pion


///
/// PionLockFreeQueue: a FIFO queue that is thread-safe, lock-free and uses fixed memory
/// 
template <typename T, boost::uint16_t MaxSize = 50000, boost::uint32_t SleepNanoSec = 100000>
class PionLockFreeQueue :
	private boost::noncopyable
{
protected:	

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
		apr_uint32_t			value;
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
		return m_index_table[node_ptr.data.index];
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
		QueueNodePtr new_value;
		new_value.data.index = new_index;
		new_value.data.counter = old_ptr.data.counter + 1;
		return (apr_atomic_cas32(&(cur_ptr.value), new_value.value, old_ptr.value) == old_ptr.value);
	}

	/// returns the index position for a QueueNode that is available for use (may block)
	inline boost::uint16_t acquireNode(void) {
		QueueNodePtr	current_idx_ptr;
		boost::uint16_t	new_index;
		boost::uint16_t	result_index;

		while (true) {
			while (true) {
				// get current avail_index value
				current_idx_ptr.value = m_avail_index.value;
				// sleep if current avail_index value == 0
				if (current_idx_ptr.data.index > 0)
					break;
				PionScheduler::sleep(0, SleepNanoSec);
			}
			
			// prepare what will become the new avail_index value
			new_index = current_idx_ptr.value - 1;
			
			// fetch the result value optimistically
			result_index = m_nodes_avail[new_index].data.index;

			// use cas operation to update avail_index value
			if (cas(m_avail_index, current_idx_ptr, new_index))
				break;	// cas successful - all done!
		}
		
		return result_index;
	}

	/// releases a QueueNode that is no longer in use
	inline void releaseNode(const boost::uint16_t node_index) {
		QueueNodePtr	current_idx_ptr;
		QueueNodePtr	current_node_ptr;
		
		while (true) {
			while (true) {
				// get current avail_index value
				current_idx_ptr.value = m_avail_index.value;
				// sleep until current avail_index value is < MaxSize
				if (current_idx_ptr.data.index < MaxSize)
					break;
				// this should never be possible
				PION_ASSERT(false);
			};

			// get value for the next available index position
			current_node_ptr.value = m_nodes_avail[current_idx_ptr.data.index].value;

			// re-check current avail_index to avoid unnecessary cas calls
			if (current_idx_ptr.value == m_avail_index.value) {
				// use cas operation to optimistically assign the new node_index value
				if (cas(m_nodes_avail[current_idx_ptr.data.index],
					current_node_ptr, node_index))
				{
					// use cas operation to update avail_index value
					if (cas(m_avail_index, current_idx_ptr, current_idx_ptr.data.index + 1))
						break;	// both cas were successful - all done!
				}
			}
		}
	}
	

public:

	/// constructs a new PionLockFreeQueue
	PionLockFreeQueue(void)
	{
		// point head and tail to the node at index 1 (0 is reserved for NULL)
		m_head_ptr.data.index = m_tail_ptr.data.index = 1;
		m_head_ptr.data.counter = m_tail_ptr.data.counter = 0;
		// initialize avail_index to zero
		m_avail_index.value = 0;
		// initialize nodes_avil to zero
		for (boost::uint16_t n = 0; n < MaxSize; ++n)
			m_nodes_avail[n].value = 0;
		// initialize next values to zero
		for (boost::uint16_t n = 0; n < MaxSize+2; ++n)
			m_index_table[n].m_next.value = 0;
		// push everything but the first two nodes into the available stack
		for (boost::uint16_t n = 2; n < MaxSize+2; ++n)
			releaseNode(n);
	}
	
	/// virtual destructor
	virtual ~PionLockFreeQueue() {}
		
	/// returns the number of items that are currently in the queue
	inline boost::uint16_t size(void) const { return m_avail_index.data.index; }
	
	/// returns true if the queue is empty; false if it is not
	inline bool is_empty(void) const { return m_avail_index.data.index == 0; }
	
	/**
	 * pushes a new item into the back of the queue
	 *
	 * @param t the item to add to the back of the queue
	 */
	inline void push(T& t) {
		// retrieve a new list node for the queue item
		const boost::uint16_t node_index(acquireNode());

		// prepare it to be added to the list
		QueueNode& node_ref = m_index_table[node_index];
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
	boost::array<QueueNode, MaxSize+2>		m_index_table;
	
	/// keeps track of all the QueueNode objects that are available for use
	boost::array<QueueNodePtr, MaxSize>		m_nodes_avail;
	
	/// pointer to the first item in the list
	volatile QueueNodePtr					m_head_ptr;

	/// pointer to the last item in the list
	volatile QueueNodePtr					m_tail_ptr;

	/// index pointer to the next QueueNode object that is available for use
	volatile QueueNodePtr					m_avail_index;
};


}	// end namespace pion

#endif
