// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <algorithm>
#include <pion/net/HTTPMessageParser.hpp>

using namespace pion::net;

namespace pion {	// begin namespace pion
namespace net {		// begin namespace plugins

boost::tribool HTTPMessageParser::processHeader(const char *ptr, size_t len)
{
	PION_ASSERT(!m_headers_parsed);

	setReadBuffer(ptr, len);

	boost::tribool rc = boost::indeterminate;
	
	rc = parseHTTPHeaders(getMessage());

	// headers parsed
	if(rc == true) 
	{ 
		m_headers_parsed = true;
		rc = boost::indeterminate;

		// check if we have payload content to read
		determineContentLength();

		if(hasContent()) 
		{
			if(m_read_ptr < m_read_end_ptr)
			{
				if(getMessage().isChunked())
				{
					rc = parseChunks(getMessage().getChunkBuffers());
					// finished parsing all chunks?
					if(rc) { getMessage().concatenateChunks(); }
				}
				else
				{
					rc = addToContentBuffer(m_read_ptr, m_read_end_ptr - m_read_ptr);
				}
			}
		}
		else
		{
			// no content - the message is done
			getMessage().setContentLength(0);
			getMessage().createContentBuffer();
			rc = true;
		}
	}
	return rc;
}

boost::tribool HTTPMessageParser::processContent(const char *ptr, size_t len)
{
	PION_ASSERT(m_headers_parsed);
	boost::tribool result = boost::indeterminate;

	// set pointers for new HTTP content data to be consumed
	setReadBuffer(ptr, len);

	if(getMessage().isChunked())
	{
		result = parseChunks(getMessage().getChunkBuffers());
		if(result)
		{
			// finished parsing all chunks
			getMessage().concatenateChunks();
		}
		else if(!result) 
		{
			// the message is invalid or an error occured
			getMessage().setIsValid(false);
		} 
	}
	else
	{
		if(m_content_len == static_cast<size_t>(-1))
		{
			// content len is unknown, read until EOS
			// use the HTTPParser to convert the data read into a message chunk
			consumeContentAsNextChunk(getMessage().getChunkBuffers());
		}
		else
		{
			// content len is known, just copy the data to the message content buffer
			result = addToContentBuffer(ptr, len);
		}
	}

	return result;
}


boost::tribool HTTPMessageParser::addToContentBuffer(const char *ptr, size_t len)
{
	PION_ASSERT(m_content_len > 0 && m_content_len != -1);
	PION_ASSERT(m_content_len > m_content_len_read);
	PION_ASSERT(m_content_len - m_content_len_read >= len);

	if(m_content_len > m_content_len_read)
	{
		memcpy( getMessage().getContent() + m_content_len_read, ptr, std::min(len, m_content_len - m_content_len_read));
		m_content_len_read += len;
	}
	else
	{
		return false;
	}

	if(m_content_len_read >= m_content_len)
	{
		return true;
	}
	else
	{
		return boost::indeterminate;
	}
}

boost::tribool HTTPMessageParser::readNext(const char *ptr, size_t len)
{
	boost::tribool rc = m_headers_parsed ? 
		processContent(ptr, len) : processHeader(ptr, len);

	if(rc) { finishMessage(); } 
	
	return rc;
}

void HTTPMessageParser::determineContentLength()
{
	// check if we have payload content to read
	getMessage().updateTransferCodingUsingHeader();

	if(getMessage().isChunked())
	{
		m_content_len = -1;
	}
	else
	{		
		if(!getMessage().isContentLengthImplied())
		{
			if (getMessage().hasHeader(HTTPTypes::HEADER_CONTENT_LENGTH)) 
			{
				// message has a content-length header
				getMessage().updateContentLengthUsingHeader();
				m_content_len = getMessage().getContentLength();
				m_content_len_read = 0;
				getMessage().createContentBuffer();
			}
			else 
			{
				// no content-length specified, and the content length cannot 
				// otherwise be determined

				// only if not a request, read through the close of the connection
				if (!m_is_request) 
				{
					// clear the chunk buffers before we start
					getMessage().getChunkBuffers().clear();
					
					// read in the remaining data available in the HTTPParser read buffer
					HTTPParser::consumeContentAsNextChunk(getMessage().getChunkBuffers());
					
					// continue reading from the connection until it is closed
					m_content_len = -1;
				} else {
					// the message has no content
					m_content_len = 0;
				}
			}
		}
		else
		{
			// the message has no content
			m_content_len = 0;
		}
	}
}

} // end namespace net
} // end namespace pion