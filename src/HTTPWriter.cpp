// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/asio.hpp>
#include <pion/net/HTTPWriter.hpp>
#include <pion/net/HTTPMessage.hpp>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)


// HTTPWriter member functions

void HTTPWriter::prepareWriteBuffers(HTTPMessage::WriteBuffers& write_buffers,
									 const bool send_final_chunk)
{
	// check if the HTTP headers have been sent yet
	if (! m_sent_headers) {
		// initialize write buffers for send operation
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

}	// end namespace net
}	// end namespace pion

