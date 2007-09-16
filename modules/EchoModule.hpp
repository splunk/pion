// -----------------------------------------------------------------
// libpion: a C++ framework for building lightweight HTTP interfaces
// -----------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_ECHOMODULE_HEADER__
#define __PION_ECHOMODULE_HEADER__

#include <libpion/HTTPModule.hpp>


///
/// EchoModule: module that echos back requests (to test request parsing)
/// 
class EchoModule :
	public pion::HTTPModule
{
public:
	EchoModule(void) {}
	virtual ~EchoModule() {}
	virtual bool handleRequest(pion::HTTPRequestPtr& request,
							   pion::TCPConnectionPtr& tcp_conn);
};

#endif
