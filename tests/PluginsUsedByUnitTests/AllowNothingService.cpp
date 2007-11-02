// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include "AllowNothingService.hpp"
#include <pion/PionConfig.hpp>
#include <pion/net/HTTPResponse.hpp>

using namespace pion;
using namespace pion::net;

void AllowNothingService::handleRequest(HTTPRequestPtr& request, TCPConnectionPtr& tcp_conn)
{
	static const std::string DENY_HTML = "<html><body>No, you can't.</body></html>\r\n\r\n";
	HTTPResponsePtr response(HTTPResponse::create(request, tcp_conn));
	response->setResponseCode(HTTPTypes::RESPONSE_CODE_METHOD_NOT_ALLOWED);
	response->setResponseMessage(HTTPTypes::RESPONSE_MESSAGE_METHOD_NOT_ALLOWED);

	// This is a legitimate header, but it crashes when it's sent.
	//response->addHeader("Allow", "");

	// Use this line to demonstrate that it's the empty header value that's causing the problem.
	response->addHeader("Allow", "GET");

	response->writeNoCopy(DENY_HTML);
	response->send();
}

/// creates new AllowNothingService objects
extern "C" PION_SERVICE_API AllowNothingService *pion_create_AllowNothingService(void)
{
	return new AllowNothingService();
}

/// destroys AllowNothingService objects
extern "C" PION_SERVICE_API void pion_destroy_AllowNothingService(AllowNothingService *service_ptr)
{
	delete service_ptr;
}
