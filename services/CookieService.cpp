// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include "CookieService.hpp"
#include <pion/algorithm.hpp>
#include <pion/http/response_writer.hpp>

using namespace pion;

namespace pion {        // begin namespace pion
namespace plugins {     // begin namespace plugins

    
// CookieService member functions

/// handles requests for CookieService
void CookieService::operator()(http::request_ptr& http_request_ptr, tcp::connection_ptr& tcp_conn)
{
    static const std::string HEADER_HTML = "<html>\n<head>\n<title>Cookie Service</title>\n"
        "</head>\n<body>\n\n<h1>Cookie Service</h1>\n";
    static const std::string FOOTER_HTML = "\n</body>\n</html>\n";

    // Set Content-type for HTML and write the header
    http::response_writer_ptr writer(http::response_writer::create(tcp_conn, *http_request_ptr,
                                                            boost::bind(&tcp::connection::finish, tcp_conn)));
    writer->getResponse().setContentType(http::types::CONTENT_TYPE_HTML);
    writer->writeNoCopy(HEADER_HTML);

    // Check if we have an action to perform
    if (http_request_ptr->hasQuery("action")) {
        if (algorithm::url_decode(http_request_ptr->getQuery("action")) == "Add Cookie") {
            // add a new cookie
            const std::string cookie_name(http_request_ptr->getQuery("cookie_name"));
            const std::string cookie_value(http_request_ptr->getQuery("cookie_value"));
            if (cookie_name.empty() || cookie_value.empty()) {
                writer << "\n<p>[Error: You must specify a name and value to add a cookie]</p>\n\n";
            } else {
                writer->getResponse().setCookie(cookie_name, cookie_value);
                writer << "\n<p>[Added cookie "
                    << cookie_name << '=' << cookie_value << "]</p>\n\n";
            }
        } else if (http_request_ptr->getQuery("action") == "delete") {
            const std::string cookie_name(http_request_ptr->getQuery("cookie_name"));
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
    if (http_request_ptr->hasHeader(http::types::HEADER_COOKIE)) {
        writer << "\n<h2>Cookie Headers</h2>\n<ul>\n";
        std::pair<ihash_multimap::const_iterator, ihash_multimap::const_iterator>
            header_pair = http_request_ptr->get_headers().equal_range(http::types::HEADER_COOKIE);
        for (ihash_multimap::const_iterator header_iterator = header_pair.first;
             header_iterator != http_request_ptr->get_headers().end()
             && header_iterator != header_pair.second; ++header_iterator)
        {
            writer << "<li>Cookie: " << header_iterator->second << "\n";
        }
        writer << "</ul>\n\n";
    } else {
        writer << "\n<h2>No Cookie Headers</h2>\n\n";
    }
    
    // display existing cookies
    ihash_multimap& cookie_params = http_request_ptr->get_cookies();
    if (! cookie_params.empty()) {
        writer << "\n<h2>Cookie Variables</h2>\n<ul>\n";
        for (ihash_multimap::const_iterator i = cookie_params.begin();
             i != cookie_params.end(); ++i)
        {
            writer << "<li>" << i->first << ": " << i->second
                << " <a href=\"" << http_request_ptr->getResource()
                << "?action=delete&cookie_name=" << i->first
                << "\">[Delete]</a>\n";
        }
        writer << "</ul>\n\n";
    } else {
        writer << "\n<h2>No Cookie Variables</h2>\n\n";
    }

    // display form to add a cookie
    writer << "\n<h2>Add Cookie</h2>\n"
        "<p><form action=\"" << http_request_ptr->getResource() << "\" method=\"POST\">\n"
        "Name: <input type=\"text\" name=\"cookie_name\"><br />\n"
        "Value: <input type=\"text\" name=\"cookie_value\"><br />\n"
        "<input type=\"submit\" name=\"action\" value=\"Add Cookie\"></p>\n"
        "</form>\n\n";
    
    // write the footer
    writer->writeNoCopy(FOOTER_HTML);
    
    // send the writer
    writer->send();
}


}   // end namespace plugins
}   // end namespace pion


/// creates new CookieService objects
extern "C" PION_API pion::plugins::CookieService *pion_create_CookieService(void)
{
    return new pion::plugins::CookieService();
}

/// destroys CookieService objects
extern "C" PION_API void pion_destroy_CookieService(pion::plugins::CookieService *service_ptr)
{
    delete service_ptr;
}
