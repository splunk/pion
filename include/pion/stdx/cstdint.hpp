// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2015 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_STDX_CSTDINT_HEADER__
#define __PION_STDX_CSTDINT_HEADER__

#if defined(PION_HAVE_CXX11)
#include <cstdint>
#else
#include <boost/cstdint.hpp>
#endif

namespace pion {    // begin namespace pion
namespace stdx {    // begin namespace stdx

#if defined(PION_HAVE_CXX11)

using ::std::int8_t;
using ::std::int16_t;
using ::std::int32_t;
using ::std::int64_t;
using ::std::uint8_t;
using ::std::uint16_t;
using ::std::uint32_t;
using ::std::uint64_t;

#else

using ::boost::int8_t;
using ::boost::int16_t;
using ::boost::int32_t;
using ::boost::int64_t;
using ::boost::uint8_t;
using ::boost::uint16_t;
using ::boost::uint32_t;
using ::boost::uint64_t;

#endif

}   // end namespace stdx
}   // end namespace pion

#endif
