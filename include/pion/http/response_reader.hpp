// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTP_RESPONSE_READER_HEADER__
#define __PION_HTTP_RESPONSE_READER_HEADER__

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/function/function2.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <pion/config.hpp>
#include <pion/http/response.hpp>
#include <pion/http/reader.hpp>


namespace pion {    // begin namespace pion
namespace http {    // begin namespace http


///
/// response_reader: asynchronously reads and parses HTTP responses
///
class response_reader :
    public http::reader,
    public boost::enable_shared_from_this<response_reader>
{

public:

    /// function called after the HTTP message has been parsed
    typedef boost::function3<void, http::response_ptr, tcp::connection_ptr,
        const boost::system::error_code&>   finished_handler_t;

    
    // default destructor
    virtual ~response_reader() {}
    
    /**
     * creates new response_reader objects
     *
     * @param tcp_conn TCP connection containing a new message to parse
     * @param http_request the request we are responding to
     * @param handler function called after the message has been parsed
     */
    static inline boost::shared_ptr<response_reader>
        create(const tcp::connection_ptr& tcp_conn, const http::request& http_request,
               finished_handler_t handler)
    {
        return boost::shared_ptr<response_reader>
            (new response_reader(tcp_conn, http_request, handler));
    }

    /// sets a function to be called after HTTP headers have been parsed
    inline void set_headers_parsed_callback(finished_handler_t& h) { m_parsed_headers = h; }

    
protected:

    /**
     * protected constructor restricts creation of objects (use create())
     *
     * @param tcp_conn TCP connection containing a new message to parse
     * @param http_request the request we are responding to
     * @param handler function called after the message has been parsed
     */
    response_reader(const tcp::connection_ptr& tcp_conn, const http::request& http_request,
                       finished_handler_t handler)
        : http::reader(false, tcp_conn), m_http_msg(new http::response(http_request)),
        m_finished(handler)
    {
        m_http_msg->set_remote_ip(tcp_conn->get_remote_ip());
        set_logger(PION_GET_LOGGER("pion.http.response_reader"));
    }
        
    /// Reads more bytes from the TCP connection
    virtual void read_bytes(void) {
        get_connection()->async_read_some(boost::bind(&response_reader::consume_bytes,
                                                        shared_from_this(),
                                                        boost::asio::placeholders::error,
                                                        boost::asio::placeholders::bytes_transferred));
    }

    /// Called after we have finished parsing the HTTP message headers
    virtual void finished_parsing_headers(const boost::system::error_code& ec) {
        // call the finished headers handler with the HTTP message
        if (m_parsed_headers) m_parsed_headers(m_http_msg, get_connection(), ec);
    }
    
    /// Called after we have finished reading/parsing the HTTP message
    virtual void finished_reading(const boost::system::error_code& ec) {
        // call the finished handler with the finished HTTP message
        if (m_finished) m_finished(m_http_msg, get_connection(), ec);
    }
    
    /// Returns a reference to the HTTP message being parsed
    virtual http::message& get_message(void) { return *m_http_msg; }

    
    /// The new HTTP message container being created
    http::response_ptr             m_http_msg;

    /// function called after the HTTP message has been parsed
    finished_handler_t             m_finished;

    /// function called after the HTTP message headers have been parsed
    finished_handler_t             m_parsed_headers;
};


/// data type for a response_reader pointer
typedef boost::shared_ptr<response_reader>   response_reader_ptr;


}   // end namespace http
}   // end namespace pion

#endif
