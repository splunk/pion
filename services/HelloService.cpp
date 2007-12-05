// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include "HelloService.hpp"
#include <pion/net/HTTPResponseWriter.hpp>

using namespace pion;
using namespace pion::net;


// HelloService member functions

/// handles requests for HelloService
void HelloService::handleRequest(HTTPRequestPtr& request, TCPConnectionPtr& tcp_conn)
{
	static const std::string HELLO_HTML = "<html><body>Hello World!</body></html>";
	HTTPResponseWriterPtr writer(HTTPResponseWriter::create(tcp_conn, *request));
	writer->writeNoCopy(HELLO_HTML);
	writer->writeNoCopy(HTTPTypes::STRING_CRLF);
	writer->writeNoCopy(HTTPTypes::STRING_CRLF);
	writer->send();
}


/// creates new HelloService objects
extern "C" PION_SERVICE_API HelloService *pion_create_HelloService(void)
{
	return new HelloService();
}


/// destroys HelloService objects
extern "C" PION_SERVICE_API void pion_destroy_HelloService(HelloService *service_ptr)
{
	delete service_ptr;
}
