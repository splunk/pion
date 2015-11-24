// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2015 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_STDX_MUTEX_HEADER__
#define __PION_STDX_MUTEX_HEADER__

#if defined(PION_HAVE_CXX11)
#include <mutex>
#else
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#endif

namespace pion {    // begin namespace pion
namespace stdx {    // begin namespace stdx

#if defined(PION_HAVE_CXX11)

using ::std::mutex;
using ::std::lock_guard;
using ::std::unique_lock;

#else

using ::boost::mutex;
using ::boost::lock_guard;
using ::boost::unique_lock;

#endif

}   // end namespace stdx
}   // end namespace pion

#endif
