// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_SHUTDOWNMANAGER_HEADER__
#define __PION_SHUTDOWNMANAGER_HEADER__

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#ifndef PION_WIN32
	#include <signal.h>
#endif


///
/// ShutdownManager: used to manage shutdown for the main thread
///
class ShutdownManager {
public:
	// default constructor & destructor
	ShutdownManager(void) : m_shutdown_now(false) {}
	~ShutdownManager() {}

	/// signals the shutdown condition
	inline void shutdown(void) {
		boost::mutex::scoped_lock shutdown_lock(m_shutdown_mutex);
		m_shutdown_now = true;
		m_shutdown_cond.notify_all();
	}
	
	/// blocks until the shutdown condition has been signaled
	inline void wait(void) {
		boost::mutex::scoped_lock shutdown_lock(m_shutdown_mutex);
		while (! m_shutdown_now)
			m_shutdown_cond.wait(shutdown_lock);
	}
	
private:
	/// true if we should shutdown now
	bool					m_shutdown_now;
	
	/// used to protect the shutdown condition
	boost::mutex			m_shutdown_mutex;
	
	/// triggered when it is time to shutdown
	boost::condition		m_shutdown_cond;
};

/// static shutdown manager instance used to control shutdown of main()
static ShutdownManager	main_shutdown_manager;


/// signal handlers that trigger the shutdown manager
#ifdef PION_WIN32
BOOL WINAPI console_ctrl_handler(DWORD ctrl_type)
{
	switch(ctrl_type) {
		case CTRL_C_EVENT:
		case CTRL_BREAK_EVENT:
		case CTRL_CLOSE_EVENT:
		case CTRL_SHUTDOWN_EVENT:
			main_shutdown_manager.shutdown();
			return TRUE;
		default:
			return FALSE;
	}
}
#else
void handle_signal(int sig)
{
	main_shutdown_manager.shutdown();
}
#endif


#endif
