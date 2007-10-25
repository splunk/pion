// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/bind.hpp>
#include <boost/thread/xtime.hpp>
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
		m_asio_service.reset();

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

		PION_LOG_INFO(m_logger, "Shutting down thread scheduler");

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

#if defined(PION_WIN32) && defined(PION_CYGWIN_DIRECTORY)
		// pause for 1 extra second to work-around shutdown crash on Cygwin
		// which seems related to static objects used in the ASIO library
		boost::xtime stop_time;
		boost::xtime_get(&stop_time, boost::TIME_UTC);
		stop_time.sec++;
		boost::thread::sleep(stop_time);
#endif

		PION_LOG_INFO(m_logger, "Pion thread scheduler has shutdown");

	} else {
	
		// Stop the service to make sure for certain that no events are pending
		m_asio_service.stop();
		
		// Make sure that the thread pool is empty
		m_thread_pool.clear();
	}
}

void PionScheduler::run(void)
{
	boost::xtime stop_time;
	do {
		// handle I/O events managed by the service
		try {
			m_asio_service.run();
		} catch (std::exception& e) {
			PION_LOG_FATAL(m_logger, "Caught exception in pool thread: " << e.what());
		}
		if (m_is_running) {
			PION_LOG_DEBUG(m_logger, "Sleeping thread (no work available)");
			boost::xtime_get(&stop_time, boost::TIME_UTC);
			stop_time.nsec += SLEEP_WHEN_NO_WORK_NSEC;
			if (static_cast<unsigned long>(stop_time.nsec) >= NSEC_IN_SECOND) {
				stop_time.sec++;
				stop_time.nsec -= NSEC_IN_SECOND;
			}
			boost::thread::sleep(stop_time);
		}
	} while (m_is_running);
}

}	// end namespace pion
