// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PIONNETENGINE_HEADER__
#define __PION_PIONNETENGINE_HEADER__

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/once.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <pion/PionConfig.hpp>
#include <pion/PionLogger.hpp>
#include <pion/net/TCPServer.hpp>
#include <pion/net/HTTPServer.hpp>
#include <exception>
#include <list>
#include <map>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)

///
/// PionNetEngine: singleton class that manages TCP servers and threads
/// 
class PION_NET_API PionNetEngine :
	private boost::noncopyable
{
public:

	/// exception thrown if start() is called after Pion is running
	class AlreadyStartedException : public std::exception {
		virtual const char* what() const throw() {
			return "Pion has already started";
		}
	};

	/// exception thrown if start() is called before any servers are defined
	class NoServersException : public std::exception {
		virtual const char* what() const throw() {
			return "Pion cannot start until servers are defined";
		}
	};


	/// public destructor: not virtual, should not be extended
	~PionNetEngine() { shutdown(); }

	/**
     * return an instance of the PionNetEngine singleton
	 * 
     * @return PionNetEngine& instance of PionNetEngine
	 */
	inline static PionNetEngine& getInstance(void) {
		boost::call_once(PionNetEngine::createInstance, m_instance_flag);
		return *m_instance_ptr;
	}

	/**
	 * Adds a new TCP server
	 * 
	 * @param tcp_server the TCP server to add
	 * 
     * @return true if the server was added; false if a conflict occurred
	 */
	bool addServer(TCPServerPtr tcp_server);

	/**
	 * Adds a new HTTP server
	 * 
	 * @param tcp_port the TCP port the server listens to
	 * 
     * @return pointer to the new server (pointer is undefined if failure)
	 */
	HTTPServerPtr addHTTPServer(const unsigned int tcp_port);
	
	/**
	 * Retrieves an existing TCP server for the given port number
	 * 
	 * @param tcp_port the TCP port the server listens to
	 * 
     * @return pointer to the new server (pointer is undefined if failure)
	 */
	TCPServerPtr getServer(const unsigned int tcp_port);

	/// Should be called once during startup, after all servers have been registered
	void startup(void);

	/// Should be called once during shutdown for cleanup
	void shutdown(void);

	/// the calling thread will sleep until the engine has stopped
	void join(void);
		
	/// sets the number of threads to be used (these are shared by all servers)
	inline void setNumThreads(const unsigned int n) { m_num_threads = n; }
	
	/// returns the number of threads currently in use
	inline unsigned int getNumThreads(void) const { return m_num_threads; }

	/// sets the logger to be used
	inline void setLogger(PionLogger log_ptr) { m_logger = log_ptr; }

	/// returns the logger currently in use
	inline PionLogger getLogger(void) { return m_logger; }
	
	/// returns the async I/O service used by the engine
	inline boost::asio::io_service& getIOService(void) { return m_asio_service; }

	
private:

	/// private constructor for singleton pattern
	PionNetEngine(void)
		: m_logger(PION_GET_LOGGER("Pion")),
		m_is_running(false), m_num_threads(DEFAULT_NUM_THREADS) {}

	/// creates the singleton instance, protected by boost::call_once
	static void createInstance(void);

	/// start function for pooled threads
	void run(void);

	
	/// typedef for a group of TCPServer objects
	typedef std::map<unsigned int, TCPServerPtr>			TCPServerMap;
	
	/// typedef for a group of server threads
	typedef std::list<boost::shared_ptr<boost::thread> >	PionThreadPool;

	/// default number of threads initialized for the thread pool
	static const unsigned int		DEFAULT_NUM_THREADS;

	/// points to the singleton instance after creation
	static PionNetEngine *			m_instance_ptr;
	
	/// used for thread-safe singleton pattern
	static boost::once_flag			m_instance_flag;

	/// primary logging interface used by this class
	PionLogger						m_logger;

	/// map of port numbers to TCPServer objects
	TCPServerMap					m_servers;
	
	/// pool of threads used to receive and process requests
	PionThreadPool					m_thread_pool;
	
	/// manages async I/O events
	boost::asio::io_service			m_asio_service;
	
	/// mutex to make class thread-safe
	boost::mutex					m_mutex;
	
	/// condition triggered when the engine has stopped
	boost::condition				m_engine_has_stopped;

	/// true if pion is running
	bool							m_is_running;

	/// number of threads in the pool
	unsigned int					m_num_threads;
};

}	// end namespace net
}	// end namespace pion

#endif
