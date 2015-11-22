// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2015 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_STDX_FUNCTIONAL_HEADER__
#define __PION_STDX_FUNCTIONAL_HEADER__

#if defined(PION_HAVE_CXX11)
#include <functional>
#else
#include <boost/function.hpp>
#include <boost/bind.hpp>
#endif

namespace pion {    // begin namespace pion
namespace stdx {    // begin namespace stdx

#if defined(PION_HAVE_CXX11)

using ::std::function;
using ::std::bind;
using ::std::ref;
using ::std::cref;

namespace placeholders = ::std::placeholders;

#else

using ::boost::function;
using ::boost::bind;
using ::boost::ref;
using ::boost::cref;

namespace placeholders {
static ::boost::arg<1> _1;
static ::boost::arg<2> _2;
static ::boost::arg<3> _3;
static ::boost::arg<4> _4;
static ::boost::arg<5> _5;
static ::boost::arg<6> _6;
static ::boost::arg<7> _7;
static ::boost::arg<8> _8;
static ::boost::arg<9> _9;
}   // end namespace placeholders

#endif

}   // end namespace stdx
}   // end namespace pion

#endif
