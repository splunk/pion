// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PIONNET_HEADER__
#define __PION_PIONNET_HEADER__

#include <pion/PionConfig.hpp>
#include <pion/PionPlugin.hpp>
#include <pion/net/PionNetEngine.hpp>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)

///
/// PionNet: library wrapper for the PionNetEngine singleton
/// 
struct PionNet {

	/**
	 * Adds a new TCP server
	 * 
	 * @param tcp_server the TCP server to add
	 * 
     * @return true if the server was added; false if a conflict occurred
	 */
	inline static bool addServer(TCPServerPtr tcp_server) {
		return PionNetEngine::getInstance().addServer(tcp_server);
	}
	
	/**
	 * Adds a new HTTP server
	 * 
	 * @param tcp_port the TCP port the server listens to
	 * 
     * @return pointer to the new server (pointer is undefined if failure)
	 */
	inline static HTTPServerPtr addHTTPServer(const unsigned int tcp_port) {
		return PionNetEngine::getInstance().addHTTPServer(tcp_port);
	}
	
	/**
	 * Retrieves an existing TCP server for the given port number
	 * 
	 * @param tcp_port the TCP port the server listens to
	 * 
     * @return pointer to the new server (pointer is undefined if failure)
	 */
	inline static TCPServerPtr getServer(const unsigned int tcp_port) {
		return PionNetEngine::getInstance().getServer(tcp_port);
	}
	
	/// Should be called once during startup, after all servers have been registered
	inline static void startup(void) {
		PionNetEngine::getInstance().startup();
	}
	
	/// Should be called once during shutdown for cleanup
	inline static void shutdown(void) {
		PionNetEngine::getInstance().shutdown();
	}

	/// the calling thread will sleep until the engine has stopped
	inline static void join(void) {
		PionNetEngine::getInstance().join();
	}
	
	/// sets the number of threads to be used (these are shared by all servers)
	inline static void setNumThreads(const unsigned int n) {
		PionNetEngine::getInstance().setNumThreads(n);
	}

	/// returns the number of threads currently in use
	inline static unsigned int getNumThreads(void) {
		return PionNetEngine::getInstance().getNumThreads();
	}

	/// sets the logger to be used
	inline static void setLogger(PionLogger log_ptr) {
		PionNetEngine::getInstance().setLogger(log_ptr);
	}

	/// returns the logger currently in use
	inline static PionLogger getLogger(void) {
		return PionNetEngine::getInstance().getLogger();
	}

	/// appends a directory to the plug-in search path
	inline static void addPluginDirectory(const std::string& dir) {
		PionPlugin::addPluginDirectory(dir);
	}
	
	/// clears all directories from the plug-in search path
	inline static void resetPluginDirectories(void) {
		PionPlugin::resetPluginDirectories();
	}
};

}	// end namespace net
}	// end namespace pion

#endif
