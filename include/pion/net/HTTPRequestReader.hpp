// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTPREQUESTREADER_HEADER__
#define __PION_HTTPREQUESTREADER_HEADER__

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <pion/PionConfig.hpp>
#include <pion/net/HTTPRequest.hpp>
#include <pion/net/HTTPReader.hpp>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)


///
/// HTTPRequestReader: asynchronously reads and parses HTTP requests
///
class HTTPRequestReader :
	public HTTPReader,
	public boost::enable_shared_from_this<HTTPRequestReader>
{

public:

	/// function called after the HTTP message has been parsed
	typedef boost::function2<void, HTTPRequestPtr, TCPConnectionPtr>	FinishedHandler;

	
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
		

	/// Finishes updating the HTTP message we are parsing
	virtual void finishMessage(void) {
		finishRequest(*m_http_msg);
	}

	/// Called after we have finished reading/parsing the HTTP message
	virtual void finishedReading(void) {
		// call the finished handler with the finished HTTP message
		m_finished(m_http_msg, getTCPConnection());
	}
	
	/// Returns a reference to the HTTP message being parsed
	virtual HTTPMessage& getMessage(void) { return *m_http_msg; }

	/// Reads more HTTP header bytes from the TCP connection
	virtual void getMoreHeaderBytes(void) {
		getTCPConnection()->async_read_some(boost::bind(&HTTPRequestReader::readHeaderBytes,
														shared_from_this(),
														boost::asio::placeholders::error,
														boost::asio::placeholders::bytes_transferred));
	}
	
	/**
	 * Reads more payload content bytes from the TCP connection
	 *
	 * @param bytes_to_read number of bytes to read from the connection
	 */
	virtual void getMoreContentBytes(const std::size_t bytes_to_read) {
		getTCPConnection()->async_read(boost::asio::buffer(m_http_msg->getContent() + gcount(), bytes_to_read),
									   boost::asio::transfer_at_least(bytes_to_read),
									   boost::bind(&HTTPRequestReader::readContentBytes,
												   shared_from_this(),
												   boost::asio::placeholders::error,
												   boost::asio::placeholders::bytes_transferred));
	}

	/// Reads more HTTP chunked content bytes from the TCP connection
	virtual void getMoreChunkedContentBytes(void) {
		getTCPConnection()->async_read_some(boost::bind(&HTTPRequestReader::readChunkedContentBytes,
														shared_from_this(),
														boost::asio::placeholders::error,
														boost::asio::placeholders::bytes_transferred));
	}
		
	/// The new HTTP message container being created
	HTTPRequestPtr				m_http_msg;

	/// function called after the HTTP message has been parsed
	FinishedHandler				m_finished;
};


/// data type for a HTTPRequestReader pointer
typedef boost::shared_ptr<HTTPRequestReader>	HTTPRequestReaderPtr;


}	// end namespace net
}	// end namespace pion

#endif
