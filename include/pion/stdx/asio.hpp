// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2015 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_STDX_ASIO_HEADER__
#define __PION_STDX_ASIO_HEADER__

#include <pion/config.hpp>
#include <pion/stdx/functional.hpp>

#if defined(PION_HAVE_STANDALONE_ASIO)

#if !defined(PION_HAVE_CXX11)
#error "Standalone ASIO mode requires C++11"
#endif

// Don't use boost types in asio, use the ones from std::
#define ASIO_STANDALONE

// This is needed so that we have the deadline_timer
// https://github.com/chriskohlhoff/asio/issues/82
#define ASIO_HAS_BOOST_DATE_TIME

#include <asio.hpp>

#ifdef PION_HAVE_SSL
#ifdef PION_XCODE
// ignore openssl warnings if building with XCode
#pragma GCC system_header
#endif
#include <asio/ssl.hpp>
#endif

#else
#include <boost/asio.hpp>

#ifdef PION_HAVE_SSL
#ifdef PION_XCODE
// ignore openssl warnings if building with XCode
#pragma GCC system_header
#endif
#include <boost/asio/ssl.hpp>
#endif

#endif

namespace pion {    // begin namespace pion
namespace stdx {    // begin namespace stdx

#if defined(PION_HAVE_STANDALONE_ASIO)

namespace asio {

using namespace ::asio;

// In standalone mode, ASIO doesn't define placeholders.
// https://github.com/chriskohlhoff/asio/issues/83
namespace placeholders {

const decltype(stdx::placeholders::_1) error{};
const decltype(stdx::placeholders::_2) bytes_transferred{};
const decltype(stdx::placeholders::_2) iterator{};
const decltype(stdx::placeholders::_2) results{};
const decltype(stdx::placeholders::_2) endpoint{};
const decltype(stdx::placeholders::_2) signal_number{};

}   // end namespace placeholders

}   // end namespace asio

#else

namespace asio = ::boost::asio;

#endif

}   // end namespace stdx
}   // end namespace pion

#endif
