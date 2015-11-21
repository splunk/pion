// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2015 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_STDX_THREAD_HEADER__
#define __PION_STDX_THREAD_HEADER__

#if defined(PION_HAVE_CXX11)
#include <thread>
#else
#include <boost/thread/thread.hpp>
#endif

namespace pion {    // begin namespace pion
namespace stdx {    // begin namespace stdx

#if defined(PION_HAVE_CXX11)

using ::std::thread;
namespace this_thread = ::std::this_thread;

#else

using ::boost::thread;

#endif

}   // end namespace stdx
}   // end namespace pion

#endif
