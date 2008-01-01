// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTPRESPONSEREADER_HEADER__
#define __PION_HTTPRESPONSEREADER_HEADER__

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <pion/PionConfig.hpp>
#include <pion/net/HTTPResponse.hpp>
#include <pion/net/HTTPReader.hpp>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)


///
/// HTTPResponseReader: asynchronously reads and parses HTTP responses
///
class HTTPResponseReader :
	public HTTPReader,
	public boost::enable_shared_from_this<HTTPResponseReader>
{

public:

	/// function called after the HTTP message has been parsed
	typedef boost::function2<void, HTTPResponsePtr, TCPConnectionPtr>	FinishedHandler;

	
	// default destructor
	virtual ~HTTPResponseReader() {}
	
	/**
	 * creates new HTTPResponseReader objects
	 *
	 * @param tcp_conn TCP connection containing a new message to parse
	 * @param handler function called after the message has been parsed
	 */
	static inline boost::shared_ptr<HTTPResponseReader>
		create(TCPConnectionPtr& tcp_conn, FinishedHandler handler)
	{
		return boost::shared_ptr<HTTPResponseReader>
			(new HTTPResponseReader(tcp_conn, handler));
	}

	
protected:

	/**
	 * protected constructor restricts creation of objects (use create())
	 *
	 * @param tcp_conn TCP connection containing a new message to parse
	 * @param handler function called after the message has been parsed
	 */
	HTTPResponseReader(TCPConnectionPtr& tcp_conn, FinishedHandler handler)
		: HTTPReader(false, tcp_conn), m_http_msg(new HTTPResponse),
		m_finished(handler)
	{
		m_http_msg->setRemoteIp(tcp_conn->getRemoteIp());
		setLogger(PION_GET_LOGGER("pion.net.HTTPResponseReader"));
	}
		

	/// Finishes updating the HTTP message we are parsing
	virtual void finishMessage(void) {
		finishResponse(*m_http_msg);
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
		getTCPConnection()->async_read_some(boost::bind(&HTTPResponseReader::readHeaderBytes,
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
									   boost::bind(&HTTPResponseReader::readContentBytes,
												   shared_from_this(),
												   boost::asio::placeholders::error,
												   boost::asio::placeholders::bytes_transferred));
	}

	
	/// The new HTTP message container being created
	HTTPResponsePtr				m_http_msg;

	/// function called after the HTTP message has been parsed
	FinishedHandler				m_finished;
};


/// data type for a HTTPResponseReader pointer
typedef boost::shared_ptr<HTTPResponseReader>	HTTPResponseReaderPtr;


}	// end namespace net
}	// end namespace pion

#endif
