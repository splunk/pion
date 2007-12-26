// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTPREQUESTWRITER_HEADER__
#define __PION_HTTPREQUESTWRITER_HEADER__

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <pion/PionConfig.hpp>
#include <pion/net/HTTPWriter.hpp>
#include <pion/net/HTTPRequest.hpp>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)

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
	 * 
	 * @return boost::shared_ptr<HTTPRequestWriter> shared pointer to
	 *         the new writer object that was created
	 */
	static inline boost::shared_ptr<HTTPRequestWriter> create(TCPConnectionPtr& tcp_conn)
	{
		return boost::shared_ptr<HTTPRequestWriter>(new HTTPRequestWriter(tcp_conn));
	}

	/**
	 * creates new HTTPRequestWriter objects
	 * 
	 * @param tcp_conn TCP connection used to send the request
	 * @param http_request pointer to the request that will be sent
	 * 
	 * @return boost::shared_ptr<HTTPRequestWriter> shared pointer to
	 *         the new writer object that was created
	 */
	static inline boost::shared_ptr<HTTPRequestWriter> create(TCPConnectionPtr& tcp_conn,
															  HTTPRequestPtr& http_request)
	{
		return boost::shared_ptr<HTTPRequestWriter>(new HTTPRequestWriter(tcp_conn, http_request));
	}

	/// returns a non-const reference to the request that will be sent
	inline HTTPRequest& getRequest(void) { return *m_http_request; }
	
	
protected:
	
	/**
	 * protected constructor restricts creation of objects (use create())
	 * 
	 * @param tcp_conn TCP connection used to send the request
	 * @param http_request pointer to the request that will be sent
	 */
	HTTPRequestWriter(TCPConnectionPtr& tcp_conn)
		: HTTPWriter(tcp_conn), m_http_request(new HTTPRequest)
	{
		setLogger(PION_GET_LOGGER("pion.net.HTTPRequestWriter"));
	}
	
	/**
	 * protected constructor restricts creation of objects (use create())
	 * 
	 * @param tcp_conn TCP connection used to send the request
	 * @param http_request pointer to the request that will be sent
	 */
	HTTPRequestWriter(TCPConnectionPtr& tcp_conn, HTTPRequestPtr& http_request)
		: HTTPWriter(tcp_conn), m_http_request(http_request)
	{
		setLogger(PION_GET_LOGGER("pion.net.HTTPRequestWriter"));
		// check if we should initialize the payload content using
		// the request's content buffer
		if (http_request->getContentLength() > 0
			&& http_request->getContent() != NULL
			&& http_request->getContent()[0] != '\0')
		{
			writeNoCopy(http_request->getContent(), http_request->getContentLength());
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
		PionLogger logger = getLogger();
		if (write_error) {
			// encountered error sending request
			PION_LOG_WARN(logger, "Unable to send HTTP request (" << write_error.message() << ')');
		} else {
			// request sent OK
			if (sendingChunkedMessage()) {
				PION_LOG_DEBUG(logger, "Sent HTTP request chunk of " << bytes_written << " bytes");
				clear();
			} else {
				PION_LOG_DEBUG(logger, "Sent HTTP request of " << bytes_written << " bytes");
			}
		}
	}


private:
	
	/// the request that will be sent
	HTTPRequestPtr			m_http_request;
	
	/// the initial HTTP request header line
	std::string				m_request_line;
};


/// data type for a HTTPRequestWriter pointer
typedef boost::shared_ptr<HTTPRequestWriter>	HTTPRequestWriterPtr;


/// override operator<< for convenience
template <typename T>
HTTPRequestWriterPtr& operator<<(HTTPRequestWriterPtr& writer, const T& data) {
	writer->write(data);
	return writer;
}


}	// end namespace net
}	// end namespace pion

#endif
