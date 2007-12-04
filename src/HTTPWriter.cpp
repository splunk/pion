// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/net/HTTPWriter.hpp>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)

	
// HTTPWriter member functions

void HTTPWriter::prepareWriteBuffers(HTTPMessage::WriteBuffers& write_buffers,
									 const bool send_final_chunk)
{
	// check if the HTTP headers have been sent yet
	if (! m_sent_headers) {
		// iniitalize write buffers for send operation
		prepareBuffersForSend(write_buffers);

		// only send the headers once
		m_sent_headers = true;
	}

	// combine I/O write buffers (headers and content) so that everything
	// can be sent together; otherwise, we would have to send headers
	// and content separately, which would not be as efficient
	
	// don't send anything if there is no data in content buffers
	if (m_content_length > 0) {
		if (supportsChunkedMessages() && sendingChunkedMessage()) {
			// prepare the next chunk of data to send
			// write chunk length in hex
			char cast_buf[35];
			sprintf(cast_buf, "%lx", static_cast<long>(m_content_length));
			
			// add chunk length as a string at the back of the text cache
			m_text_cache.push_back(cast_buf);
			// append length of chunk to write_buffers
			write_buffers.push_back(boost::asio::buffer(m_text_cache.back()));
			// append an extra CRLF for chunk formatting
			write_buffers.push_back(boost::asio::buffer(HTTPTypes::STRING_CRLF));
			
			// append response content buffers
			write_buffers.insert(write_buffers.end(), m_content_buffers.begin(),
								 m_content_buffers.end());
			// append an extra CRLF for chunk formatting
			write_buffers.push_back(boost::asio::buffer(HTTPTypes::STRING_CRLF));
		} else {
			// append response content buffers
			write_buffers.insert(write_buffers.end(), m_content_buffers.begin(),
								 m_content_buffers.end());
		}
	}
	
	// prepare a zero-byte (final) chunk
	if (send_final_chunk && supportsChunkedMessages() && sendingChunkedMessage()) {
		// add chunk length as a string at the back of the text cache
		m_text_cache.push_back("0");
		// append length of chunk to write_buffers
		write_buffers.push_back(boost::asio::buffer(m_text_cache.back()));
		// append an extra CRLF for chunk formatting
		write_buffers.push_back(boost::asio::buffer(HTTPTypes::STRING_CRLF));
		write_buffers.push_back(boost::asio::buffer(HTTPTypes::STRING_CRLF));
	}
}

void HTTPWriter::handleWrite(const boost::system::error_code& write_error,
							 std::size_t bytes_written)
{
	if (write_error) {
		// encountered error sending response
		m_tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_CLOSE);	// make sure it will get closed
		PION_LOG_WARN(m_logger, "Unable to send HTTP response (" << write_error.message() << ')');
	} else {
		// response sent OK
		if (sendingChunkedMessage()) {
			PION_LOG_DEBUG(m_logger, "Sent HTTP response chunk of " << bytes_written << " bytes");
		} else {
			PION_LOG_DEBUG(m_logger, "Sent HTTP response of " << bytes_written << " bytes ("
						   << (m_tcp_conn->getKeepAlive() ? "keeping alive)" : "closing)"));
		}
	}
	
	// TCPConnection::finish() calls TCPServer::finishConnection, which will either:
	// a) call HTTPServer::handleConnection again if keep-alive is true; or,
	// b) close the socket and remove it from the server's connection pool
	m_tcp_conn->finish();
}

}	// end namespace net
}	// end namespace pion

