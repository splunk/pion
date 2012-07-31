// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTP_REQUEST_READER_HEADER__
#define __PION_HTTP_REQUEST_READER_HEADER__

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/function/function2.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <pion/config.hpp>
#include <pion/http/request.hpp>
#include <pion/http/reader.hpp>


namespace pion {    // begin namespace pion
namespace net {     // begin namespace net (Pion Network Library)


///
/// HTTPRequestReader: asynchronously reads and parses HTTP requests
///
class HTTPRequestReader :
    public HTTPReader,
    public boost::enable_shared_from_this<HTTPRequestReader>
{

public:

    /// function called after the HTTP message has been parsed
    typedef boost::function3<void, HTTPRequestPtr, TCPConnectionPtr,
        const boost::system::error_code&>   FinishedHandler;

    
    // default destructor
    virtual ~HTTPRequestReader() {}
    
    /**
     * creates new HTTPRequestReader objects
     *
     * @param tcp_conn TCP connection containing a new message to parse
     * @param handler function called after the message has been parsed
     */
    static inline boost::shared_ptr<HTTPRequestReader>
        create(TCPConnectionPtr& tcp_conn, FinishedHandler handler)
    {
        return boost::shared_ptr<HTTPRequestReader>
            (new HTTPRequestReader(tcp_conn, handler));
    }
    
    /// sets a function to be called after HTTP headers have been parsed
    inline void setHeadersParsedCallback(FinishedHandler& h) { m_parsed_headers = h; }
    
    
protected:

    /**
     * protected constructor restricts creation of objects (use create())
     *
     * @param tcp_conn TCP connection containing a new message to parse
     * @param handler function called after the message has been parsed
     */
    HTTPRequestReader(TCPConnectionPtr& tcp_conn, FinishedHandler handler)
        : HTTPReader(true, tcp_conn), m_http_msg(new HTTPRequest),
        m_finished(handler)
    {
        m_http_msg->setRemoteIp(tcp_conn->getRemoteIp());
        setLogger(PION_GET_LOGGER("pion.net.HTTPRequestReader"));
    }
        
    /// Reads more bytes from the TCP connection
    virtual void readBytes(void) {
        getTCPConnection()->async_read_some(boost::bind(&HTTPRequestReader::consumeBytes,
                                                        shared_from_this(),
                                                        boost::asio::placeholders::error,
                                                        boost::asio::placeholders::bytes_transferred));
    }

    /// Called after we have finished parsing the HTTP message headers
    virtual void finishedParsingHeaders(const boost::system::error_code& ec) {
        // call the finished headers handler with the HTTP message
        if (m_parsed_headers) m_parsed_headers(m_http_msg, getTCPConnection(), ec);
    }
    
    /// Called after we have finished reading/parsing the HTTP message
    virtual void finishedReading(const boost::system::error_code& ec) {
        // call the finished handler with the finished HTTP message
        if (m_finished) m_finished(m_http_msg, getTCPConnection(), ec);
    }
    
    /// Returns a reference to the HTTP message being parsed
    virtual HTTPMessage& getMessage(void) { return *m_http_msg; }

    /// The new HTTP message container being created
    HTTPRequestPtr              m_http_msg;

    /// function called after the HTTP message has been parsed
    FinishedHandler             m_finished;

    /// function called after the HTTP message headers have been parsed
    FinishedHandler             m_parsed_headers;
};


/// data type for a HTTPRequestReader pointer
typedef boost::shared_ptr<HTTPRequestReader>    HTTPRequestReaderPtr;


}   // end namespace net
}   // end namespace pion

#endif
