// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include "EchoModule.hpp"
#include <boost/bind.hpp>
#include <pion/net/HTTPResponse.hpp>

using namespace pion;
using namespace pion::net;


/// used by handleRequest to write dictionary terms
void writeDictionaryTerm(HTTPResponsePtr& response,
						 const HTTPTypes::StringDictionary::value_type& val,
						 const bool decode)
{
	// text is copied into response text cache
	response << val.first << HTTPTypes::HEADER_NAME_VALUE_DELIMITER
	<< (decode ? HTTPTypes::url_decode(val.second) : val.second)
	<< HTTPTypes::STRING_CRLF;
}


// EchoModule member functions

/// handles requests for EchoModule
bool EchoModule::handleRequest(HTTPRequestPtr& request, TCPConnectionPtr& tcp_conn)
{
	// this modules uses static text to test the mixture of "copied" with
	// "static" (no-copy) text
	static const std::string REQUEST_ECHO_TEXT("[Request Echo]");
	static const std::string REQUEST_HEADERS_TEXT("[Request Headers]");
	static const std::string QUERY_PARAMS_TEXT("[Query Parameters]");
	static const std::string COOKIE_PARAMS_TEXT("[Cookie Parameters]");
	static const std::string POST_CONTENT_TEXT("[POST Content]");
	
	// Set Content-type to "text/plain" (plain ascii text)
	HTTPResponsePtr response(HTTPResponse::create());
	response->setContentType(HTTPTypes::CONTENT_TYPE_TEXT);
	
	// write request information
	response->writeNoCopy(REQUEST_ECHO_TEXT);
	response->writeNoCopy(HTTPTypes::STRING_CRLF);
	response->writeNoCopy(HTTPTypes::STRING_CRLF);
	response
		<< "Request method: "
		<< request->getMethod()
		<< HTTPTypes::STRING_CRLF
		<< "Resource requested: "
		<< request->getResource()
		<< HTTPTypes::STRING_CRLF
		<< "Query string: "
		<< request->getQueryString()
		<< HTTPTypes::STRING_CRLF
		<< "HTTP version: "
		<< request->getVersionMajor() << '.' << request->getVersionMinor()
		<< HTTPTypes::STRING_CRLF
		<< "Content length: "
		<< (unsigned long)request->getContentLength()
		<< HTTPTypes::STRING_CRLF
		<< HTTPTypes::STRING_CRLF;
			 
	// write request headers
	response->writeNoCopy(REQUEST_HEADERS_TEXT);
	response->writeNoCopy(HTTPTypes::STRING_CRLF);
	response->writeNoCopy(HTTPTypes::STRING_CRLF);
	std::for_each(request->getHeaders().begin(), request->getHeaders().end(),
				  boost::bind(&writeDictionaryTerm, response, _1, false));
	response->writeNoCopy(HTTPTypes::STRING_CRLF);

	// write query parameters
	response->writeNoCopy(QUERY_PARAMS_TEXT);
	response->writeNoCopy(HTTPTypes::STRING_CRLF);
	response->writeNoCopy(HTTPTypes::STRING_CRLF);
	std::for_each(request->getQueryParams().begin(), request->getQueryParams().end(),
				  boost::bind(&writeDictionaryTerm, response, _1, true));
	response->writeNoCopy(HTTPTypes::STRING_CRLF);
	
	// write cookie parameters
	response->writeNoCopy(COOKIE_PARAMS_TEXT);
	response->writeNoCopy(HTTPTypes::STRING_CRLF);
	response->writeNoCopy(HTTPTypes::STRING_CRLF);
	std::for_each(request->getCookieParams().begin(), request->getCookieParams().end(),
				  boost::bind(&writeDictionaryTerm, response, _1, false));
	response->writeNoCopy(HTTPTypes::STRING_CRLF);
	
	// write POST content
	response->writeNoCopy(POST_CONTENT_TEXT);
	response->writeNoCopy(HTTPTypes::STRING_CRLF);
	response->writeNoCopy(HTTPTypes::STRING_CRLF);
	if (request->getContentLength() != 0) {
		response->write(request->getPostContent(), request->getContentLength());
		response->writeNoCopy(HTTPTypes::STRING_CRLF);
		response->writeNoCopy(HTTPTypes::STRING_CRLF);
	}
	
	// send the response
	response->send(tcp_conn);
	return true;
}


/// creates new EchoModule objects
extern "C" PION_PLUGIN_API EchoModule *pion_create_EchoModule(void)
{
	return new EchoModule();
}


/// destroys EchoModule objects
extern "C" PION_PLUGIN_API void pion_destroy_EchoModule(EchoModule *module_ptr)
{
	delete module_ptr;
}
