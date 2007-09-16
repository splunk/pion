// -----------------------------------------------------------------
// libpion: a C++ framework for building lightweight HTTP interfaces
// -----------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PION_HEADER__
#define __PION_PION_HEADER__

#include <libpion/PionConfig.hpp>
#include <libpion/PionEngine.hpp>
#include <libpion/PionPlugin.hpp>


namespace pion {	// begin namespace pion

///
/// Pion: library wrapper for the PionEngine singleton
/// 
struct Pion {

	/**
	 * Adds a new TCP server
	 * 
	 * @param tcp_server the TCP server to add
	 * 
     * @return true if the server was added; false if a conflict occurred
	 */
	inline static bool addServer(TCPServerPtr tcp_server) {
		return PionEngine::getInstance().addServer(tcp_server);
	}
	
	/**
	 * Adds a new HTTP server
	 * 
	 * @param tcp_port the TCP port the server listens to
	 * 
     * @return pointer to the new server (pointer is undefined if failure)
	 */
	inline static HTTPServerPtr addHTTPServer(const unsigned int tcp_port) {
		return PionEngine::getInstance().addHTTPServer(tcp_port);
	}
	
	/**
	 * Retrieves an existing TCP server for the given port number
	 * 
	 * @param tcp_port the TCP port the server listens to
	 * 
     * @return pointer to the new server (pointer is undefined if failure)
	 */
	inline static TCPServerPtr getServer(const unsigned int tcp_port) {
		return PionEngine::getInstance().getServer(tcp_port);
	}
	
	/// Should be called once during startup, after all servers have been registered
	inline static void startup(void) {
		PionEngine::getInstance().startup();
	}
	
	/// Should be called once during shutdown for cleanup
	inline static void shutdown(void) {
		PionEngine::getInstance().shutdown();
	}

	/// the calling thread will sleep until the engine has stopped
	inline static void join(void) {
		PionEngine::getInstance().join();
	}
	
	/// sets the number of threads to be used (these are shared by all servers)
	inline static void setNumThreads(const unsigned int n) {
		PionEngine::getInstance().setNumThreads(n);
	}

	/// returns the number of threads currently in use
	inline static unsigned int getNumThreads(void) {
		return PionEngine::getInstance().getNumThreads();
	}

	/// sets the logger to be used
	inline static void setLogger(PionLogger log_ptr) {
		PionEngine::getInstance().setLogger(log_ptr);
	}

	/// returns the logger currently in use
	inline static PionLogger getLogger(void) {
		return PionEngine::getInstance().getLogger();
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

}	// end namespace pion

#endif
