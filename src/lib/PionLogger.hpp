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

#ifndef __PION_PIONLOGGER_HEADER__
#define __PION_PIONLOGGER_HEADER__

#if defined(HAVE_LOG4CXX)

	// log4cxx unfortunately has lots of problems that produce warnings

	// this disables warnings
	#if defined __GNUC__
		#pragma GCC system_header
	#elif defined __SUNPRO_CC
		#pragma disable_warn
	#elif defined _MSC_VER
		#pragma warning(push, 1)
	#endif 

	#include <log4cxx/logger.h>
	#include <log4cxx/basicconfigurator.h>

	// this re-enables warnings
	#if defined __SUNPRO_CC
		#pragma enable_warn
	#elif defined _MSC_VER
		#pragma warning(pop)
	#endif 

#else

	// Logging is disabled -> add do-nothing stubs for log4cxx calls
	namespace log4cxx {
		typedef int LoggerPtr;
		namespace Logger { static LoggerPtr getLogger(char *) { return 0; } }
	}

	// use "++logger" to avoid warnings about LOG not being used
	#define LOG4CXX_DEBUG(logger, message) { if (false) ++logger; }
	#define LOG4CXX_INFO(logger, message) { if (false) ++logger; }
	#define LOG4CXX_WARN(logger, message) { if (false) ++logger; }
	#define LOG4CXX_FATAL(logger, message) { if (false) ++logger; }

#endif

#endif
