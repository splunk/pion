// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/thread/mutex.hpp>
#include <pion/PionScheduler.hpp>

namespace pion {	// begin namespace pion


// static members of PionScheduler
const unsigned int		PionScheduler::DEFAULT_NUM_THREADS = 8;
const unsigned long		PionScheduler::NSEC_IN_SECOND = 1000000000;	// (10^9)
const unsigned long		PionScheduler::SLEEP_WHEN_NO_WORK_NSEC = NSEC_IN_SECOND / 4;
PionScheduler *			PionScheduler::m_instance_ptr = NULL;
boost::once_flag		PionScheduler::m_instance_flag = BOOST_ONCE_INIT;


// PionScheduler member functions

void PionScheduler::createInstance(void)
{
	static PionScheduler pion_instance;
	m_instance_ptr = &pion_instance;
}

void PionScheduler::startup(void)
{
	// lock mutex for thread safety
	boost::mutex::scoped_lock scheduler_lock(m_mutex);

	if (! m_is_running) {
		PION_LOG_INFO(m_logger, "Starting thread scheduler");
		m_is_running = true;

		// start multiple threads to handle async tasks
		for (unsigned int n = 0; n < m_num_threads; ++n) {
			boost::shared_ptr<boost::thread> new_thread(new boost::thread( boost::bind(&PionScheduler::run, this) ));
			m_thread_pool.push_back(new_thread);
		}
	}
}

void PionScheduler::shutdown(void)
{
	// lock mutex for thread safety
	boost::mutex::scoped_lock scheduler_lock(m_mutex);

	if (m_is_running) {

		PION_LOG_INFO(m_logger, "Shutting down the thread scheduler");
		
		while (m_active_users > 0) {
			// first, wait for any active users to exit
			PION_LOG_INFO(m_logger, "Waiting for " << m_active_users << " scheduler users to finish");
			m_no_more_active_users.wait(scheduler_lock);
		}

		// Stop the service to make sure no more events are pending
		m_asio_service.stop();

		if (! m_thread_pool.empty()) {
			PION_LOG_DEBUG(m_logger, "Waiting for threads to shutdown");

			// wait until all threads in the pool have stopped
			m_is_running = false;

			// make sure we do not call join() for the current thread,
			// since this may yield "undefined behavior"
			boost::thread current_thread;
			for (ThreadPool::iterator i = m_thread_pool.begin();
				i != m_thread_pool.end(); ++i)
			{
				if (**i != current_thread) (*i)->join();
			}

			// clear the thread pool (also deletes thread objects)
			m_thread_pool.clear();
		}

		PION_LOG_INFO(m_logger, "The thread scheduler has shutdown");
		m_scheduler_has_stopped.notify_all();

	} else {
	
		// Stop the service to make sure for certain that no events are pending
		m_asio_service.stop();
		
		// Make sure that the thread pool is empty
		m_thread_pool.clear();

		// Make sure anyone waiting on shutdown gets notified
		// even if the scheduler did not startup successfully
		m_scheduler_has_stopped.notify_all();
	}
}

void PionScheduler::join(void)
{
	boost::mutex::scoped_lock scheduler_lock(m_mutex);
	while (m_is_running) {
		// sleep until scheduler_has_stopped condition is signaled
		m_scheduler_has_stopped.wait(scheduler_lock);
	}
}

void PionScheduler::addActiveUser(void)
{
	if (!m_is_running) startup();
	boost::mutex::scoped_lock scheduler_lock(m_mutex);
	++m_active_users;
}

void PionScheduler::removeActiveUser(void)
{
	boost::mutex::scoped_lock scheduler_lock(m_mutex);
	if (--m_active_users == 0)
		m_no_more_active_users.notify_all();
}

void PionScheduler::run(void)
{
	boost::xtime wakeup_time;
	do {
		// handle I/O events managed by the service
		++m_running_threads;
		try {
			m_asio_service.run();
		} catch (std::exception& e) {
			PION_LOG_FATAL(m_logger, "Caught exception in pool thread: " << e.what());
		}
		if (--m_running_threads == static_cast<unsigned long>(0)) {
			m_asio_service.reset();
		}
		if (m_is_running) {
			boost::xtime_get(&wakeup_time, boost::TIME_UTC);
			wakeup_time.nsec += SLEEP_WHEN_NO_WORK_NSEC;
			if (static_cast<unsigned long>(wakeup_time.nsec) >= NSEC_IN_SECOND) {
				wakeup_time.sec++;
				wakeup_time.nsec -= NSEC_IN_SECOND;
			}
			boost::thread::sleep(wakeup_time);
		}
	} while (m_is_running);
}

}	// end namespace pion
