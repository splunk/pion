// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2015 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_STDX_SYSTEM_ERROR_HEADER__
#define __PION_STDX_SYSTEM_ERROR_HEADER__

#if defined(PION_HAVE_CXX11)
#include <system_error>
#else
#include <boost/system/error_code.hpp>
#endif

namespace pion {    // begin namespace pion
namespace stdx {    // begin namespace stdx

#if defined(PION_HAVE_CXX11)

using ::std::error_code;
using ::std::error_category;
using ::std::error_condition;
using ::std::system_error;
using ::std::system_category;
using ::std::errc;

#else

using ::boost::system::error_code;
using ::boost::system::error_category;
using ::boost::system::error_condition;
using ::boost::system::system_error;
using ::boost::system::system_category;

// Interestingly, in boost, errc is a namespace
namespace errc = ::boost::system::errc;

#endif

}   // end namespace stdx
}   // end namespace pion

#endif
