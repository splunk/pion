// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HELLOMODULE_HEADER__
#define __PION_HELLOMODULE_HEADER__

#include <pion/net/HTTPModule.hpp>


///
/// HelloModule: module that responds with "Hello World"
/// 
class HelloModule :
	public pion::HTTPModule
{
public:
	HelloModule(void) {}
	virtual ~HelloModule() {}
	virtual bool handleRequest(pion::HTTPRequestPtr& request,
							   pion::TCPConnectionPtr& tcp_conn);
};

#endif
