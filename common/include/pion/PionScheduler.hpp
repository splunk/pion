// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PIONSCHEDULER_HEADER__
#define __PION_PIONSCHEDULER_HEADER__

#include <list>
#include <boost/asio.hpp>
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/detail/atomic_count.hpp>
#include <pion/PionConfig.hpp>
#include <pion/PionLogger.hpp>


namespace pion {	// begin namespace pion

///
/// PionScheduler: combines Boost.ASIO with a managed thread pool for scheduling
/// 
class PION_COMMON_API PionScheduler :
	private boost::noncopyable
{
public:

	/// constructs a new PionScheduler
	PionScheduler(void)
		: m_logger(PION_GET_LOGGER("pion.PionScheduler")),
		m_running_threads(0), m_num_threads(DEFAULT_NUM_THREADS),
		m_active_users(0), m_is_running(false)
	{}
	
	/// virtual destructor
	virtual ~PionScheduler() { shutdown(); }

	/// Starts the thread scheduler (this is called automatically when necessary)
	void startup(void);
	
	/// Stops the thread scheduler (this is called automatically when the program exits)
	void shutdown(void);

	/// the calling thread will sleep until the scheduler has stopped
	void join(void);
	
	/// registers an active user with the thread scheduler.  Shutdown of the
	/// PionScheduler is deferred until there are no more active users.  This
	/// ensures that any work queued will not reference destructed objects
	void addActiveUser(void);

	/// unregisters an active user with the thread scheduler
	void removeActiveUser(void);
	
	/// returns an async I/O service used to schedule work
	virtual boost::asio::io_service& getIOService(void) { return m_service; }

	/// returns true if the scheduler is running
	inline bool isRunning(void) const { return m_is_running; }
	
	/// sets the number of threads to be used (these are shared by all servers)
	inline void setNumThreads(const boost::uint32_t n) { m_num_threads = n; }
	
	/// returns the number of threads currently in use
	inline boost::uint32_t getNumThreads(void) const { return m_num_threads; }

	/// returns the number of threads that are currently running
	inline boost::uint32_t getRunningThreads(void) const { return m_running_threads; }

	/// sets the logger to be used
	inline void setLogger(PionLogger log_ptr) { m_logger = log_ptr; }

	/// returns the logger currently in use
	inline PionLogger getLogger(void) { return m_logger; }
	
	
private:

	/// start function for worker threads
	void run(void);


	/// typedef for a pool of worker threads
	typedef std::list<boost::shared_ptr<boost::thread> >	ThreadPool;

	/// default number of worker threads in the thread pool
	static const boost::uint32_t	DEFAULT_NUM_THREADS;

	/// number of nanoseconds in one full second (10 ^ 9)
	static const boost::uint32_t	NSEC_IN_SECOND;

	/// number of nanoseconds a thread should sleep for when there is no work
	static const boost::uint32_t	SLEEP_WHEN_NO_WORK_NSEC;


	/// mutex to make class thread-safe
	boost::mutex					m_mutex;
	
	/// primary logging interface used by this class
	PionLogger						m_logger;

	/// pool of threads used to perform work
	ThreadPool						m_thread_pool;
	
	/// service used to manage async I/O events (currently only 1 is used)
	boost::asio::io_service			m_service;
	
	/// condition triggered when there are no more active users
	boost::condition				m_no_more_active_users;

	/// condition triggered when the scheduler has stopped
	boost::condition				m_scheduler_has_stopped;

	/// number of worker threads that are currently running
	boost::detail::atomic_count		m_running_threads;
	
	/// total number of worker threads in the pool
	boost::uint32_t					m_num_threads;

	/// the scheduler will not shutdown until there are no more active users
	boost::uint32_t					m_active_users;

	/// true if the thread scheduler is running
	bool							m_is_running;
};

}	// end namespace pion

#endif
