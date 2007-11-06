// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/net/HTTPReader.hpp>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)

	
// HTTPReader member functions

void HTTPReader::receive(void)
{
	if (m_tcp_conn->getPipelined()) {
		// there are pipeliend messages available in the connection's read buffer
		m_tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_CLOSE);	// default to close the connection
		m_tcp_conn->loadReadPosition(m_read_ptr, m_read_end_ptr);
		consumeHeaderBytes();
	} else {
		// no pipelined messages available in the read buffer -> read bytes from the socket
		m_tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_CLOSE);	// default to close the connection
		getMoreHeaderBytes();
	}
}

void HTTPReader::readHeaderBytes(const boost::system::error_code& read_error,
								 std::size_t bytes_read)
{
	if (read_error) {
		// a read error occured
		handleReadError(read_error);
		return;
	}
	
	PION_LOG_DEBUG(m_logger, "Read " << bytes_read << " bytes from HTTP "
				   << (m_is_request ? "request" : "response"));

	// set pointers for new HTTP header data to be consumed
	setReadBuffer(m_tcp_conn->getReadBuffer().data(), bytes_read);

	// consume the new HTTP header bytes available
	consumeHeaderBytes();
}

void HTTPReader::consumeHeaderBytes(void)
{
	// parse the bytes read from the last operation
	//
	// note that boost::tribool may have one of THREE states:
	//
	// false: encountered an error while parsing message
	// true: finished successfully parsing the message
	// indeterminate: parsed bytes, but the message is not yet finished
	//
	boost::tribool result = parseMessage(getMessage());
	
	if (gcount() > 0) {
		// parsed > 0 bytes in HTTP headers
		PION_LOG_DEBUG(m_logger, "Parsed " << gcount() << " HTTP header bytes");
	}

	if (result) {
		// finished reading HTTP headers and they are valid

		// check if we have payload content to read
		consumeContent(getMessage());
		const unsigned long content_bytes_to_read = getMessage().getContentLength() - gcount();

		if (content_bytes_to_read == 0) {
		
			// read all of the content from the last read operation
			const boost::system::error_code no_error;
			readContentBytes(no_error, 0);

		} else {
			// read the rest of the payload content into the buffer
			// and only return after we've finished or an error occurs
			getMoreContentBytes(content_bytes_to_read);
		}
		
	} else if (!result) {
		// the message is invalid or an error occured

	#ifndef NDEBUG
		// display extra error information if debug mode is enabled
		std::string bad_message;
		m_read_ptr = m_tcp_conn->getReadBuffer().data();
		while (m_read_ptr < m_read_end_ptr && bad_message.size() < 50) {
			if (!isprint(*m_read_ptr) || *m_read_ptr == '\n' || *m_read_ptr=='\r')
				bad_message += '.';
			else bad_message += *m_read_ptr;
			++m_read_ptr;
		}
		PION_LOG_ERROR(m_logger, "Bad " << (m_is_request ? "request" : "response")
					   << " debug: " << bad_message);
	#endif

		m_tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_CLOSE);	// make sure it will get closed
		getMessage().setIsValid(false);
		finishedReading();
		
	} else {
		// not yet finished parsing the message -> read more data
		
		receive();
	}
}

void HTTPReader::readContentBytes(const boost::system::error_code& read_error,
								  std::size_t bytes_read)
{
	if (read_error) {
		// a read error occured
		handleReadError(read_error);
		return;
	}

	if (bytes_read != 0) {
		PION_LOG_DEBUG(m_logger, "Read " << bytes_read
					   << (m_is_request ? " request" : " response")
					   << " content bytes (finished)");
	}
	
	// the message is valid: finish it
	finishMessage();

	// set the connection's lifecycle type
	if (getMessage().checkKeepAlive()) {
		if ( eof() ) {
			// the connection should be kept alive, but does not have pipelined messages
			m_tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_KEEPALIVE);
		} else {
			// the connection has pipelined messages
			m_tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_PIPELINED);

			// save the read position as a bookmark so that it can be retrieved
			// by a new HTTP parser, which will be created after the current
			// message has been handled
			m_tcp_conn->saveReadPosition(m_read_ptr, m_read_end_ptr);
			
			PION_LOG_DEBUG(m_logger, "HTTP pipelined "
						   << (m_is_request ? "request (" : "response (")
						   << bytes_available() << " bytes available)");
		}
	} else {
		m_tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_CLOSE);
	}

	// we have finished parsing the HTTP message
	finishedReading();
}

void HTTPReader::handleReadError(const boost::system::error_code& read_error)
{
	// only log errors if the parsing has already begun
	if (getTotalBytesRead() > 0) {
		if (read_error == boost::asio::error::operation_aborted) {
			// if the operation was aborted, the acceptor was stopped,
			// which means another thread is shutting-down the server
			PION_LOG_INFO(m_logger, "HTTP " << (m_is_request ? "request" : "response")
						  << " parsing aborted (shutting down)");
		} else {
			PION_LOG_INFO(m_logger, "HTTP " << (m_is_request ? "request" : "response")
						  << " parsing aborted (" << read_error.message() << ')');
		}
	}
	// close the connection, forcing the client to establish a new one
	m_tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_CLOSE);	// make sure it will get closed
	m_tcp_conn->finish();
}

}	// end namespace net
}	// end namespace pion

