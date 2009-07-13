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

#include <new>
#include <boost/cstdint.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/detail/atomic_count.hpp>
#include <pion/PionConfig.hpp>
#include <pion/PionException.hpp>
#if defined(PION_HAVE_LOCKFREE) && !defined(_MSC_VER)
	#include <boost/lockfree/detail/freelist.hpp>
#endif


// NOTE: the data structures contained in this file are based upon algorithms
// published in the paper "Simple, Fast, and Practical Non-Blocking and Blocking
// Concurrent Queue Algorithms" (1996, Maged M. Michael and Michael L. Scott,
// Department of Computer Science, University of Rochester).
// See http://www.cs.rochester.edu/u/scott/papers/1996_PODC_queues.pdf


namespace pion {	// begin namespace pion


///
/// PionLockedQueue: a thread-safe, two-lock concurrent FIFO queue
/// 
template <typename T,
	boost::uint32_t MaxSize = 250000,
	boost::uint32_t SleepMilliSec = 10 >
class PionLockedQueue :
	private boost::noncopyable
{
protected:

	/// data structure used to wrap each item in the queue
	struct QueueNode {
		T		data;		//< data wrapped by the node item
		QueueNode *	next;		//< points to the next node in the queue
		boost::uint32_t	version;	//< the node item's version number
	};

	/// returns a new queue node item for use in the queue
	inline QueueNode *createNode(void) {
#if defined(PION_HAVE_LOCKFREE) && !defined(_MSC_VER)
		return new (m_free_list.allocate()) QueueNode();
#else
		return new QueueNode();
#endif
	}

	/// frees memory for an existing queue node item
	inline void destroyNode(QueueNode *node_ptr) {
#if defined(PION_HAVE_LOCKFREE) && !defined(_MSC_VER)
		node_ptr->~QueueNode();
		m_free_list.deallocate(node_ptr);
#else
		delete node_ptr;
#endif
	}
	
	/**
	 * dequeues the next item from the top of the queue
	 *
	 * @param t assigned to the item at the top of the queue, if it is not empty
	 * @param boost::uint32_t version number of the item retrieved, or head node if none
	 *
	 * @return true if an item was retrieved; false if the queue is empty
	 */
	inline bool dequeue(T& t, boost::uint32_t& version) {
		// just return if the list is empty
		boost::mutex::scoped_lock head_lock(m_head_mutex);
		QueueNode *new_head_ptr = m_head_ptr->next;
		if (! new_head_ptr) {
			version = m_head_ptr->version;
			return false;
		}

		// get a copy of the item at the head of the list
		version = new_head_ptr->version;
		t = new_head_ptr->data;

		// update the pointer to the head of the list
		QueueNode *old_head_ptr = m_head_ptr;
		m_head_ptr = new_head_ptr;
		head_lock.unlock();

		// free the QueueNode for the old head of the list
		destroyNode(old_head_ptr);

		// decrement size
		--m_size;

		// item successfully dequeued
		return true;
	}


public:

	/// data structure used to manage idle consumer threads waiting for items
	class ConsumerThread {
	public:

		/// constructor assumes thread is active/running
		ConsumerThread(void) : m_is_running(true), m_next_ptr(NULL) {}

		/// returns true while the consumer thread is active/running
		inline bool isRunning(void) const { return m_is_running; }

		/// stops the thread -- if waiting on pop() will return immediately
		inline void stop(void) { m_is_running = false; m_wakeup_event.notify_one(); }

		/// stops the thread -- if waiting on pop() will return immediately
		inline void reset(void) { m_is_running = true; m_next_ptr = NULL; }

	private:

		/// allow PionLockedQueue direct access to members
		friend class PionLockedQueue;

		volatile bool		m_is_running;		//< true while the thread is running/active
		boost::condition	m_wakeup_event;	//< triggered when a new item is available
		ConsumerThread *	m_next_ptr;		//< pointer to the next idle thread
	};


	/// constructs a new PionLockedQueue
	PionLockedQueue(void)
		: m_head_ptr(NULL), m_tail_ptr(NULL), m_idle_ptr(NULL),
		m_next_version(1), m_size(0)
	{
		// initialize with a dummy node since m_head_ptr is always 
		// pointing to the item before the head of the list
		m_head_ptr = m_tail_ptr = createNode();
		m_head_ptr->next = NULL;
		m_head_ptr->version = 0;
	}
	
	/// virtual destructor
	virtual ~PionLockedQueue() {
		clear();
		destroyNode(m_tail_ptr);
	}
	
	/// returns true if the queue is empty; false if it is not
	inline bool empty(void) const { return (m_head_ptr->next == NULL); }
	
	/// returns the number of items that are currently in the queue
	std::size_t size(void) const {
		return m_size;
	}
	
	/// clears the list by removing all remaining items
	void clear(void) {
		boost::mutex::scoped_lock tail_lock(m_tail_mutex);
		boost::mutex::scoped_lock head_lock(m_head_mutex);
		while (m_head_ptr->next) {
			m_tail_ptr = m_head_ptr;
			m_head_ptr = m_head_ptr->next;
			destroyNode(m_tail_ptr);
		}
		m_tail_ptr = m_head_ptr;
	}

	/**
	 * pushes a new item into the back of the queue
	 *
	 * @param t the item to add to the back of the queue
	 */
	void push(const T& t) {
		// sleep while MaxSize is exceeded
		if (MaxSize > 0) {
			boost::system_time wakeup_time;
			while (size() >= MaxSize) {
				wakeup_time = boost::get_system_time()
					+ boost::posix_time::millisec(SleepMilliSec);
				boost::thread::sleep(wakeup_time);
			}
		}

		// create a new list node for the queue item
		QueueNode *node_ptr = createNode();
		node_ptr->data = t;
		node_ptr->next = NULL;
		node_ptr->version = 0;

		// append node to the end of the list
		boost::mutex::scoped_lock tail_lock(m_tail_mutex);
		node_ptr->version = (m_next_version += 2);
		m_tail_ptr->next = node_ptr;

		// update the tail pointer for the new node
		m_tail_ptr = node_ptr;

		// increment size
		++m_size;

		// wake up an idle thread (if any)
		if (m_idle_ptr) {
			ConsumerThread *idle_ptr = m_idle_ptr;
			m_idle_ptr = m_idle_ptr->m_next_ptr;
			idle_ptr->m_wakeup_event.notify_one();
		}
	}
	
	/**
	 * pops the next item from the top of the queue.  this may cause the calling
	 * thread to sleep until an item is available, and will only return when an
	 * item has been successfully retrieved or when the thread is stopping
	 *
	 * @param t assigned to the item at the top of the queue
	 * @param thread_info ConsumerThread object used to manage the thread
	 *
	 * @return true if an item was retrieved, false if the queue is empty
	 */
	bool pop(T& t, ConsumerThread& thread_info) {
		boost::uint32_t last_known_version;
		while (thread_info.isRunning()) {
			// try to get the next value
			if ( dequeue(t, last_known_version) )
				return true;	// got an item

			// queue is empty
			boost::mutex::scoped_lock tail_lock(m_tail_mutex);
			if (m_tail_ptr->version == last_known_version) {
				// still empty after acquiring lock
				thread_info.m_next_ptr = m_idle_ptr;
				m_idle_ptr = & thread_info;
				// wait for an item to become available
				thread_info.m_wakeup_event.wait(tail_lock);
			}
		}
		return false;
	}

	/**
	 * pops the next item from the top of the queue
	 *
	 * @param t assigned to the item at the top of the queue, if it is not empty
	 *
	 * @return true if an item was retrieved, false if the queue is empty
	 */
	inline bool pop(T& t) { boost::uint32_t version; return dequeue(t, version); }
	

private:

#if defined(PION_HAVE_LOCKFREE) && !defined(_MSC_VER)
	/// a caching free list of queue nodes used to reduce memory operations
	boost::lockfree::caching_freelist<QueueNode>	m_free_list;
#endif
	
	/// mutex used to protect the head pointer to the first item
	boost::mutex					m_head_mutex;

	/// mutex used to protect the tail pointer to the last item
	boost::mutex					m_tail_mutex;

	/// pointer to the first item in the list
	QueueNode *						m_head_ptr;

	/// pointer to the last item in the list
	QueueNode *						m_tail_ptr;

	/// pointer to a list of idle threads waiting for work
	ConsumerThread *				m_idle_ptr;

	/// value of the next tail version number
	boost::uint32_t					m_next_version;
	
	/// used to keep track of the number of items in the queue
	boost::detail::atomic_count		m_size;
};

	
}	// end namespace pion

#endif
