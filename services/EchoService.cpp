// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include "EchoService.hpp"
#include <boost/bind.hpp>
#include <pion/algorithm.hpp>
#include <pion/http/response_writer.hpp>
#include <pion/http/user.hpp>

using namespace pion;
using namespace pion::net;

namespace pion {        // begin namespace pion
namespace plugins {     // begin namespace plugins

    
/// used by handleRequest to write dictionary terms
void writeDictionaryTerm(HTTPResponseWriterPtr& writer,
                         const HTTPTypes::QueryParams::value_type& val,
                         const bool decode)
{
    // text is copied into writer text cache
    writer << val.first << HTTPTypes::HEADER_NAME_VALUE_DELIMITER
    << (decode ? algo::url_decode(val.second) : val.second)
    << HTTPTypes::STRING_CRLF;
}


// EchoService member functions

/// handles requests for EchoService
void EchoService::operator()(HTTPRequestPtr& request, TCPConnectionPtr& tcp_conn)
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
    HTTPResponseWriterPtr writer(HTTPResponseWriter::create(tcp_conn, *request,
                                                            boost::bind(&TCPConnection::finish, tcp_conn)));
    writer->getResponse().setContentType(HTTPTypes::CONTENT_TYPE_TEXT);
    
    // write request information
    writer->writeNoCopy(REQUEST_ECHO_TEXT);
    writer->writeNoCopy(HTTPTypes::STRING_CRLF);
    writer->writeNoCopy(HTTPTypes::STRING_CRLF);
    writer
        << "Request method: "
        << request->getMethod()
        << HTTPTypes::STRING_CRLF
        << "Resource originally requested: "
        << request->getOriginalResource()
        << HTTPTypes::STRING_CRLF
        << "Resource delivered: "
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
    writer->writeNoCopy(REQUEST_HEADERS_TEXT);
    writer->writeNoCopy(HTTPTypes::STRING_CRLF);
    writer->writeNoCopy(HTTPTypes::STRING_CRLF);
    std::for_each(request->getHeaders().begin(), request->getHeaders().end(),
                  boost::bind(&writeDictionaryTerm, writer, _1, false));
    writer->writeNoCopy(HTTPTypes::STRING_CRLF);

    // write query parameters
    writer->writeNoCopy(QUERY_PARAMS_TEXT);
    writer->writeNoCopy(HTTPTypes::STRING_CRLF);
    writer->writeNoCopy(HTTPTypes::STRING_CRLF);
    std::for_each(request->getQueryParams().begin(), request->getQueryParams().end(),
                  boost::bind(&writeDictionaryTerm, writer, _1, true));
    writer->writeNoCopy(HTTPTypes::STRING_CRLF);
    
    // write cookie parameters
    writer->writeNoCopy(COOKIE_PARAMS_TEXT);
    writer->writeNoCopy(HTTPTypes::STRING_CRLF);
    writer->writeNoCopy(HTTPTypes::STRING_CRLF);
    std::for_each(request->getCookieParams().begin(), request->getCookieParams().end(),
                  boost::bind(&writeDictionaryTerm, writer, _1, false));
    writer->writeNoCopy(HTTPTypes::STRING_CRLF);
    
    // write POST content
    writer->writeNoCopy(POST_CONTENT_TEXT);
    writer->writeNoCopy(HTTPTypes::STRING_CRLF);
    writer->writeNoCopy(HTTPTypes::STRING_CRLF);
    if (request->getContentLength() != 0) {
        writer->write(request->getContent(), request->getContentLength());
        writer->writeNoCopy(HTTPTypes::STRING_CRLF);
        writer->writeNoCopy(HTTPTypes::STRING_CRLF);
    }
    
    // if authenticated, write user info
    PionUserPtr user = request->getUser();
    if (user) {
        writer->writeNoCopy(USER_INFO_TEXT);
        writer->writeNoCopy(HTTPTypes::STRING_CRLF);
        writer->writeNoCopy(HTTPTypes::STRING_CRLF);
        writer << "User authenticated, username: " << user->getUsername();
        writer->writeNoCopy(HTTPTypes::STRING_CRLF);
    }
    
    // send the writer
    writer->send();
}


}   // end namespace plugins
}   // end namespace pion


/// creates new EchoService objects
extern "C" PION_API pion::plugins::EchoService *pion_create_EchoService(void)
{
    return new pion::plugins::EchoService();
}

/// destroys EchoService objects
extern "C" PION_API void pion_destroy_EchoService(pion::plugins::EchoService *service_ptr)
{
    delete service_ptr;
}
