// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include "AllowNothingService.hpp"
#include <pion/config.hpp>
#include <pion/http/response_writer.hpp>

using namespace pion;

namespace pion {        // begin namespace pion
namespace plugins {     // begin namespace plugins

    
void AllowNothingService::operator()(http::request_ptr& http_request_ptr, tcp::connection_ptr& tcp_conn)
{
    static const std::string DENY_HTML = "<html><body>No, you can't.</body></html>";
    http::response_writer_ptr writer(http::response_writer::create(tcp_conn, *http_request_ptr,
                                                            boost::bind(&tcp::connection::finish, tcp_conn)));
    writer->get_response().set_status_code(http::types::RESPONSE_CODE_METHOD_NOT_ALLOWED);
    writer->get_response().set_status_message(http::types::RESPONSE_MESSAGE_METHOD_NOT_ALLOWED);

    // This is a legitimate header, but it crashes when it's sent.
    //writer->get_response().add_header("Allow", "");

    // Use this line to demonstrate that it's the empty header value that's causing the problem.
    writer->get_response().add_header("Allow", "GET");

    writer->write_no_copy(DENY_HTML);
    writer->write_no_copy(http::types::STRING_CRLF);
    writer->write_no_copy(http::types::STRING_CRLF);
    writer->send();
}

    
}   // end namespace plugins
}   // end namespace pion


/// creates new AllowNothingService objects
extern "C" PION_PLUGIN pion::plugins::AllowNothingService *pion_create_AllowNothingService(void)
{
    return new pion::plugins::AllowNothingService();
}

/// destroys AllowNothingService objects
extern "C" PION_PLUGIN void pion_destroy_AllowNothingService(pion::plugins::AllowNothingService *service_ptr)
{
    delete service_ptr;
}
