// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTP_RESPONSE_WRITER_HEADER__
#define __PION_HTTP_RESPONSE_WRITER_HEADER__

#include <memory>
#include <asio.hpp>
#include <boost/noncopyable.hpp>
#include <pion/config.hpp>
#include <pion/http/writer.hpp>
#include <pion/http/request.hpp>
#include <pion/http/response.hpp>


namespace pion {    // begin namespace pion
namespace http {    // begin namespace http


///
/// response_writer: used to asynchronously send HTTP responses
/// 
class PION_API response_writer :
    public http::writer,
    public std::enable_shared_from_this<response_writer>
{
public:
    
    /// default destructor
    virtual ~response_writer() {}

    /**
     * creates new response_writer objects
     * 
     * @param tcp_conn TCP connection used to send the response
     * @param http_response pointer to the response that will be sent
     * @param handler function called after the response has been sent
     * 
     * @return std::shared_ptr<response_writer> shared pointer to
     *         the new writer object that was created
     */
    static inline std::shared_ptr<response_writer> create(const tcp::connection_ptr& tcp_conn,
                                                            const http::response_ptr& http_response_ptr,
                                                            finished_handler_t handler = finished_handler_t())
    {
        return std::shared_ptr<response_writer>(new response_writer(tcp_conn, http_response_ptr, handler));
    }

    /**
     * creates new response_writer objects
     * 
     * @param tcp_conn TCP connection used to send the response
     * @param http_request the request we are responding to
     * @param handler function called after the request has been sent
     * 
     * @return std::shared_ptr<response_writer> shared pointer to
     *         the new writer object that was created
     */
    static inline std::shared_ptr<response_writer> create(const tcp::connection_ptr& tcp_conn,
                                                            const http::request& http_request,
                                                            finished_handler_t handler = finished_handler_t())
    {
        return std::shared_ptr<response_writer>(new response_writer(tcp_conn, http_request, handler));
    }
    
    /// returns a non-const reference to the response that will be sent
    inline http::response& get_response(void) { return *m_http_response; }
    
    
protected:
    
    /**
     * protected constructor restricts creation of objects (use create())
     * 
     * @param tcp_conn TCP connection used to send the response
     * @param http_response pointer to the response that will be sent
     * @param handler function called after the request has been sent
     */
    response_writer(const tcp::connection_ptr& tcp_conn, const http::response_ptr& http_response_ptr,
                    finished_handler_t handler)
        : http::writer(tcp_conn, handler), m_http_response(http_response_ptr)
    {
        set_logger(PION_GET_LOGGER("pion.http.response_writer"));
        // tell the http::writer base class whether or not the client supports chunks
        supports_chunked_messages(m_http_response->get_chunks_supported());
        // check if we should initialize the payload content using
        // the response's content buffer
        if (m_http_response->get_content_length() > 0
            && m_http_response->get_content() != NULL
            && m_http_response->get_content()[0] != '\0')
        {
            write_no_copy(m_http_response->get_content(), m_http_response->get_content_length());
        }
    }
    
    /**
     * protected constructor restricts creation of objects (use create())
     * 
     * @param tcp_conn TCP connection used to send the response
     * @param http_request the request we are responding to
     * @param handler function called after the request has been sent
     */
    response_writer(const tcp::connection_ptr& tcp_conn, const http::request& http_request,
                       finished_handler_t handler)
        : http::writer(tcp_conn, handler), m_http_response(new http::response(http_request))
    {
        set_logger(PION_GET_LOGGER("pion.http.response_writer"));
        // tell the http::writer base class whether or not the client supports chunks
        supports_chunked_messages(m_http_response->get_chunks_supported());
    }
    
    
    /**
     * initializes a vector of write buffers with the HTTP message information
     *
     * @param write_buffers vector of write buffers to initialize
     */
    virtual void prepare_buffers_for_send(http::message::write_buffers_t& write_buffers) {
        if (get_content_length() > 0)
            m_http_response->set_content_length(get_content_length());
        m_http_response->prepare_buffers_for_send(write_buffers,
                                               get_connection()->get_keep_alive(),
                                               sending_chunked_message());
    }   

    /// returns a function bound to http::writer::handle_write()
    virtual write_handler_t bind_to_write_handler(void) {
        return std::bind(&response_writer::handle_write, shared_from_this(),
                           std::placeholders::_1,
                           std::placeholders::_2);
    }

    /**
     * called after the response is sent
     * 
     * @param write_error error status from the last write operation
     * @param bytes_written number of bytes sent by the last write operation
     */
    virtual void handle_write(const asio::error_code& write_error,
                             std::size_t bytes_written)
    {
        (void)bytes_written;

        logger log_ptr(get_logger());
        if (!write_error) {
            // response sent OK
            if (sending_chunked_message()) {
                PION_LOG_DEBUG(log_ptr, "Sent HTTP response chunk of " << bytes_written << " bytes");
            } else {
                PION_LOG_DEBUG(log_ptr, "Sent HTTP response of " << bytes_written << " bytes ("
                               << (get_connection()->get_keep_alive() ? "keeping alive)" : "closing)"));
            }
        }
        finished_writing(write_error);
    }

    
private:
    
    /// the response that will be sent
    http::response_ptr      m_http_response;
    
    /// the initial HTTP response header line
    std::string             m_response_line;
};


/// data type for a response_writer pointer
typedef std::shared_ptr<response_writer>   response_writer_ptr;


/// override operator<< for convenience
template <typename T>
const response_writer_ptr& operator<<(const response_writer_ptr& writer, const T& data) {
    writer->write(data);
    return writer;
}

inline response_writer_ptr& operator<<(response_writer_ptr& writer, std::ostream& (*iomanip)(std::ostream&)) {
    writer->write(iomanip);
    return writer;
}

}   // end namespace http
}   // end namespace pion

#endif
