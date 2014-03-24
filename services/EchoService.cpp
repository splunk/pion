// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include "EchoService.hpp"
#include <boost/bind.hpp>
#include <pion/algorithm.hpp>
#include <pion/http/response_writer.hpp>
#include <pion/user.hpp>

using namespace pion;

namespace pion {        // begin namespace pion
namespace plugins {     // begin namespace plugins

    
/// used by handle_request to write dictionary terms
void writeDictionaryTerm(http::response_writer_ptr& writer,
                         const ihash_multimap::value_type& val)
{
    // text is copied into writer text cache
    writer << val.first << http::types::HEADER_NAME_VALUE_DELIMITER
    << val.second
    << http::types::STRING_CRLF;
}


// EchoService member functions

/// handles requests for EchoService
void EchoService::operator()(http::request_ptr& http_request_ptr, tcp::connection_ptr& tcp_conn)
{
    // this web service uses static text to test the mixture of "copied" with
    // "static" (no-copy) text
    static const std::string REQUEST_ECHO_TEXT("[Request Echo]");
    static const std::string REQUEST_HEADERS_TEXT("[Request Headers]");
    static const std::string QUERY_PARAMS_TEXT("[Query Parameters]");
    static const std::string COOKIE_PARAMS_TEXT("[Cookie Parameters]");
    static const std::string POST_CONTENT_TEXT("[POST Content]");
    static const std::string USER_INFO_TEXT("[USER Info]");
    
    // Set Content-type to "text/plain" (plain ascii text)
    http::response_writer_ptr writer(http::response_writer::create(tcp_conn, *http_request_ptr,
                                                            boost::bind(&tcp::connection::finish, tcp_conn)));
    writer->get_response().set_content_type(http::types::CONTENT_TYPE_TEXT);
    
    // write request information
    writer->write_no_copy(REQUEST_ECHO_TEXT);
    writer->write_no_copy(http::types::STRING_CRLF);
    writer->write_no_copy(http::types::STRING_CRLF);
    writer
        << "Request method: "
        << http_request_ptr->get_method()
        << http::types::STRING_CRLF
        << "Resource originally requested: "
        << http_request_ptr->get_original_resource()
        << http::types::STRING_CRLF
        << "Resource delivered: "
        << http_request_ptr->get_resource()
        << http::types::STRING_CRLF
        << "Query string: "
        << http_request_ptr->get_query_string()
        << http::types::STRING_CRLF
        << "HTTP version: "
        << http_request_ptr->get_version_major() << '.' << http_request_ptr->get_version_minor()
        << http::types::STRING_CRLF
        << "Content length: "
        << (unsigned long)http_request_ptr->get_content_length()
        << http::types::STRING_CRLF
        << http::types::STRING_CRLF;
             
    // write request headers
    writer->write_no_copy(REQUEST_HEADERS_TEXT);
    writer->write_no_copy(http::types::STRING_CRLF);
    writer->write_no_copy(http::types::STRING_CRLF);
    std::for_each(http_request_ptr->get_headers().begin(), http_request_ptr->get_headers().end(),
                  boost::bind(&writeDictionaryTerm, writer, _1));
    writer->write_no_copy(http::types::STRING_CRLF);

    // write query parameters
    writer->write_no_copy(QUERY_PARAMS_TEXT);
    writer->write_no_copy(http::types::STRING_CRLF);
    writer->write_no_copy(http::types::STRING_CRLF);
    std::for_each(http_request_ptr->get_queries().begin(), http_request_ptr->get_queries().end(),
                  boost::bind(&writeDictionaryTerm, writer, _1));
    writer->write_no_copy(http::types::STRING_CRLF);
    
    // write cookie parameters
    writer->write_no_copy(COOKIE_PARAMS_TEXT);
    writer->write_no_copy(http::types::STRING_CRLF);
    writer->write_no_copy(http::types::STRING_CRLF);
    std::for_each(http_request_ptr->get_cookies().begin(), http_request_ptr->get_cookies().end(),
                  boost::bind(&writeDictionaryTerm, writer, _1));
    writer->write_no_copy(http::types::STRING_CRLF);
    
    // write POST content
    writer->write_no_copy(POST_CONTENT_TEXT);
    writer->write_no_copy(http::types::STRING_CRLF);
    writer->write_no_copy(http::types::STRING_CRLF);
    if (http_request_ptr->get_content_length() != 0) {
        writer->write(http_request_ptr->get_content(), http_request_ptr->get_content_length());
        writer->write_no_copy(http::types::STRING_CRLF);
        writer->write_no_copy(http::types::STRING_CRLF);
    }
    
    // if authenticated, write user info
    user_ptr user = http_request_ptr->get_user();
    if (user) {
        writer->write_no_copy(USER_INFO_TEXT);
        writer->write_no_copy(http::types::STRING_CRLF);
        writer->write_no_copy(http::types::STRING_CRLF);
        writer << "User authenticated, username: " << user->get_username();
        writer->write_no_copy(http::types::STRING_CRLF);
    }
    
    // send the writer
    writer->send();
}


}   // end namespace plugins
}   // end namespace pion


/// creates new EchoService objects
extern "C" PION_PLUGIN pion::plugins::EchoService *pion_create_EchoService(void)
{
    return new pion::plugins::EchoService();
}

/// destroys EchoService objects
extern "C" PION_PLUGIN void pion_destroy_EchoService(pion::plugins::EchoService *service_ptr)
{
    delete service_ptr;
}
