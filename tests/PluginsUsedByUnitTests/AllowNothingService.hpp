// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_ALLOW_NOTHING_SERVICE_HEADER__
#define __PION_ALLOW_NOTHING_SERVICE_HEADER__

#include <pion/net/WebService.hpp>

///
/// This class has a corresponding create function (pion_create_AllowNothingService) and
/// destroy function (pion_destroy_AllowNothingService), as required for use by PionPlugin.
/// 
class AllowNothingService : public pion::net::WebService
{
public:
	AllowNothingService(void) {}
	~AllowNothingService() {}
	virtual void handleRequest(pion::net::HTTPRequestPtr& request,
							   pion::net::TCPConnectionPtr& tcp_conn);
};

#endif
