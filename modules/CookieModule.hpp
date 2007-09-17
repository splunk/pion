// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_COOKIEMODULE_HEADER__
#define __PION_COOKIEMODULE_HEADER__

#include <pion/net/HTTPModule.hpp>


///
/// CookieModule: module that displays and updates cookies
/// 
class CookieModule :
	public pion::net::HTTPModule
{
public:
	CookieModule(void) {}
	virtual ~CookieModule() {}
	virtual bool handleRequest(pion::net::HTTPRequestPtr& request,
							   pion::net::TCPConnectionPtr& tcp_conn);
};

#endif
