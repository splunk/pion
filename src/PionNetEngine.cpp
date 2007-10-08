// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/bind.hpp>
#include <pion/net/PionNetEngine.hpp>
#ifdef PION_CYGWIN_DIRECTORY
	// for Cygwin shutdown crash work-around
	#include <boost/thread/xtime.hpp>
#endif

namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)


// static members of PionNetEngine
const unsigned int		PionNetEngine::DEFAULT_NUM_THREADS = 8;
PionNetEngine *			PionNetEngine::m_instance_ptr = NULL;
boost::once_flag		PionNetEngine::m_instance_flag = BOOST_ONCE_INIT;


// PionNetEngine member functions

void PionNetEngine::createInstance(void)
{
	static PionNetEngine pion_instance;
	m_instance_ptr = &pion_instance;
}

void PionNetEngine::startup(void)
{
	// check for errors
	if (m_is_running) throw AlreadyStartedException();
	if (m_servers.empty()) throw NoServersException();

	// lock mutex for thread safety
	boost::mutex::scoped_lock engine_lock(m_mutex);

	PION_LOG_INFO(m_logger, "Starting up");

	// schedule async tasks to listen for each server
	for (TCPServerMap::iterator i = m_servers.begin(); i!=m_servers.end(); ++i) {
		i->second->start();
	}

	// start multiple threads to handle async tasks
	for (unsigned int n = 0; n < m_num_threads; ++n) {
		boost::shared_ptr<boost::thread> new_thread(new boost::thread( boost::bind(&PionNetEngine::run, this) ));
		m_thread_pool.push_back(new_thread);
	}

	m_is_running = true;
}

void PionNetEngine::shutdown(void)
{
	// lock mutex for thread safety
	boost::mutex::scoped_lock engine_lock(m_mutex);

	if (m_is_running) {

		PION_LOG_INFO(m_logger, "Shutting down");

		// stop listening for new connections
		for (TCPServerMap::iterator i = m_servers.begin(); i!=m_servers.end(); ++i) {
			i->second->stop();
		}

		// Stop the service to make sure no more events are pending
		m_asio_service.stop();

		if (! m_thread_pool.empty()) {
			PION_LOG_DEBUG(m_logger, "Waiting for threads to shutdown");

			// wait until all threads in the pool have stopped

			// make sure we do not call join() for the current thread,
			// since this may yield "undefined behavior"
			boost::thread current_thread;
			for (PionThreadPool::iterator i = m_thread_pool.begin();
				i != m_thread_pool.end(); ++i)
			{
				if (**i != current_thread) (*i)->join();
			}

			// clear the thread pool (also deletes thread objects)
			m_thread_pool.clear();
		}

		// Reset all of the registered servers
		m_servers.clear();

#ifdef PION_CYGWIN_DIRECTORY
		// pause for 1 extra second to work-around shutdown crash on Cygwin
		// which seems related to static objects used in the ASIO library
		boost::xtime stop_time;
		boost::xtime_get(&stop_time, boost::TIME_UTC);
		stop_time.sec++;
		boost::thread::sleep(stop_time);
#endif

		PION_LOG_INFO(m_logger, "Pion has shutdown");

		m_is_running = false;
		m_engine_has_stopped.notify_all();

	} else {
	
		// Stop the service to make sure for certain that no events are pending
		m_asio_service.stop();
		
		// Make sure that the servers and thread pool is empty
		m_servers.clear();
		m_thread_pool.clear();

		// Make sure anyone waiting on shutdown gets notified
		// even if the server did not startup successfully
		m_engine_has_stopped.notify_all();
	}
}

void PionNetEngine::join(void)
{
	boost::mutex::scoped_lock engine_lock(m_mutex);
	if (m_is_running) {
		// sleep until engine_has_stopped condition is signaled
		m_engine_has_stopped.wait(engine_lock);
	}
}

void PionNetEngine::run(void)
{
	try {
		// handle I/O events managed by the service
		m_asio_service.run();
	} catch (std::exception& e) {
		PION_LOG_FATAL(m_logger, "Caught exception in pool thread: " << e.what());
	}
}

bool PionNetEngine::addServer(TCPServerPtr tcp_server)
{
	// lock mutex for thread safety
	boost::mutex::scoped_lock engine_lock(m_mutex);
	
	// attempt to insert tcp_server into the server map
	std::pair<TCPServerMap::iterator, bool> result =
		m_servers.insert( std::make_pair(tcp_server->getPort(), tcp_server) );

	return result.second;
}

HTTPServerPtr PionNetEngine::addHTTPServer(const unsigned int tcp_port)
{
	HTTPServerPtr http_server(HTTPServer::create(tcp_port));

	// lock mutex for thread safety
	boost::mutex::scoped_lock engine_lock(m_mutex);
	
	// attempt to insert http_server into the server map
	std::pair<TCPServerMap::iterator, bool> result =
		m_servers.insert( std::make_pair(tcp_port, http_server) );

	if (! result.second) http_server.reset();
	
	return http_server;
}

TCPServerPtr PionNetEngine::getServer(const unsigned int tcp_port)
{
	// lock mutex for thread safety
	boost::mutex::scoped_lock engine_lock(m_mutex);
	
	// check if a server already exists
	TCPServerMap::iterator i = m_servers.find(tcp_port);
	
	return (i==m_servers.end() ? TCPServerPtr() : i->second);
}

}	// end namespace net
}	// end namespace pion
