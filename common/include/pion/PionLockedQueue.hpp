// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PIONLOCKEDQUEUE_HEADER__
#define __PION_PIONLOCKEDQUEUE_HEADER__

#include <boost/cstdint.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/detail/atomic_count.hpp>
#include <pion/PionConfig.hpp>
#include <pion/PionScheduler.hpp>


namespace pion {	// begin namespace pion


///
/// PionLockedQueue: a thread-safe, two-lock concurrent FIFO queue.  Based on the paper "Simple,
/// Fast, and Practical Non-Blocking and Blocking Concurrent Queue Algorithms" by Maged M. Michael
/// and Michael L. Scott (1996, Department of Computer Science, University of Rochester).
/// See http://www.cs.rochester.edu/u/scott/papers/1996_PODC_queues.pdf
/// 
template <typename T, boost::uint32_t MaxSize = 10000, boost::uint32_t SleepNanoSec = 10000000>
class PionLockedQueue :
	private boost::noncopyable
{
public:

	/// constructs a new PionLockedQueue
	PionLockedQueue(void)
		: m_head_ptr(NULL), m_tail_ptr(NULL), m_size(0)
	{
		// initialize with a dummy node since m_head_ptr is always 
		// pointing to the item before the head of the list
		m_head_ptr = m_tail_ptr = new QueueNode();
	}
	
	/// virtual destructor
	virtual ~PionLockedQueue() { clear(); }
	
	/// returns the number of items that are currently in the queue
	inline boost::uint32_t size(void) const { return m_size; }
	
	/// returns true if the queue is empty; false if it is not
	inline bool is_empty(void) const {
		boost::mutex::scoped_lock head_lock(m_head_mutex);
		return (m_head_ptr->next() != NULL);
	}
	
	/// clears the list by removing all remaining items
	inline void clear(void) {
		boost::mutex::scoped_lock tail_lock(m_tail_mutex);
		boost::mutex::scoped_lock head_lock(m_head_mutex);
		while (m_head_ptr->next()) {
			m_tail_ptr = m_head_ptr;
			m_head_ptr = m_head_ptr->next();
			delete m_tail_ptr;
		}
		m_tail_ptr = m_head_ptr;
	}

	/**
	 * pushes a new item into the back of the queue
	 *
	 * @param t the item to add to the back of the queue
	 */
	inline void push(T& t) {
		// sleep MaxSize is exceeded
		if (MaxSize > 0) {
			while (size() >= MaxSize)
				PionScheduler::sleep(0, SleepNanoSec);
		}
		// create a new list node for the queue item
		QueueNode *node_ptr(new QueueNode(t));
		boost::mutex::scoped_lock tail_lock(m_tail_mutex);
		// append node to the end of the list
		m_tail_ptr->next(node_ptr);
		// update the tail pointer for the new node
		m_tail_ptr = node_ptr;
		tail_lock.unlock();
		++m_size;
	}
	
	/**
	 * pops the next item from the top of the queue
	 *
	 * @param t assigned to the item at the top of the queue, if it is not empty
	 *
	 * @return true if an item was retrieved, false if the queue is empty
	 */
	inline bool pop(T& t) {
		boost::mutex::scoped_lock head_lock(m_head_mutex);
		// throw if the list is empty
		if (m_head_ptr->next() == NULL)
			return false;
		// get a copy of the item at the head of the list
		t = m_head_ptr->next()->data();
		// update the pointer to the head of the list
		QueueNode *old_head_ptr(m_head_ptr);
		m_head_ptr = m_head_ptr->next();
		head_lock.unlock();
		// free the QueueNode for the old head of the list
		delete old_head_ptr;
		// item successfully retrieved
		--m_size;
		return true;
	}
	

private:
	
	///
	/// QueueNode: an object used to wrap each item in the queue
	/// 
	class QueueNode :
		private boost::noncopyable
	{
	public:
	
		/// default constructor used for base QueueNode
		QueueNode(void) : m_next(NULL) {}
		
		/// constructs a QueueNode object
		QueueNode(T& t) : m_data(t), m_next(NULL) {}
		
		/// sets the next pointer for the node
		inline void next(QueueNode *ptr) { m_next = ptr; }
		
		/// returns a pointer to the next node in the list
		inline QueueNode *next(void) const { return m_next; }

		/// returns a reference to the data wrapped by the node
		inline const T& data(void) const { return m_data; }

	private:
	
		/// the data wrapped by the node object
		T				m_data;
		
		/// points to the next node in the list
		QueueNode *		m_next;
	};

	
	/// mutex used to protect the head pointer to the first item
	boost::mutex					m_head_mutex;

	/// mutex used to protect the tail pointer to the last item
	boost::mutex					m_tail_mutex;
	
	/// pointer to the first item in the list
	QueueNode *						m_head_ptr;

	/// pointer to the last item in the list
	QueueNode *						m_tail_ptr;
	
	/// used to keep track of the number of items in the queue
	boost::detail::atomic_count		m_size;
};

	
}	// end namespace pion

#endif
