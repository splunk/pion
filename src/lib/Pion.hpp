// -----------------------------------------------------------------
// libpion: a C++ framework for building lightweight HTTP interfaces
// -----------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.
// 
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//

#ifndef __PION_PION_HEADER__
#define __PION_PION_HEADER__

#include "PionEngine.hpp"


namespace pion {	// begin namespace pion

///
/// Pion: library wrapper for the PionEngine singleton
/// 
namespace Pion {

	/**
     * Creates a server for the given port number if one does not
     * already exist
	 * 
	 * @param port the port the server listens to
	 */
	inline static void addServer(const unsigned int port) {
		PionEngine::getInstance().getServer(port);
	}

	/**
     * Retrieves a server for the given port number.
     * Creates a new server if necessary
     * 
     * @param port the port the server listens to
	 * 
     * @return TCPServerPtr pointer to a server
	 */
	inline static TCPServerPtr getServer(const unsigned int port) {
		return PionEngine::getInstance().getServer(port);
	}

	/// starts pion
	inline static void start(void) {
		PionEngine::getInstance().start();
	}

	/// stops pion
	inline static void stop(void) {
		PionEngine::getInstance().stop();
	}

	/// the calling thread will sleep until the engine has stopped
	inline static void join(void) {
		PionEngine::getInstance().join();
	}
	
	// simple configuration functions
	inline static void setNumThreads(const unsigned int n) { PionEngine::getInstance().setNumThreads(n); }
	inline static unsigned int getNumThreads(void) { return PionEngine::getInstance().getNumThreads(); }
	inline static void setLogger(log4cxx::LoggerPtr log_ptr) { PionEngine::getInstance().setLogger(log_ptr); }
	inline static log4cxx::LoggerPtr getLogger(void) { return PionEngine::getInstance().getLogger(); }

}	// end namespace Pion

}	// end namespace pion

#endif
