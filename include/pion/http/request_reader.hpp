// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTP_REQUEST_READER_HEADER__
#define __PION_HTTP_REQUEST_READER_HEADER__

#include <memory>
#include <functional>
#include <asio.hpp>
#include <pion/config.hpp>
#include <pion/http/request.hpp>
#include <pion/http/reader.hpp>


namespace pion {    // begin namespace pion
namespace http {    // begin namespace http


///
/// request_reader: asynchronously reads and parses HTTP requests
///
class request_reader :
    public http::reader,
    public std::enable_shared_from_this<request_reader>
{

public:

    /// function called after the HTTP message has been parsed
    typedef std::function<void(http::request_ptr, tcp::connection_ptr,
        const asio::error_code&)>   finished_handler_t;

    
    // default destructor
    virtual ~request_reader() {}
    
    /**
     * creates new request_reader objects
     *
     * @param tcp_conn TCP connection containing a new message to parse
     * @param handler function called after the message has been parsed
     */
    static inline std::shared_ptr<request_reader>
        create(const tcp::connection_ptr& tcp_conn, finished_handler_t handler)
    {
        return std::shared_ptr<request_reader>
            (new request_reader(tcp_conn, handler));
    }
    
    /// sets a function to be called after HTTP headers have been parsed
    inline void set_headers_parsed_callback(finished_handler_t& h) { m_parsed_headers = h; }
    
    
protected:

    /**
     * protected constructor restricts creation of objects (use create())
     *
     * @param tcp_conn TCP connection containing a new message to parse
     * @param handler function called after the message has been parsed
     */
    request_reader(const tcp::connection_ptr& tcp_conn, finished_handler_t handler)
        : http::reader(true, tcp_conn), m_http_msg(new http::request),
        m_finished(handler)
    {
        m_http_msg->set_remote_ip(tcp_conn->get_remote_ip());
        set_logger(PION_GET_LOGGER("pion.http.request_reader"));
    }
        
    /// Reads more bytes from the TCP connection
    virtual void read_bytes(void) {
        get_connection()->async_read_some(std::bind(static_cast<void(request_reader::*)(const asio::error_code&, std::size_t)>(&request_reader::consume_bytes),
                                                        shared_from_this(),
                                                        std::placeholders::_1,
                                                        std::placeholders::_2));
    }

    /// Called after we have finished parsing the HTTP message headers
    virtual void finished_parsing_headers(const asio::error_code& ec) {
        // call the finished headers handler with the HTTP message
        if (m_parsed_headers) m_parsed_headers(m_http_msg, get_connection(), ec);
    }
    
    /// Called after we have finished reading/parsing the HTTP message
    virtual void finished_reading(const asio::error_code& ec) {
        // call the finished handler with the finished HTTP message
        if (m_finished) m_finished(m_http_msg, get_connection(), ec);
    }
    
    /// Returns a reference to the HTTP message being parsed
    virtual http::message& get_message(void) { return *m_http_msg; }

    /// The new HTTP message container being created
    http::request_ptr              m_http_msg;

    /// function called after the HTTP message has been parsed
    finished_handler_t             m_finished;

    /// function called after the HTTP message headers have been parsed
    finished_handler_t             m_parsed_headers;
};


/// data type for a request_reader pointer
typedef std::shared_ptr<request_reader>    request_reader_ptr;


}   // end namespace http
}   // end namespace pion

#endif
