// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PIONSCHEDULER_HEADER__
#define __PION_PIONSCHEDULER_HEADER__

#include <list>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/once.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/detail/atomic_count.hpp>
#include <pion/PionConfig.hpp>
#include <pion/PionLogger.hpp>


namespace pion {	// begin namespace pion

///
/// PionScheduler: singleton class that manages TCP servers and threads
/// 
class PION_COMMON_API PionScheduler :
	private boost::noncopyable
{
public:

	/// public destructor: not virtual, should not be extended
	~PionScheduler() { shutdown(); }

	/**
     * return an instance of the PionScheduler singleton
	 * 
     * @return PionScheduler& instance of PionScheduler
	 */
	inline static PionScheduler& getInstance(void) {
		boost::call_once(PionScheduler::createInstance, m_instance_flag);
		return *m_instance_ptr;
	}

	/// Starts the thread scheduler (this is called automatically when necessary)
	void startup(void);
	
	/// Stops the thread scheduler (this is called automatically when the program exits)
	void shutdown(void);

	/// the calling thread will sleep until the engine has stopped
	void join(void);
	
	/// registers an active user with the thread scheduler
	void addActiveUser(void);

	/// registers an active user with the thread scheduler
	void removeActiveUser(void);
	
	/// returns the async I/O service used by the engine
	inline boost::asio::io_service& getIOService(void) { return m_asio_service; }

	/// returns true if the scheduler is running
	inline bool isRunning(void) const { return m_is_running; }
	
	/// sets the number of threads to be used (these are shared by all servers)
	inline void setNumThreads(const unsigned int n) { m_num_threads = n; }
	
	/// returns the number of threads currently in use
	inline unsigned int getNumThreads(void) const { return m_num_threads; }

	/// returns the number of threads that are currently running
	inline unsigned long getRunningThreads(void) const { return m_running_threads; }

	/// sets the logger to be used
	inline void setLogger(PionLogger log_ptr) { m_logger = log_ptr; }

	/// returns the logger currently in use
	inline PionLogger getLogger(void) { return m_logger; }
	
	
private:

	/// private constructor for singleton pattern
	PionScheduler(void)
		: m_logger(PION_GET_LOGGER("pion.PionScheduler")),
		m_is_running(false), m_num_threads(DEFAULT_NUM_THREADS),
		m_active_users(0), m_running_threads(0) {}

	/// creates the singleton instance, protected by boost::call_once
	static void createInstance(void);

	/// start function for pooled threads
	void run(void);


	/// typedef for a group of server threads
	typedef std::list<boost::shared_ptr<boost::thread> >	ThreadPool;

	/// default number of threads initialized for the thread pool
	static const unsigned int		DEFAULT_NUM_THREADS;

	/// number of nanoseconds in one full second (10 ^ 9)
	static const unsigned long		NSEC_IN_SECOND;

	/// number of nanoseconds a thread should sleep for when there is no work
	static const unsigned long		SLEEP_WHEN_NO_WORK_NSEC;


	/// points to the singleton instance after creation
	static PionScheduler *			m_instance_ptr;
	
	/// used for thread-safe singleton pattern
	static boost::once_flag			m_instance_flag;

	/// primary logging interface used by this class
	PionLogger						m_logger;

	/// pool of threads used to perform work
	ThreadPool						m_thread_pool;
	
	/// manages async I/O events
	boost::asio::io_service			m_asio_service;
	
	/// mutex to make class thread-safe
	boost::mutex					m_mutex;

	/// condition triggered when there are no more active users
	boost::condition				m_no_more_active_users;

	/// condition triggered when the scheduler has stopped
	boost::condition				m_scheduler_has_stopped;

	/// true if the thread scheduler is running
	bool							m_is_running;
	
	/// number of threads in the pool
	unsigned int					m_num_threads;

	/// the scheduler will not shutdown until there are no more active users
	unsigned int					m_active_users;

	/// number of threads that are currently running
	boost::detail::atomic_count		m_running_threads;
};

}	// end namespace pion

#endif
