// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_ALLOW_NOTHING_SERVICE_HEADER__
#define __PION_ALLOW_NOTHING_SERVICE_HEADER__

#include <pion/net/WebService.hpp>


namespace pion {		// begin namespace pion
namespace plugins {		// begin namespace plugins
		
///
/// This class has a corresponding create function (pion_create_AllowNothingService) and
/// destroy function (pion_destroy_AllowNothingService), as required for use by PionPlugin.
/// 
class AllowNothingService : public pion::net::WebService
{
public:
	AllowNothingService(void) {}
	~AllowNothingService() {}
	virtual void operator()(pion::net::HTTPRequestPtr& request,
							pion::net::TCPConnectionPtr& tcp_conn);
};
	
}	// end namespace plugins
}	// end namespace pion

#endif
