// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTP_REQUEST_WRITER_HEADER__
#define __PION_HTTP_REQUEST_WRITER_HEADER__

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <pion/config.hpp>
#include <pion/http/writer.hpp>
#include <pion/http/request.hpp>


namespace pion {    // begin namespace pion
namespace http {    // begin namespace http


///
/// request_writer: used to asynchronously send HTTP requests
/// 
class request_writer :
    public http::writer,
    public boost::enable_shared_from_this<request_writer>
{
public:
    
    /// default destructor
    virtual ~request_writer() {}

    /**
     * creates new request_writer objects
     *
     * @param tcp_conn TCP connection used to send the request
     * @param handler function called after the request has been sent
     * 
     * @return boost::shared_ptr<request_writer> shared pointer to
     *         the new writer object that was created
     */
    static inline boost::shared_ptr<request_writer> create(tcp::connection_ptr& tcp_conn,
                                                              finished_handler_t handler = finished_handler_t())
    {
        return boost::shared_ptr<request_writer>(new request_writer(tcp_conn, handler));
    }
    
    /**
     * creates new request_writer objects
     * 
     * @param tcp_conn TCP connection used to send the request
     * @param http_request_ptr pointer to the request that will be sent
     * @param handler function called after the request has been sent
     * 
     * @return boost::shared_ptr<request_writer> shared pointer to
     *         the new writer object that was created
     */
    static inline boost::shared_ptr<request_writer> create(tcp::connection_ptr& tcp_conn,
                                                              http::request_ptr& http_request_ptr,
                                                              finished_handler_t handler = finished_handler_t())
    {
        return boost::shared_ptr<request_writer>(new request_writer(tcp_conn, http_request_ptr, handler));
    }

    /// returns a non-const reference to the request that will be sent
    inline http::request& get_request(void) { return *m_http_request; }
    
    
protected:
    
    /**
     * protected constructor restricts creation of objects (use create())
     * 
     * @param tcp_conn TCP connection used to send the request
     * @param handler function called after the request has been sent
     */
    request_writer(tcp::connection_ptr& tcp_conn, finished_handler_t handler)
        : http::writer(tcp_conn, handler), m_http_request(new http::request)
    {
        set_logger(PION_GET_LOGGER("pion.http.request_writer"));
    }
    
    /**
     * protected constructor restricts creation of objects (use create())
     * 
     * @param tcp_conn TCP connection used to send the request
     * @param http_request_ptr pointer to the request that will be sent
     * @param handler function called after the request has been sent
     */
    request_writer(tcp::connection_ptr& tcp_conn, http::request_ptr& http_request_ptr,
                      finished_handler_t handler)
        : http::writer(tcp_conn, handler), m_http_request(http_request_ptr)
    {
        set_logger(PION_GET_LOGGER("pion.http.request_writer"));
        // check if we should initialize the payload content using
        // the request's content buffer
        if (m_http_request->get_content_length() > 0
            && m_http_request->get_content() != NULL
            && m_http_request->get_content()[0] != '\0')
        {
            write_no_copy(m_http_request->get_content(),
                        m_http_request->get_content_length());
        }
    }

    
    /**
     * initializes a vector of write buffers with the HTTP message information
     *
     * @param write_buffers vector of write buffers to initialize
     */
    virtual void prepare_buffers_for_send(http::message::write_buffers_t& write_buffers) {
        if (get_content_length() > 0)
            m_http_request->set_content_length(get_content_length());
        m_http_request->prepare_buffers_for_send(write_buffers,
                                              get_connection()->get_keep_alive(),
                                              sending_chunked_message());
    }

    /// returns a function bound to http::writer::handle_write()
    virtual write_handler_t bind_to_write_handler(void) {
        return boost::bind(&request_writer::handle_write, shared_from_this(),
                           boost::asio::placeholders::error,
                           boost::asio::placeholders::bytes_transferred);
    }

    /**
     * called after the request is sent
     * 
     * @param write_error error status from the last write operation
     * @param bytes_written number of bytes sent by the last write operation
     */
    virtual void handle_write(const boost::system::error_code& write_error,
                             std::size_t bytes_written)
    {
        (void)bytes_written;

        logger log_ptr(get_logger());
        if (! write_error) {
            // request sent OK
            if (sending_chunked_message()) {
                PION_LOG_DEBUG(log_ptr, "Sent HTTP request chunk of " << bytes_written << " bytes");
                clear();
            } else {
                PION_LOG_DEBUG(log_ptr, "Sent HTTP request of " << bytes_written << " bytes");
            }
        }
        finished_writing(write_error);
    }


private:
    
    /// the request that will be sent
    http::request_ptr       m_http_request;
    
    /// the initial HTTP request header line
    std::string             m_request_line;
};


/// data type for a request_writer pointer
typedef boost::shared_ptr<request_writer>    request_writer_ptr;


/// override operator<< for convenience
template <typename T>
const request_writer_ptr& operator<<(const request_writer_ptr& writer, const T& data) {
    writer->write(data);
    return writer;
}


}   // end namespace http
}   // end namespace pion

#endif
