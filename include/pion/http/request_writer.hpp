// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
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
namespace net {     // begin namespace net (Pion Network Library)

///
/// HTTPRequestWriter: used to asynchronously send HTTP requests
/// 
class HTTPRequestWriter :
    public HTTPWriter,
    public boost::enable_shared_from_this<HTTPRequestWriter>
{
public:
    
    /// default destructor
    virtual ~HTTPRequestWriter() {}

    /**
     * creates new HTTPRequestWriter objects
     *
     * @param tcp_conn TCP connection used to send the request
     * @param handler function called after the request has been sent
     * 
     * @return boost::shared_ptr<HTTPRequestWriter> shared pointer to
     *         the new writer object that was created
     */
    static inline boost::shared_ptr<HTTPRequestWriter> create(TCPConnectionPtr& tcp_conn,
                                                              FinishedHandler handler = FinishedHandler())
    {
        return boost::shared_ptr<HTTPRequestWriter>(new HTTPRequestWriter(tcp_conn, handler));
    }
    
    /**
     * creates new HTTPRequestWriter objects
     * 
     * @param tcp_conn TCP connection used to send the request
     * @param http_request pointer to the request that will be sent
     * @param handler function called after the request has been sent
     * 
     * @return boost::shared_ptr<HTTPRequestWriter> shared pointer to
     *         the new writer object that was created
     */
    static inline boost::shared_ptr<HTTPRequestWriter> create(TCPConnectionPtr& tcp_conn,
                                                              HTTPRequestPtr& http_request,
                                                              FinishedHandler handler = FinishedHandler())
    {
        return boost::shared_ptr<HTTPRequestWriter>(new HTTPRequestWriter(tcp_conn, http_request, handler));
    }

    /// returns a non-const reference to the request that will be sent
    inline HTTPRequest& getRequest(void) { return *m_http_request; }
    
    
protected:
    
    /**
     * protected constructor restricts creation of objects (use create())
     * 
     * @param tcp_conn TCP connection used to send the request
     * @param http_request pointer to the request that will be sent
     * @param handler function called after the request has been sent
     */
    HTTPRequestWriter(TCPConnectionPtr& tcp_conn, FinishedHandler handler)
        : HTTPWriter(tcp_conn, handler), m_http_request(new HTTPRequest)
    {
        setLogger(PION_GET_LOGGER("pion.net.HTTPRequestWriter"));
    }
    
    /**
     * protected constructor restricts creation of objects (use create())
     * 
     * @param tcp_conn TCP connection used to send the request
     * @param http_request pointer to the request that will be sent
     * @param handler function called after the request has been sent
     */
    HTTPRequestWriter(TCPConnectionPtr& tcp_conn, HTTPRequestPtr& http_request,
                      FinishedHandler handler)
        : HTTPWriter(tcp_conn, handler), m_http_request(http_request)
    {
        setLogger(PION_GET_LOGGER("pion.net.HTTPRequestWriter"));
        // check if we should initialize the payload content using
        // the request's content buffer
        if (m_http_request->getContentLength() > 0
            && m_http_request->getContent() != NULL
            && m_http_request->getContent()[0] != '\0')
        {
            writeNoCopy(m_http_request->getContent(),
                        m_http_request->getContentLength());
        }
    }

    
    /**
     * initializes a vector of write buffers with the HTTP message information
     *
     * @param write_buffers vector of write buffers to initialize
     */
    virtual void prepareBuffersForSend(HTTPMessage::WriteBuffers& write_buffers) {
        if (getContentLength() > 0)
            m_http_request->setContentLength(getContentLength());
        m_http_request->prepareBuffersForSend(write_buffers,
                                              getTCPConnection()->getKeepAlive(),
                                              sendingChunkedMessage());
    }

    /// returns a function bound to HTTPWriter::handleWrite()
    virtual WriteHandler bindToWriteHandler(void) {
        return boost::bind(&HTTPRequestWriter::handleWrite, shared_from_this(),
                           boost::asio::placeholders::error,
                           boost::asio::placeholders::bytes_transferred);
    }

    /**
     * called after the request is sent
     * 
     * @param write_error error status from the last write operation
     * @param bytes_written number of bytes sent by the last write operation
     */
    virtual void handleWrite(const boost::system::error_code& write_error,
                             std::size_t bytes_written)
    {
        PionLogger log_ptr(getLogger());
        if (! write_error) {
            // request sent OK
            if (sendingChunkedMessage()) {
                PION_LOG_DEBUG(log_ptr, "Sent HTTP request chunk of " << bytes_written << " bytes");
                clear();
            } else {
                PION_LOG_DEBUG(log_ptr, "Sent HTTP request of " << bytes_written << " bytes");
            }
        }
        finishedWriting(write_error);
    }


private:
    
    /// the request that will be sent
    HTTPRequestPtr          m_http_request;
    
    /// the initial HTTP request header line
    std::string             m_request_line;
};


/// data type for a HTTPRequestWriter pointer
typedef boost::shared_ptr<HTTPRequestWriter>    HTTPRequestWriterPtr;


/// override operator<< for convenience
template <typename T>
const HTTPRequestWriterPtr& operator<<(const HTTPRequestWriterPtr& writer, const T& data) {
    writer->write(data);
    return writer;
}


}   // end namespace net
}   // end namespace pion

#endif
