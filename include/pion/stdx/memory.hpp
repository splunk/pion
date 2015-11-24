// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2015 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_STDX_MEMORY_HEADER__
#define __PION_STDX_MEMORY_HEADER__

#if defined(PION_HAVE_CXX11)
#include <memory>
#else
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

namespace pion {    // begin namespace pion
namespace stdx {    // begin namespace stdx

#if defined(PION_HAVE_CXX11)

using ::std::shared_ptr;
using ::std::enable_shared_from_this;

#else

using ::boost::shared_ptr;
using ::boost::enable_shared_from_this;

#endif

}   // end namespace stdx
}   // end namespace pion

#endif
