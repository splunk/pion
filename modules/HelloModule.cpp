// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include "HelloModule.hpp"
#include <pion/net/HTTPResponse.hpp>

using namespace pion;
using namespace pion::net;


// HelloModule member functions

/// handles requests for HelloModule
bool HelloModule::handleRequest(HTTPRequestPtr& request, TCPConnectionPtr& tcp_conn)
{
	static const std::string HELLO_HTML = "<html><body>Hello World!</body></html>\r\n\r\n";
	HTTPResponsePtr response(HTTPResponse::create());
	response->writeNoCopy(HELLO_HTML);
	response->send(tcp_conn);
	return true;
}


/// creates new HelloModule objects
extern "C" PION_PLUGIN_API HelloModule *pion_create_HelloModule(void)
{
	return new HelloModule();
}


/// destroys HelloModule objects
extern "C" PION_PLUGIN_API void pion_destroy_HelloModule(HelloModule *module_ptr)
{
	delete module_ptr;
}
