// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/asio.hpp>
#include <boost/logic/tribool.hpp>
#include <pion/net/HTTPReader.hpp>
#include <pion/net/HTTPRequest.hpp>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)

	
// HTTPReader member functions

void HTTPReader::receive(void)
{
	if (m_tcp_conn->getPipelined()) {
		// there are pipelined messages available in the connection's read buffer
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
	boost::tribool result = parseHTTPHeaders(getMessage());
	
	if (gcount() > 0) {
		// parsed > 0 bytes in HTTP headers
		PION_LOG_DEBUG(m_logger, "Parsed " << gcount() << " HTTP header bytes");
	}

	if (result) {
		// finished reading HTTP headers and they are valid

		// check if we have payload content to read
		getMessage().updateTransferCodingUsingHeader();

		if (getMessage().isChunked()) {
			// message content is encoded using chunks

			if (m_read_ptr < m_read_end_ptr) {
				result = parseChunks(getMessage().getChunkBuffers());
				if (boost::indeterminate(result)) {
					// read more bytes from the connection
					getMoreChunkedContentBytes();
				} else if (!result) {
					// the message is invalid or an error occured
					m_tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_CLOSE);	// make sure it will get closed
					getMessage().setIsValid(false);
					finishedReading();
				} else {
					// finished parsing all chunks
					getMessage().concatenateChunks();
					readContentBytes(0);
				}
			} else {
				getMoreChunkedContentBytes();
			}
			
		} else if (! getMessage().isContentLengthImplied()) {
			// we cannot assume that the message has no content

			if (getMessage().hasHeader(HTTPTypes::HEADER_CONTENT_LENGTH)) {
				// message has a content-length header

				consumeContent(getMessage());
				const std::size_t content_bytes_to_read = getMessage().getContentLength() - gcount();
				if (content_bytes_to_read == 0) {
					// read all of the content from the last read operation
					readContentBytes(0);
				} else {
					// read the rest of the payload content into the buffer
					// and only return after we've finished or an error occurs
					getMoreContentBytes(content_bytes_to_read);
				}
				
			} else {
				// no content-length specified, and the content length cannot 
				// otherwise be determined

				// only if not a request, read through the close of the connection
				if (dynamic_cast<HTTPRequest*>(&getMessage()) == NULL) {
					// clear the chunk buffers before we start
					getMessage().getChunkBuffers().clear();
					
					// read in the remaining data available in the HTTPParser read buffer
					HTTPParser::consumeContentAsNextChunk(getMessage().getChunkBuffers());
					
					// continue reading from the TCP connection until it is closed
					getMoreContentBytes();
				} else {
					// the message has no content
					
					getMessage().setContentLength(0);
					getMessage().createContentBuffer();
					readContentBytes(0);
				}
			}
		} else {
			// the message has no content
			
			getMessage().setContentLength(0);
			getMessage().createContentBuffer();
			readContentBytes(0);
		}
		
	} else if (!result) {
		// the message is invalid or an error occured

	#ifndef NDEBUG
		// display extra error information if debug mode is enabled
		std::string bad_message;
		m_read_ptr = m_tcp_conn->getReadBuffer().data();
		while (m_read_ptr < m_read_end_ptr && bad_message.size() < 50) {
#ifndef _MSC_VER
// There's a bug in MSVC's implementation of isprint().
			if (!isprint(*m_read_ptr) || *m_read_ptr == '\n' || *m_read_ptr=='\r')
				bad_message += '.';
			else bad_message += *m_read_ptr;
#endif
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
	// check if a read error occured
	if (read_error)
		handleReadError(read_error);
	else
		readContentBytes(bytes_read);
}
	
void HTTPReader::readContentBytes(std::size_t bytes_read)
{
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

void HTTPReader::readContentBytesUntilEOS(const boost::system::error_code& read_error,
										  std::size_t bytes_read)
{
	if (read_error || bytes_read == 0) {
		// connection closed -> finish the HTTP message
		getMessage().concatenateChunks();
		finishMessage();
		m_tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_CLOSE);
		finishedReading();
		return;
	}

	// use the HTTPParser to convert the data read into a message chunk
	HTTPParser::setReadBuffer(m_tcp_conn->getReadBuffer().data(), bytes_read);
	HTTPParser::consumeContentAsNextChunk(getMessage().getChunkBuffers());
	
	// continue reading from the TCP connection
	getMoreContentBytes();
}
	
void HTTPReader::readChunkedContentBytes(const boost::system::error_code& read_error,
										 std::size_t bytes_read)
{
	if (read_error) {
		// a read error occured
		handleReadError(read_error);
		return;
	}

	// set pointers for new HTTP content data to be consumed
	setReadBuffer(m_tcp_conn->getReadBuffer().data(), bytes_read);

	boost::tribool result = parseChunks(getMessage().getChunkBuffers());
	if (boost::indeterminate(result)) {
		// read more bytes from the connection
		getMoreChunkedContentBytes();
	} else if (!result) {
		// the message is invalid or an error occured
		m_tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_CLOSE);	// make sure it will get closed
		getMessage().setIsValid(false);
		finishedReading();
	} else {
		// finished parsing all chunks
		getMessage().concatenateChunks();
		const boost::system::error_code no_error;
		readContentBytes(no_error, 0);
	}
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

