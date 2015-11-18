// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_ECHOSERVICE_HEADER__
#define __PION_ECHOSERVICE_HEADER__

#include <pion/http/plugin_service.hpp>


namespace pion {        // begin namespace pion
namespace plugins {     // begin namespace plugins

///
/// EchoService: web service that echos back requests (to test request parsing)
/// 
class EchoService :
    public pion::http::plugin_service
{
public:
    EchoService(void) {}
    virtual ~EchoService() {}
    virtual void operator()(const pion::http::request_ptr& http_request_ptr,
                            const pion::tcp::connection_ptr& tcp_conn);
};

}   // end namespace plugins
}   // end namespace pion

#endif
