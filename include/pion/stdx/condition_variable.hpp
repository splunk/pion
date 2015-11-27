// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2015 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_STDX_CONDITION_HEADER__
#define __PION_STDX_CONDITION_HEADER__

#if defined(PION_HAVE_CXX11)
#include <condition_variable>
#else
#include <boost/thread/condition.hpp>
#endif

namespace pion {    // begin namespace pion
namespace stdx {    // begin namespace stdx

#if defined(PION_HAVE_CXX11)

using ::std::condition_variable;

#else

typedef boost::condition condition_variable;

#endif

}   // end namespace stdx
}   // end namespace pion

#endif
