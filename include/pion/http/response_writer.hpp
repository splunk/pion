// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTP_RESPONSE_WRITER_HEADER__
#define __PION_HTTP_RESPONSE_WRITER_HEADER__

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <pion/config.hpp>
#include <pion/http/writer.hpp>
#include <pion/http/request.hpp>
#include <pion/http/response.hpp>


namespace pion {    // begin namespace pion
namespace http {    // begin namespace http


///
/// HTTPResponseWriter: used to asynchronously send HTTP responses
/// 
class PION_API HTTPResponseWriter :
    public HTTPWriter,
    public boost::enable_shared_from_this<HTTPResponseWriter>
{
public:
    
    /// default destructor
    virtual ~HTTPResponseWriter() {}

    /**
     * creates new HTTPResponseWriter objects
     * 
     * @param tcp_conn TCP connection used to send the response
     * @param http_response pointer to the response that will be sent
     * @param handler function called after the response has been sent
     * 
     * @return boost::shared_ptr<HTTPResponseWriter> shared pointer to
     *         the new writer object that was created
     */
    static inline boost::shared_ptr<HTTPResponseWriter> create(tcp::connection_ptr& tcp_conn,
                                                               HTTPResponsePtr& http_response,
                                                               FinishedHandler handler = FinishedHandler())
    {
        return boost::shared_ptr<HTTPResponseWriter>(new HTTPResponseWriter(tcp_conn, http_response, handler));
    }

    /**
     * creates new HTTPResponseWriter objects
     * 
     * @param tcp_conn TCP connection used to send the response
     * @param http_request the request we are responding to
     * @param handler function called after the request has been sent
     * 
     * @return boost::shared_ptr<HTTPResponseWriter> shared pointer to
     *         the new writer object that was created
     */
    static inline boost::shared_ptr<HTTPResponseWriter> create(tcp::connection_ptr& tcp_conn,
                                                               const HTTPRequest& http_request,
                                                               FinishedHandler handler = FinishedHandler())
    {
        return boost::shared_ptr<HTTPResponseWriter>(new HTTPResponseWriter(tcp_conn, http_request, handler));
    }
    
    /// returns a non-const reference to the response that will be sent
    inline HTTPResponse& getResponse(void) { return *m_http_response; }
    
    
protected:
    
    /**
     * protected constructor restricts creation of objects (use create())
     * 
     * @param tcp_conn TCP connection used to send the response
     * @param http_response pointer to the response that will be sent
     * @param handler function called after the request has been sent
     */
    HTTPResponseWriter(tcp::connection_ptr& tcp_conn, HTTPResponsePtr& http_response,
                       FinishedHandler handler)
        : HTTPWriter(tcp_conn, handler), m_http_response(http_response)
    {
        setLogger(PION_GET_LOGGER("pion.http.HTTPResponseWriter"));
        // tell the HTTPWriter base class whether or not the client supports chunks
        supportsChunkedMessages(m_http_response->getChunksSupported());
        // check if we should initialize the payload content using
        // the response's content buffer
        if (http_response->getContentLength() > 0
            && http_response->getContent() != NULL
            && http_response->getContent()[0] != '\0')
        {
            writeNoCopy(http_response->getContent(), http_response->getContentLength());
        }
    }
    
    /**
     * protected constructor restricts creation of objects (use create())
     * 
     * @param tcp_conn TCP connection used to send the response
     * @param http_request the request we are responding to
     * @param handler function called after the request has been sent
     */
    HTTPResponseWriter(tcp::connection_ptr& tcp_conn, const HTTPRequest& http_request,
                       FinishedHandler handler)
        : HTTPWriter(tcp_conn, handler), m_http_response(new HTTPResponse(http_request))
    {
        setLogger(PION_GET_LOGGER("pion.http.HTTPResponseWriter"));
        // tell the HTTPWriter base class whether or not the client supports chunks
        supportsChunkedMessages(m_http_response->getChunksSupported());
    }
    
    
    /**
     * initializes a vector of write buffers with the HTTP message information
     *
     * @param write_buffers vector of write buffers to initialize
     */
    virtual void prepareBuffersForSend(http::message::write_buffers_t& write_buffers) {
        if (getContentLength() > 0)
            m_http_response->setContentLength(getContentLength());
        m_http_response->prepareBuffersForSend(write_buffers,
                                               get_connection()->get_keep_alive(),
                                               sendingChunkedMessage());
    }   

    /// returns a function bound to HTTPWriter::handleWrite()
    virtual WriteHandler bindToWriteHandler(void) {
        return boost::bind(&HTTPResponseWriter::handleWrite, shared_from_this(),
                           boost::asio::placeholders::error,
                           boost::asio::placeholders::bytes_transferred);
    }

    /**
     * called after the response is sent
     * 
     * @param write_error error status from the last write operation
     * @param bytes_written number of bytes sent by the last write operation
     */
    virtual void handleWrite(const boost::system::error_code& write_error,
                             std::size_t bytes_written)
    {
        logger log_ptr(getLogger());
        if (!write_error) {
            // response sent OK
            if (sendingChunkedMessage()) {
                PION_LOG_DEBUG(log_ptr, "Sent HTTP response chunk of " << bytes_written << " bytes");
            } else {
                PION_LOG_DEBUG(log_ptr, "Sent HTTP response of " << bytes_written << " bytes ("
                               << (get_connection()->get_keep_alive() ? "keeping alive)" : "closing)"));
            }
        }
        finishedWriting(write_error);
    }

    
private:
    
    /// the response that will be sent
    HTTPResponsePtr         m_http_response;
    
    /// the initial HTTP response header line
    std::string             m_response_line;
};


/// data type for a HTTPResponseWriter pointer
typedef boost::shared_ptr<HTTPResponseWriter>   HTTPResponseWriterPtr;


/// override operator<< for convenience
template <typename T>
const HTTPResponseWriterPtr& operator<<(const HTTPResponseWriterPtr& writer, const T& data) {
    writer->write(data);
    return writer;
}


}   // end namespace http
}   // end namespace pion

#endif
