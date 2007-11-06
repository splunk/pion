// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTPRESPONSEWRITER_HEADER__
#define __PION_HTTPRESPONSEWRITER_HEADER__

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <pion/PionConfig.hpp>
#include <pion/net/HTTPWriter.hpp>
#include <pion/net/HTTPResponse.hpp>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)

///
/// HTTPResponseWriter: used to asynchronously send HTTP responses
/// 
class PION_NET_API HTTPResponseWriter :
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
	 * 
     * @return boost::shared_ptr<HTTPResponseWriter> shared pointer to
     *         the new writer object that was created
	 */
	static inline boost::shared_ptr<HTTPResponseWriter> create(TCPConnectionPtr& tcp_conn)
	{
		return boost::shared_ptr<HTTPResponseWriter>(new HTTPResponseWriter(tcp_conn));
	}

	/**
     * creates new HTTPResponseWriter objects
     * 
	 * @param tcp_conn TCP connection used to send the response
     * @param http_response pointer to the response that will be sent
	 * 
     * @return boost::shared_ptr<HTTPResponseWriter> shared pointer to
     *         the new writer object that was created
	 */
	static inline boost::shared_ptr<HTTPResponseWriter> create(TCPConnectionPtr& tcp_conn,
															   HTTPResponsePtr& http_response)
	{
		return boost::shared_ptr<HTTPResponseWriter>(new HTTPResponseWriter(tcp_conn, http_response));
	}

	/**
     * creates new HTTPResponseWriter objects
     * 
	 * @param tcp_conn TCP connection used to send the response
     * @param http_request pointer to the request we are responding to
	 * 
     * @return boost::shared_ptr<HTTPResponseWriter> shared pointer to
     *         the new writer object that was created
	 */
	static inline boost::shared_ptr<HTTPResponseWriter> create(TCPConnectionPtr& tcp_conn,
															   const HTTPMessage& http_request)
	{
		return boost::shared_ptr<HTTPResponseWriter>(new HTTPResponseWriter(tcp_conn, http_request));
	}
	
	/// returns a non-const reference to the response that will be sent
	inline HTTPResponse& getResponse(void) { return *m_http_response; }
	
	
protected:
	
	/**
	 * protected constructor restricts creation of objects (use create())
     * 
	 * @param tcp_conn TCP connection used to send the response
     * @param http_response pointer to the response that will be sent
	 */
	HTTPResponseWriter(TCPConnectionPtr& tcp_conn)
		: HTTPWriter(tcp_conn), m_http_response(new HTTPResponse)
	{
		setLogger(PION_GET_LOGGER("pion.net.HTTPResponseWriter"));
	}
	
	/**
	 * protected constructor restricts creation of objects (use create())
     * 
	 * @param tcp_conn TCP connection used to send the response
     * @param http_response pointer to the response that will be sent
	 */
	HTTPResponseWriter(TCPConnectionPtr& tcp_conn, HTTPResponsePtr& http_response)
		: HTTPWriter(tcp_conn), m_http_response(http_response)
	{
		setLogger(PION_GET_LOGGER("pion.net.HTTPResponseWriter"));
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
     * @param http_request pointer to the request we are responding to
	 */
	HTTPResponseWriter(TCPConnectionPtr& tcp_conn, const HTTPMessage& http_request)
		: HTTPWriter(tcp_conn), m_http_response(new HTTPResponse(http_request))
	{
		setLogger(PION_GET_LOGGER("pion.net.HTTPResponseWriter"));
	}
	
	
	/**
	 * initializes a vector of write buffers with the HTTP message information
	 *
	 * @param write_buffers vector of write buffers to initialize
	 */
	virtual void prepareBuffersForSend(HTTPMessage::WriteBuffers& write_buffers) {
		if (getContentLength() > 0)
			m_http_response->setContentLength(getContentLength());
		m_http_response->prepareBuffersForSend(write_buffers,
											   getTCPConnection()->getKeepAlive(),
											   sendingChunkedMessage());
	}	

	/// returns a function bound to HTTPWriter::handleWrite()
	virtual WriteHandler bindToWriteHandler(void) {
		return boost::bind(&HTTPResponseWriter::handleWrite, shared_from_this(),
						   boost::asio::placeholders::error,
						   boost::asio::placeholders::bytes_transferred);
	}

	
private:
	
	/// the response that will be sent
	HTTPResponsePtr			m_http_response;
	
	/// the initial HTTP response header line
	std::string				m_response_line;
};


/// data type for a HTTPResponseWriter pointer
typedef boost::shared_ptr<HTTPResponseWriter>	HTTPResponseWriterPtr;


/// override operator<< for convenience
template <typename T>
HTTPResponseWriterPtr& operator<<(HTTPResponseWriterPtr& writer, const T& data) {
	writer->write(data);
	return writer;
}


}	// end namespace net
}	// end namespace pion

#endif
