// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include "CookieService.hpp"
#include <pion/net/HTTPResponseWriter.hpp>

using namespace pion;
using namespace pion::net;

namespace pion {		// begin namespace pion
namespace plugins {		// begin namespace plugins

	
// CookieService member functions

/// handles requests for CookieService
void CookieService::operator()(HTTPRequestPtr& request, TCPConnectionPtr& tcp_conn)
{
	static const std::string HEADER_HTML = "<html>\n<head>\n<title>Cookie Service</title>\n"
		"</head>\n<body>\n\n<h1>Cookie Service</h1>\n";
	static const std::string FOOTER_HTML = "\n</body>\n</html>\n";

	// Set Content-type for HTML and write the header
	HTTPResponseWriterPtr writer(HTTPResponseWriter::create(tcp_conn, *request,
															boost::bind(&TCPConnection::finish, tcp_conn)));
	writer->getResponse().setContentType(HTTPTypes::CONTENT_TYPE_HTML);
	writer->writeNoCopy(HEADER_HTML);

	// Check if we have an action to perform
	if (request->hasQuery("action")) {
		if (HTTPTypes::url_decode(request->getQuery("action")) == "Add Cookie") {
			// add a new cookie
			const std::string cookie_name(request->getQuery("cookie_name"));
			const std::string cookie_value(request->getQuery("cookie_value"));
			if (cookie_name.empty() || cookie_value.empty()) {
				writer << "\n<p>[Error: You must specify a name and value to add a cookie]</p>\n\n";
			} else {
				writer->getResponse().setCookie(cookie_name, cookie_value);
				writer << "\n<p>[Added cookie "
					<< cookie_name << '=' << cookie_value << "]</p>\n\n";
			}
		} else if (request->getQuery("action") == "delete") {
			const std::string cookie_name(request->getQuery("cookie_name"));
			if (cookie_name.empty()) {
				writer << "\n<p>[Error: You must specify a name to delete a cookie]</p>\n\n";
			} else {
				writer->getResponse().deleteCookie(cookie_name);
				writer << "\n<p>[Deleted cookie " << cookie_name << "]</p>\n\n";
			}
		} else {
			writer << "\n<p>[Error: Unrecognized action]</p>\n\n";
		}
	}
	
	// display cookie headers in request
	if (request->hasHeader(HTTPTypes::HEADER_COOKIE)) {
		writer << "\n<h2>Cookie Headers</h2>\n<ul>\n";
		std::pair<HTTPTypes::Headers::const_iterator, HTTPTypes::Headers::const_iterator>
			header_pair = request->getHeaders().equal_range(HTTPTypes::HEADER_COOKIE);
		for (HTTPTypes::Headers::const_iterator header_iterator = header_pair.first;
			 header_iterator != request->getHeaders().end()
			 && header_iterator != header_pair.second; ++header_iterator)
		{
			writer << "<li>Cookie: " << header_iterator->second << "\n";
		}
		writer << "</ul>\n\n";
	} else {
		writer << "\n<h2>No Cookie Headers</h2>\n\n";
	}
	
	// display existing cookies
	HTTPTypes::CookieParams& cookie_params = request->getCookieParams();
	if (! cookie_params.empty()) {
		writer << "\n<h2>Cookie Variables</h2>\n<ul>\n";
		for (HTTPTypes::CookieParams::const_iterator i = cookie_params.begin();
			 i != cookie_params.end(); ++i)
		{
			writer << "<li>" << i->first << ": " << i->second
				<< " <a href=\"" << request->getResource()
				<< "?action=delete&cookie_name=" << i->first
				<< "\">[Delete]</a>\n";
		}
		writer << "</ul>\n\n";
	} else {
		writer << "\n<h2>No Cookie Variables</h2>\n\n";
	}

	// display form to add a cookie
	writer << "\n<h2>Add Cookie</h2>\n"
		"<p><form action=\"" << request->getResource() << "\" method=\"POST\">\n"
		"Name: <input type=\"text\" name=\"cookie_name\"><br />\n"
		"Value: <input type=\"text\" name=\"cookie_value\"><br />\n"
		"<input type=\"submit\" name=\"action\" value=\"Add Cookie\"></p>\n"
		"</form>\n\n";
	
	// write the footer
	writer->writeNoCopy(FOOTER_HTML);
	
	// send the writer
	writer->send();
}


}	// end namespace plugins
}	// end namespace pion


/// creates new CookieService objects
extern "C" PION_SERVICE_API pion::plugins::CookieService *pion_create_CookieService(void)
{
	return new pion::plugins::CookieService();
}

/// destroys CookieService objects
extern "C" PION_SERVICE_API void pion_destroy_CookieService(pion::plugins::CookieService *service_ptr)
{
	delete service_ptr;
}
