// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTP_RESPONSE_READER_HEADER__
#define __PION_HTTP_RESPONSE_READER_HEADER__

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/function/function3.hpp>
#include <boost/function/function4.hpp>
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

    /// function type called by incremental handler when it's finished
    typedef boost::function0<void>  continue_handler_t;
    
    /**
     * function type called during incremental processing when not finished.
     * parameters are:
     *
     * shared pointer to HTTP object being parsed,
     * shared pointer to tcp connection being used,
     * boolean true if finished parsing headers
     * function object to be called when ready to process more
     */
    typedef boost::function4<void, http::response_ptr, tcp::connection_ptr,
        bool, continue_handler_t>   incremental_handler_t;
    
    /**
     * function type called after the HTTP message has been parsed.
     * parameters are:
     *
     * shared pointer to HTTP object being parsed,
     * shared pointer to tcp connection being used,
     * error code if a problem occured during parsing
     */
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
        create(tcp::connection_ptr& tcp_conn, const http::request& http_request,
               finished_handler_t handler)
    {
        return boost::shared_ptr<response_reader>
            (new response_reader(tcp_conn, http_request, handler));
    }

    /// defines a callback function to be used for incremental processing
    inline void set_incremental_handler(incremental_handler_t& h) { m_incremental_handler = h; set_incremental_parsing(true); }

    
protected:

    /**
     * protected constructor restricts creation of objects (use create())
     *
     * @param tcp_conn TCP connection containing a new message to parse
     * @param http_request the request we are responding to
     * @param handler function called after the message has been parsed
     */
    response_reader(tcp::connection_ptr& tcp_conn, const http::request& http_request,
                       finished_handler_t handler)
        : http::reader(false, tcp_conn), m_http_msg(new http::response(http_request)),
        m_finished(handler)
    {
        m_http_msg->set_remote_ip(tcp_conn->get_remote_ip());
        set_logger(PION_GET_LOGGER("pion.http.response_reader"));
    }
        
    /// read and process more bytes from tcp connection
    void async_read_some(void) {
        begin_timeout();
        get_connection()->async_read_some(boost::bind(&response_reader::consume_bytes,
                                                      shared_from_this(),
                                                      boost::asio::placeholders::error,
                                                      boost::asio::placeholders::bytes_transferred));
    }

    /// Reads more bytes from the TCP connection
    virtual void read_bytes(void) {
        if (m_incremental_handler) {
            // call the finished handler allowing it to incrementally process
            // and return control when finished by calling http::reader::receive()
            m_incremental_handler(m_http_msg, get_connection(), get_finished_parsing_headers(),
                                  boost::bind(&response_reader::async_read_some, shared_from_this()));
        } else {
            async_read_some();
        }
    }

    /// Called after we have finished reading/parsing the HTTP message
    virtual void finished_reading(const boost::system::error_code& ec) {
        // call the finished handler with the finished HTTP message
        if (m_finished) m_finished(m_http_msg, get_connection(), ec);
    }
    
    /// Returns a reference to the HTTP message being parsed
    virtual http::message& get_message(void) { return *m_http_msg; }

    
    /// The new HTTP message container being created
    http::response_ptr          m_http_msg;

    /// function called after the HTTP message has been parsed
    finished_handler_t          m_finished;

    /// true if we should process the HTTP message incrementally
    incremental_handler_t       m_incremental_handler;
};


/// data type for a response_reader pointer
typedef boost::shared_ptr<response_reader>   response_reader_ptr;


}   // end namespace http
}   // end namespace pion

#endif
