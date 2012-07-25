// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HELLOSERVICE_HEADER__
#define __PION_HELLOSERVICE_HEADER__

#include <pion/http/plugin_service.hpp>


namespace pion {        // begin namespace pion
namespace plugins {     // begin namespace plugins

///
/// HelloService: web service that responds with "Hello World"
/// 
class HelloService :
    public pion::net::WebService
{
public:
    HelloService(void) {}
    virtual ~HelloService() {}
    virtual void operator()(pion::net::HTTPRequestPtr& request,
                            pion::net::TCPConnectionPtr& tcp_conn);
};

}   // end namespace plugins
}   // end namespace pion

#endif
