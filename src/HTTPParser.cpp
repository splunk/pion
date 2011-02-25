// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <cstdlib>
#include <boost/regex.hpp>
#include <boost/logic/tribool.hpp>
#include <pion/net/HTTPParser.hpp>
#include <pion/net/HTTPRequest.hpp>
#include <pion/net/HTTPResponse.hpp>
#include <pion/net/HTTPMessage.hpp>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)


// static members of HTTPParser

const boost::uint32_t	HTTPParser::STATUS_MESSAGE_MAX = 1024;	// 1 KB
const boost::uint32_t	HTTPParser::METHOD_MAX = 1024;	// 1 KB
const boost::uint32_t	HTTPParser::RESOURCE_MAX = 256 * 1024;	// 256 KB
const boost::uint32_t	HTTPParser::QUERY_STRING_MAX = 1024 * 1024;	// 1 MB
const boost::uint32_t	HTTPParser::HEADER_NAME_MAX = 1024;	// 1 KB
const boost::uint32_t	HTTPParser::HEADER_VALUE_MAX = 1024 * 1024;	// 1 MB
const boost::uint32_t	HTTPParser::QUERY_NAME_MAX = 1024;	// 1 KB
const boost::uint32_t	HTTPParser::QUERY_VALUE_MAX = 1024 * 1024;	// 1 MB
const boost::uint32_t	HTTPParser::COOKIE_NAME_MAX = 1024;	// 1 KB
const boost::uint32_t	HTTPParser::COOKIE_VALUE_MAX = 1024 * 1024;	// 1 MB
const std::size_t		HTTPParser::DEFAULT_CONTENT_MAX = 1024 * 1024;	// 1 MB


// HTTPParser member functions

void HTTPParser::setError(boost::system::error_code& ec, int ev)
{
	static ErrorCategory cat;
	ec = boost::system::error_code(ev, cat);
}

boost::tribool HTTPParser::parse(HTTPMessage& http_msg,
	boost::system::error_code& ec)
{
	PION_ASSERT(! eof() );

	boost::tribool rc = boost::indeterminate;
	std::size_t total_bytes_parsed = 0;

	if(http_msg.hasMissingPackets()) {
		http_msg.setDataAfterMissingPacket(true);
	}

	do {
		switch (m_message_parse_state) {
			// just started parsing the HTTP message
			case PARSE_START:
				m_message_parse_state = PARSE_HEADERS;
				// step through to PARSE_HEADERS

			// parsing the HTTP headers
			case PARSE_HEADERS:
				rc = parseHeaders(http_msg, ec);
				total_bytes_parsed += m_bytes_last_read;
				// check if we have finished parsing HTTP headers
				if (rc == true) {
					// finishHeaderParsing() updates m_message_parse_state
					rc = finishHeaderParsing(http_msg, ec);
				}
				break;

			// parsing chunked payload content
			case PARSE_CHUNKS:
				rc = parseChunks(http_msg.getChunkCache(), ec);
				total_bytes_parsed += m_bytes_last_read;
				// check if we have finished parsing all chunks
				if (rc == true) {
					http_msg.concatenateChunks();
				}
				break;

			// parsing regular payload content with a known length
			case PARSE_CONTENT:
				rc = consumeContent(http_msg, ec);
				total_bytes_parsed += m_bytes_last_read;
				break;

			// parsing payload content with no length (until EOF)
			case PARSE_CONTENT_NO_LENGTH:
				consumeContentAsNextChunk(http_msg.getChunkCache());
				total_bytes_parsed += m_bytes_last_read;
				break;

			// finished parsing the HTTP message
			case PARSE_END:
				rc = true;
				break;
		}
	} while ( boost::indeterminate(rc) && ! eof() );

	// check if we've finished parsing the HTTP message
	if (rc == true) {
		m_message_parse_state = PARSE_END;
		finish(http_msg);
	} else if(rc == false) {
		computeMsgStatus(http_msg, false);
	}

	// update bytes last read (aggregate individual operations for caller)
	m_bytes_last_read = total_bytes_parsed;

	return rc;
}

boost::tribool HTTPParser::parseMissingData(HTTPMessage& http_msg,
	std::size_t len, boost::system::error_code& ec)
{
	static const char MISSING_DATA_CHAR = 'X';
	boost::tribool rc = boost::indeterminate;

	http_msg.setMissingPackets(true);

	switch (m_message_parse_state) {

		// cannot recover from missing data while parsing HTTP headers
		case PARSE_START:
		case PARSE_HEADERS:
			setError(ec, ERROR_MISSING_HEADER_DATA);
			rc = false;
			break;

		// parsing chunked payload content
		case PARSE_CHUNKS:
			// parsing chunk data -> we can only recover if data fits into current chunk
			if (m_chunked_content_parse_state == PARSE_CHUNK
				&& m_bytes_read_in_current_chunk < m_size_of_current_chunk
				&& (m_size_of_current_chunk - m_bytes_read_in_current_chunk) >= len)
			{
				// use dummy content for missing data
				for (std::size_t n = 0; n < len && http_msg.getChunkCache().size() < m_max_content_length; ++n) 
					http_msg.getChunkCache().push_back(MISSING_DATA_CHAR);

				m_bytes_read_in_current_chunk += len;
				m_bytes_last_read = len;
				m_bytes_total_read += len;
				m_bytes_content_read += len;

				if (m_bytes_read_in_current_chunk == m_size_of_current_chunk) {
					m_chunked_content_parse_state = PARSE_EXPECTING_CR_AFTER_CHUNK;
				}
			} else {
				// cannot recover from missing data
				setError(ec, ERROR_MISSING_CHUNK_DATA);
				rc = false;
			}
			break;

		// parsing regular payload content with a known length
		case PARSE_CONTENT:
			// parsing content (with length) -> we can only recover if data fits into content
			if (m_bytes_content_remaining == 0) {
				// we have all of the remaining payload content
				rc = true;
			} else if (m_bytes_content_remaining < len) {
				// cannot recover from missing data
				setError(ec, ERROR_MISSING_TOO_MUCH_CONTENT);
				rc = false;
			} else {

				// make sure content buffer is not already full
				if ( (m_bytes_content_read+len) <= m_max_content_length) {
					// use dummy content for missing data
					for (std::size_t n = 0; n < len; ++n)
						http_msg.getContent()[m_bytes_content_read++] = MISSING_DATA_CHAR;
				} else {
					m_bytes_content_read += len;
				}

				m_bytes_content_remaining -= len;
				m_bytes_total_read += len;
				m_bytes_last_read = len;

				if (m_bytes_content_remaining == 0)
					rc = true;
			}
			break;

		// parsing payload content with no length (until EOF)
		case PARSE_CONTENT_NO_LENGTH:
			// use dummy content for missing data
			for (std::size_t n = 0; n < len && http_msg.getChunkCache().size() < m_max_content_length; ++n) 
				http_msg.getChunkCache().push_back(MISSING_DATA_CHAR);
			m_bytes_last_read = len;
			m_bytes_total_read += len;
			m_bytes_content_read += len;
			break;

		// finished parsing the HTTP message
		case PARSE_END:
			rc = true;
			break;
	}

	// check if we've finished parsing the HTTP message
	if (rc == true) {
		m_message_parse_state = PARSE_END;
		finish(http_msg);
	} else if(rc == false) {
		computeMsgStatus(http_msg, false);
	}

	return rc;
}

boost::tribool HTTPParser::parseHeaders(HTTPMessage& http_msg,
	boost::system::error_code& ec)
{
	//
	// note that boost::tribool may have one of THREE states:
	//
	// false: encountered an error while parsing HTTP headers
	// true: finished successfully parsing the HTTP headers
	// indeterminate: parsed bytes, but the HTTP headers are not yet finished
	//
	const char *read_start_ptr = m_read_ptr;
	m_bytes_last_read = 0;
	while (m_read_ptr < m_read_end_ptr) {

		if (m_save_raw_headers)
			m_raw_headers += *m_read_ptr;
		
		switch (m_headers_parse_state) {
		case PARSE_METHOD_START:
			// we have not yet started parsing the HTTP method string
			if (*m_read_ptr != ' ' && *m_read_ptr!='\r' && *m_read_ptr!='\n') {	// ignore leading whitespace
				if (!isChar(*m_read_ptr) || isControl(*m_read_ptr) || isSpecial(*m_read_ptr)) {
					setError(ec, ERROR_METHOD_CHAR);
					return false;
				}
				m_headers_parse_state = PARSE_METHOD;
				m_method.erase();
				m_method.push_back(*m_read_ptr);
			}
			break;

		case PARSE_METHOD:
			// we have started parsing the HTTP method string
			if (*m_read_ptr == ' ') {
				m_resource.erase();
				m_headers_parse_state = PARSE_URI_STEM;
			} else if (!isChar(*m_read_ptr) || isControl(*m_read_ptr) || isSpecial(*m_read_ptr)) {
				setError(ec, ERROR_METHOD_CHAR);
				return false;
			} else if (m_method.size() >= METHOD_MAX) {
				setError(ec, ERROR_METHOD_SIZE);
				return false;
			} else {
				m_method.push_back(*m_read_ptr);
			}
			break;

		case PARSE_URI_STEM:
			// we have started parsing the URI stem (or resource name)
			if (*m_read_ptr == ' ') {
				m_headers_parse_state = PARSE_HTTP_VERSION_H;
			} else if (*m_read_ptr == '?') {
				m_query_string.erase();
				m_headers_parse_state = PARSE_URI_QUERY;
			} else if (*m_read_ptr == '\r') {
				http_msg.setVersionMajor(0);
				http_msg.setVersionMinor(0);
				m_headers_parse_state = PARSE_EXPECTING_NEWLINE;
			} else if (*m_read_ptr == '\n') {
				http_msg.setVersionMajor(0);
				http_msg.setVersionMinor(0);
				m_headers_parse_state = PARSE_EXPECTING_CR;
			} else if (isControl(*m_read_ptr)) {
				setError(ec, ERROR_URI_CHAR);
				return false;
			} else if (m_resource.size() >= RESOURCE_MAX) {
				setError(ec, ERROR_URI_SIZE);
				return false;
			} else {
				m_resource.push_back(*m_read_ptr);
			}
			break;

		case PARSE_URI_QUERY:
			// we have started parsing the URI query string
			if (*m_read_ptr == ' ') {
				m_headers_parse_state = PARSE_HTTP_VERSION_H;
			} else if (isControl(*m_read_ptr)) {
				setError(ec, ERROR_QUERY_CHAR);
				return false;
			} else if (m_query_string.size() >= QUERY_STRING_MAX) {
				setError(ec, ERROR_QUERY_SIZE);
				return false;
			} else {
				m_query_string.push_back(*m_read_ptr);
			}
			break;

		case PARSE_HTTP_VERSION_H:
			// parsing "HTTP"
			if (*m_read_ptr == '\r') {
				// should only happen for requests (no HTTP/VERSION specified)
				if (! m_is_request) {
					setError(ec, ERROR_VERSION_EMPTY);
					return false;
				}
				http_msg.setVersionMajor(0);
				http_msg.setVersionMinor(0);
				m_headers_parse_state = PARSE_EXPECTING_NEWLINE;
			} else if (*m_read_ptr == '\n') {
				// should only happen for requests (no HTTP/VERSION specified)
				if (! m_is_request) {
					setError(ec, ERROR_VERSION_EMPTY);
					return false;
				}
				http_msg.setVersionMajor(0);
				http_msg.setVersionMinor(0);
				m_headers_parse_state = PARSE_EXPECTING_CR;
			} else if (*m_read_ptr != 'H') {
				setError(ec, ERROR_VERSION_CHAR);
				return false;
			}
			m_headers_parse_state = PARSE_HTTP_VERSION_T_1;
			break;

		case PARSE_HTTP_VERSION_T_1:
			// parsing "HTTP"
			if (*m_read_ptr != 'T') {
				setError(ec, ERROR_VERSION_CHAR);
				return false;
			}
			m_headers_parse_state = PARSE_HTTP_VERSION_T_2;
			break;

		case PARSE_HTTP_VERSION_T_2:
			// parsing "HTTP"
			if (*m_read_ptr != 'T') {
				setError(ec, ERROR_VERSION_CHAR);
				return false;
			}
			m_headers_parse_state = PARSE_HTTP_VERSION_P;
			break;

		case PARSE_HTTP_VERSION_P:
			// parsing "HTTP"
			if (*m_read_ptr != 'P') {
				setError(ec, ERROR_VERSION_CHAR);
				return false;
			}
			m_headers_parse_state = PARSE_HTTP_VERSION_SLASH;
			break;

		case PARSE_HTTP_VERSION_SLASH:
			// parsing slash after "HTTP"
			if (*m_read_ptr != '/') {
				setError(ec, ERROR_VERSION_CHAR);
				return false;
			}
			m_headers_parse_state = PARSE_HTTP_VERSION_MAJOR_START;
			break;

		case PARSE_HTTP_VERSION_MAJOR_START:
			// parsing the first digit of the major version number
			if (!isDigit(*m_read_ptr)) {
				setError(ec, ERROR_VERSION_CHAR);
				return false;
			}
			http_msg.setVersionMajor(*m_read_ptr - '0');
			m_headers_parse_state = PARSE_HTTP_VERSION_MAJOR;
			break;

		case PARSE_HTTP_VERSION_MAJOR:
			// parsing the major version number (not first digit)
			if (*m_read_ptr == '.') {
				m_headers_parse_state = PARSE_HTTP_VERSION_MINOR_START;
			} else if (isDigit(*m_read_ptr)) {
				http_msg.setVersionMajor( (http_msg.getVersionMajor() * 10)
										  + (*m_read_ptr - '0') );
			} else {
				setError(ec, ERROR_VERSION_CHAR);
				return false;
			}
			break;

		case PARSE_HTTP_VERSION_MINOR_START:
			// parsing the first digit of the minor version number
			if (!isDigit(*m_read_ptr)) {
				setError(ec, ERROR_VERSION_CHAR);
				return false;
			}
			http_msg.setVersionMinor(*m_read_ptr - '0');
			m_headers_parse_state = PARSE_HTTP_VERSION_MINOR;
			break;

		case PARSE_HTTP_VERSION_MINOR:
			// parsing the major version number (not first digit)
			if (*m_read_ptr == ' ') {
				// ignore trailing spaces after version in request
				if (! m_is_request) {
					m_headers_parse_state = PARSE_STATUS_CODE_START;
				}
			} else if (*m_read_ptr == '\r') {
				// should only happen for requests
				if (! m_is_request) {
					setError(ec, ERROR_STATUS_EMPTY);
					return false;
				}
				m_headers_parse_state = PARSE_EXPECTING_NEWLINE;
			} else if (*m_read_ptr == '\n') {
				// should only happen for requests
				if (! m_is_request) {
					setError(ec, ERROR_STATUS_EMPTY);
					return false;
				}
				m_headers_parse_state = PARSE_EXPECTING_CR;
			} else if (isDigit(*m_read_ptr)) {
				http_msg.setVersionMinor( (http_msg.getVersionMinor() * 10)
										  + (*m_read_ptr - '0') );
			} else {
				setError(ec, ERROR_VERSION_CHAR);
				return false;
			}
			break;

		case PARSE_STATUS_CODE_START:
			// parsing the first digit of the response status code
			if (!isDigit(*m_read_ptr)) {
				setError(ec, ERROR_STATUS_CHAR);
				return false;
			}
			m_status_code = (*m_read_ptr - '0');
			m_headers_parse_state = PARSE_STATUS_CODE;
			break;

		case PARSE_STATUS_CODE:
			// parsing the response status code (not first digit)
			if (*m_read_ptr == ' ') {
				m_status_message.erase();
				m_headers_parse_state = PARSE_STATUS_MESSAGE;
			} else if (isDigit(*m_read_ptr)) {
				m_status_code = ( (m_status_code * 10) + (*m_read_ptr - '0') );
			} else if (*m_read_ptr == '\r') {
				// recover from status message not sent
				m_status_message.erase();
				m_headers_parse_state = PARSE_EXPECTING_NEWLINE;
			} else if (*m_read_ptr == '\n') {
				// recover from status message not sent
				m_status_message.erase();
				m_headers_parse_state = PARSE_EXPECTING_CR;
			} else {
				setError(ec, ERROR_STATUS_CHAR);
				return false;
			}
			break;

		case PARSE_STATUS_MESSAGE:
			// parsing the response status message
			if (*m_read_ptr == '\r') {
				m_headers_parse_state = PARSE_EXPECTING_NEWLINE;
			} else if (*m_read_ptr == '\n') {
				m_headers_parse_state = PARSE_EXPECTING_CR;
			} else if (isControl(*m_read_ptr)) {
				setError(ec, ERROR_STATUS_CHAR);
				return false;
			} else if (m_status_message.size() >= STATUS_MESSAGE_MAX) {
				setError(ec, ERROR_STATUS_CHAR);
				return false;
			} else {
				m_status_message.push_back(*m_read_ptr);
			}
			break;

		case PARSE_EXPECTING_NEWLINE:
			// we received a CR; expecting a newline to follow
			if (*m_read_ptr == '\n') {
				m_headers_parse_state = PARSE_HEADER_START;
			} else if (*m_read_ptr == '\r') {
				// we received two CR's in a row
				// assume CR only is (incorrectly) being used for line termination
				// therefore, the message is finished
				++m_read_ptr;
				m_bytes_last_read = (m_read_ptr - read_start_ptr);
				m_bytes_total_read += m_bytes_last_read;
				return true;
			} else if (*m_read_ptr == '\t' || *m_read_ptr == ' ') {
				m_headers_parse_state = PARSE_HEADER_WHITESPACE;
			} else if (!isChar(*m_read_ptr) || isControl(*m_read_ptr) || isSpecial(*m_read_ptr)) {
				setError(ec, ERROR_HEADER_CHAR);
				return false;
			} else {
				// assume it is the first character for the name of a header
				m_header_name.erase();
				m_header_name.push_back(*m_read_ptr);
				m_headers_parse_state = PARSE_HEADER_NAME;
			}
			break;

		case PARSE_EXPECTING_CR:
			// we received a newline without a CR
			if (*m_read_ptr == '\r') {
				m_headers_parse_state = PARSE_HEADER_START;
			} else if (*m_read_ptr == '\n') {
				// we received two newlines in a row
				// assume newline only is (incorrectly) being used for line termination
				// therefore, the message is finished
				++m_read_ptr;
				m_bytes_last_read = (m_read_ptr - read_start_ptr);
				m_bytes_total_read += m_bytes_last_read;
				return true;
			} else if (*m_read_ptr == '\t' || *m_read_ptr == ' ') {
				m_headers_parse_state = PARSE_HEADER_WHITESPACE;
			} else if (!isChar(*m_read_ptr) || isControl(*m_read_ptr) || isSpecial(*m_read_ptr)) {
				setError(ec, ERROR_HEADER_CHAR);
				return false;
			} else {
				// assume it is the first character for the name of a header
				m_header_name.erase();
				m_header_name.push_back(*m_read_ptr);
				m_headers_parse_state = PARSE_HEADER_NAME;
			}
			break;

		case PARSE_HEADER_WHITESPACE:
			// parsing whitespace before a header name
			if (*m_read_ptr == '\r') {
				m_headers_parse_state = PARSE_EXPECTING_NEWLINE;
			} else if (*m_read_ptr == '\n') {
				m_headers_parse_state = PARSE_EXPECTING_CR;
			} else if (*m_read_ptr != '\t' && *m_read_ptr != ' ') {
				if (!isChar(*m_read_ptr) || isControl(*m_read_ptr) || isSpecial(*m_read_ptr))
					setError(ec, ERROR_HEADER_CHAR);
					return false;
				// assume it is the first character for the name of a header
				m_header_name.erase();
				m_header_name.push_back(*m_read_ptr);
				m_headers_parse_state = PARSE_HEADER_NAME;
			}
			break;

		case PARSE_HEADER_START:
			// parsing the start of a new header
			if (*m_read_ptr == '\r') {
				m_headers_parse_state = PARSE_EXPECTING_FINAL_NEWLINE;
			} else if (*m_read_ptr == '\n') {
				m_headers_parse_state = PARSE_EXPECTING_FINAL_CR;
			} else if (*m_read_ptr == '\t' || *m_read_ptr == ' ') {
				m_headers_parse_state = PARSE_HEADER_WHITESPACE;
			} else if (!isChar(*m_read_ptr) || isControl(*m_read_ptr) || isSpecial(*m_read_ptr)) {
				setError(ec, ERROR_HEADER_CHAR);
				return false;
			} else {
				// first character for the name of a header
				m_header_name.erase();
				m_header_name.push_back(*m_read_ptr);
				m_headers_parse_state = PARSE_HEADER_NAME;
			}
			break;

		case PARSE_HEADER_NAME:
			// parsing the name of a header
			if (*m_read_ptr == ':') {
				m_header_value.erase();
				m_headers_parse_state = PARSE_SPACE_BEFORE_HEADER_VALUE;
			} else if (!isChar(*m_read_ptr) || isControl(*m_read_ptr) || isSpecial(*m_read_ptr)) {
				setError(ec, ERROR_HEADER_CHAR);
				return false;
			} else if (m_header_name.size() >= HEADER_NAME_MAX) {
				setError(ec, ERROR_HEADER_NAME_SIZE);
				return false;
			} else {
				// character (not first) for the name of a header
				m_header_name.push_back(*m_read_ptr);
			}
			break;

		case PARSE_SPACE_BEFORE_HEADER_VALUE:
			// parsing space character before a header's value
			if (*m_read_ptr == ' ') {
				m_headers_parse_state = PARSE_HEADER_VALUE;
			} else if (*m_read_ptr == '\r') {
				http_msg.addHeader(m_header_name, m_header_value);
				m_headers_parse_state = PARSE_EXPECTING_NEWLINE;
			} else if (*m_read_ptr == '\n') {
				http_msg.addHeader(m_header_name, m_header_value);
				m_headers_parse_state = PARSE_EXPECTING_CR;
			} else if (!isChar(*m_read_ptr) || isControl(*m_read_ptr) || isSpecial(*m_read_ptr)) {
				setError(ec, ERROR_HEADER_CHAR);
				return false;
			} else {
				// assume it is the first character for the value of a header
				m_header_value.push_back(*m_read_ptr);
				m_headers_parse_state = PARSE_HEADER_VALUE;
			}
			break;

		case PARSE_HEADER_VALUE:
			// parsing the value of a header
			if (*m_read_ptr == '\r') {
				http_msg.addHeader(m_header_name, m_header_value);
				m_headers_parse_state = PARSE_EXPECTING_NEWLINE;
			} else if (*m_read_ptr == '\n') {
				http_msg.addHeader(m_header_name, m_header_value);
				m_headers_parse_state = PARSE_EXPECTING_CR;
			} else if (isControl(*m_read_ptr)) {
				setError(ec, ERROR_HEADER_CHAR);
				return false;
			} else if (m_header_value.size() >= HEADER_VALUE_MAX) {
				setError(ec, ERROR_HEADER_VALUE_SIZE);
				return false;
			} else {
				// character (not first) for the value of a header
				m_header_value.push_back(*m_read_ptr);
			}
			break;

		case PARSE_EXPECTING_FINAL_NEWLINE:
			if (*m_read_ptr == '\n') ++m_read_ptr;
			m_bytes_last_read = (m_read_ptr - read_start_ptr);
			m_bytes_total_read += m_bytes_last_read;
			return true;

		case PARSE_EXPECTING_FINAL_CR:
			if (*m_read_ptr == '\r') ++m_read_ptr;
			m_bytes_last_read = (m_read_ptr - read_start_ptr);
			m_bytes_total_read += m_bytes_last_read;
			return true;
		}
		
		++m_read_ptr;
	}

	m_bytes_last_read = (m_read_ptr - read_start_ptr);
	m_bytes_total_read += m_bytes_last_read;
	return boost::indeterminate;
}

void HTTPParser::updateMessageWithHeaderData(HTTPMessage& http_msg) const
{
	if (isParsingRequest()) {

		// finish an HTTP request message

		HTTPRequest& http_request(dynamic_cast<HTTPRequest&>(http_msg));
		http_request.setMethod(m_method);
		http_request.setResource(m_resource);
		http_request.setQueryString(m_query_string);

		// parse query pairs from the URI query string
		if (! m_query_string.empty()) {
			if (! parseURLEncoded(http_request.getQueryParams(),
								  m_query_string.c_str(),
								  m_query_string.size())) 
				PION_LOG_WARN(m_logger, "Request query string parsing failed (URI)");
		}

		// parse "Cookie" headers in request
		std::pair<HTTPTypes::Headers::const_iterator, HTTPTypes::Headers::const_iterator>
		cookie_pair = http_request.getHeaders().equal_range(HTTPTypes::HEADER_COOKIE);
		for (HTTPTypes::Headers::const_iterator cookie_iterator = cookie_pair.first;
			 cookie_iterator != http_request.getHeaders().end()
			 && cookie_iterator != cookie_pair.second; ++cookie_iterator)
		{
			if (! parseCookieHeader(http_request.getCookieParams(),
									cookie_iterator->second, false) )
				PION_LOG_WARN(m_logger, "Cookie header parsing failed");
		}

	} else {

		// finish an HTTP response message

		HTTPResponse& http_response(dynamic_cast<HTTPResponse&>(http_msg));
		http_response.setStatusCode(m_status_code);
		http_response.setStatusMessage(m_status_message);

		// parse "Set-Cookie" headers in response
		std::pair<HTTPTypes::Headers::const_iterator, HTTPTypes::Headers::const_iterator>
		cookie_pair = http_response.getHeaders().equal_range(HTTPTypes::HEADER_SET_COOKIE);
		for (HTTPTypes::Headers::const_iterator cookie_iterator = cookie_pair.first;
			 cookie_iterator != http_response.getHeaders().end()
			 && cookie_iterator != cookie_pair.second; ++cookie_iterator)
		{
			if (! parseCookieHeader(http_response.getCookieParams(),
									cookie_iterator->second, true) )
				PION_LOG_WARN(m_logger, "Set-Cookie header parsing failed");
		}

	}
}

boost::tribool HTTPParser::finishHeaderParsing(HTTPMessage& http_msg,
	boost::system::error_code& ec)
{
	boost::tribool rc = boost::indeterminate;

	m_bytes_content_remaining = m_bytes_content_read = 0;
	http_msg.setContentLength(0);
	http_msg.updateTransferCodingUsingHeader();
	updateMessageWithHeaderData(http_msg);

	if (http_msg.isChunked()) {

		// content is encoded using chunks
		m_message_parse_state = PARSE_CHUNKS;
		
		// return true if parsing headers only
		if (m_parse_headers_only)
			rc = true;

	} else if (http_msg.isContentLengthImplied()) {

		// content length is implied to be zero
		m_message_parse_state = PARSE_END;
		rc = true;

	} else {
		// content length should be specified in the headers

		if (http_msg.hasHeader(HTTPTypes::HEADER_CONTENT_LENGTH)) {

			// message has a content-length header
			try {
				http_msg.updateContentLengthUsingHeader();
			} catch (...) {
				PION_LOG_ERROR(m_logger, "Unable to update content length");
				setError(ec, ERROR_INVALID_CONTENT_LENGTH);
				return false;
			}

			// check if content-length header == 0
			if (http_msg.getContentLength() == 0) {
				m_message_parse_state = PARSE_END;
				rc = true;
			} else {
				m_message_parse_state = PARSE_CONTENT;
				m_bytes_content_remaining = http_msg.getContentLength();

				// check if content-length exceeds maximum allowed
				if (m_bytes_content_remaining > m_max_content_length)
					http_msg.setContentLength(m_max_content_length);

				// return true if parsing headers only
				if (m_parse_headers_only)
					rc = true;
			}

		} else {
			// no content-length specified, and the content length cannot 
			// otherwise be determined

			// only if not a request, read through the close of the connection
			if (! m_is_request) {
				// clear the chunk buffers before we start
				http_msg.getChunkCache().clear();

				// continue reading content until there is no more data
				m_message_parse_state = PARSE_CONTENT_NO_LENGTH;

				// return true if parsing headers only
				if (m_parse_headers_only)
					rc = true;
			} else {
				m_message_parse_state = PARSE_END;
				rc = true;
			}
		}
	}

	// allocate a buffer for payload content (may be zero-size)
	http_msg.createContentBuffer();

	return rc;
}

bool HTTPParser::parseURLEncoded(HTTPTypes::QueryParams& dict,
								 const char *ptr, const size_t len)
{
	// used to track whether we are parsing the name or value
	enum QueryParseState {
		QUERY_PARSE_NAME, QUERY_PARSE_VALUE
	} parse_state = QUERY_PARSE_NAME;

	// misc other variables used for parsing
	const char * const end = ptr + len;
	std::string query_name;
	std::string query_value;

	// iterate through each encoded character
	while (ptr < end) {
		switch (parse_state) {

		case QUERY_PARSE_NAME:
			// parsing query name
			if (*ptr == '=') {
				// end of name found (OK if empty)
				parse_state = QUERY_PARSE_VALUE;
			} else if (*ptr == '&') {
				// if query name is empty, just skip it (i.e. "&&")
				if (! query_name.empty()) {
					// assume that "=" is missing -- it's OK if the value is empty
					dict.insert( std::make_pair(query_name, query_value) );
					query_name.erase();
				}
			} else if (*ptr == '\r' || *ptr == '\n' || *ptr == '\t') {
				// ignore linefeeds, carriage return and tabs (normally within POST content)
			} else if (isControl(*ptr) || query_name.size() >= QUERY_NAME_MAX) {
				// control character detected, or max sized exceeded
				return false;
			} else {
				// character is part of the name
				query_name.push_back(*ptr);
			}
			break;

		case QUERY_PARSE_VALUE:
			// parsing query value
			if (*ptr == '&') {
				// end of value found (OK if empty)
				if (! query_name.empty()) {
					dict.insert( std::make_pair(query_name, query_value) );
					query_name.erase();
				}
				query_value.erase();
				parse_state = QUERY_PARSE_NAME;
			} else if (*ptr == '\r' || *ptr == '\n' || *ptr == '\t') {
				// ignore linefeeds, carriage return and tabs (normally within POST content)
			} else if (isControl(*ptr) || query_value.size() >= QUERY_VALUE_MAX) {
				// control character detected, or max sized exceeded
				return false;
			} else {
				// character is part of the value
				query_value.push_back(*ptr);
			}
			break;
		}

		++ptr;
	}

	// handle last pair in string
	if (! query_name.empty())
		dict.insert( std::make_pair(query_name, query_value) );

	return true;
}

bool HTTPParser::parseCookieHeader(HTTPTypes::CookieParams& dict,
								   const char *ptr, const size_t len,
								   bool set_cookie_header)
{
	// BASED ON RFC 2109
	// http://www.ietf.org/rfc/rfc2109.txt
	// 
	// The current implementation ignores cookie attributes which begin with '$'
	// (i.e. $Path=/, $Domain=, etc.)

	// used to track what we are parsing
	enum CookieParseState {
		COOKIE_PARSE_NAME, COOKIE_PARSE_VALUE, COOKIE_PARSE_IGNORE
	} parse_state = COOKIE_PARSE_NAME;

	// misc other variables used for parsing
	const char * const end = ptr + len;
	std::string cookie_name;
	std::string cookie_value;
	char value_quote_character = '\0';

	// iterate through each character
	while (ptr < end) {
		switch (parse_state) {

		case COOKIE_PARSE_NAME:
			// parsing cookie name
			if (*ptr == '=') {
				// end of name found (OK if empty)
				value_quote_character = '\0';
				parse_state = COOKIE_PARSE_VALUE;
			} else if (*ptr == ';' || *ptr == ',') {
				// ignore empty cookie names since this may occur naturally
				// when quoted values are encountered
				if (! cookie_name.empty()) {
					// value is empty (OK)
					if (! isCookieAttribute(cookie_name, set_cookie_header))
						dict.insert( std::make_pair(cookie_name, cookie_value) );
					cookie_name.erase();
				}
			} else if (*ptr != ' ') {	// ignore whitespace
				// check if control character detected, or max sized exceeded
				if (isControl(*ptr) || cookie_name.size() >= COOKIE_NAME_MAX)
					return false;
				// character is part of the name
				cookie_name.push_back(*ptr);
			}
			break;

		case COOKIE_PARSE_VALUE:
			// parsing cookie value
			if (value_quote_character == '\0') {
				// value is not (yet) quoted
				if (*ptr == ';' || *ptr == ',') {
					// end of value found (OK if empty)
					if (! isCookieAttribute(cookie_name, set_cookie_header))
						dict.insert( std::make_pair(cookie_name, cookie_value) );
					cookie_name.erase();
					cookie_value.erase();
					parse_state = COOKIE_PARSE_NAME;
				} else if (*ptr == '\'' || *ptr == '"') {
					if (cookie_value.empty()) {
						// begin quoted value
						value_quote_character = *ptr;
					} else if (cookie_value.size() >= COOKIE_VALUE_MAX) {
						// max size exceeded
						return false;
					} else {
						// assume character is part of the (unquoted) value
						cookie_value.push_back(*ptr);
					}
				} else if (*ptr != ' ' || !cookie_value.empty()) {	// ignore leading unquoted whitespace
					// check if control character detected, or max sized exceeded
					if (isControl(*ptr) || cookie_value.size() >= COOKIE_VALUE_MAX)
						return false;
					// character is part of the (unquoted) value
					cookie_value.push_back(*ptr);
				}
			} else {
				// value is quoted
				if (*ptr == value_quote_character) {
					// end of value found (OK if empty)
					if (! isCookieAttribute(cookie_name, set_cookie_header))
						dict.insert( std::make_pair(cookie_name, cookie_value) );
					cookie_name.erase();
					cookie_value.erase();
					parse_state = COOKIE_PARSE_IGNORE;
				} else if (cookie_value.size() >= COOKIE_VALUE_MAX) {
					// max size exceeded
					return false;
				} else {
					// character is part of the (quoted) value
					cookie_value.push_back(*ptr);
				}
			}
			break;

		case COOKIE_PARSE_IGNORE:
			// ignore everything until we reach a comma "," or semicolon ";"
			if (*ptr == ';' || *ptr == ',')
				parse_state = COOKIE_PARSE_NAME;
			break;
		}

		++ptr;
	}

	// handle last cookie in string
	if (! isCookieAttribute(cookie_name, set_cookie_header))
		dict.insert( std::make_pair(cookie_name, cookie_value) );

	return true;
}

boost::tribool HTTPParser::parseChunks(HTTPMessage::ChunkCache& chunk_cache,
	boost::system::error_code& ec)
{
	//
	// note that boost::tribool may have one of THREE states:
	//
	// false: encountered an error while parsing message
	// true: finished successfully parsing the message
	// indeterminate: parsed bytes, but the message is not yet finished
	//
	const char *read_start_ptr = m_read_ptr;
	m_bytes_last_read = 0;
	while (m_read_ptr < m_read_end_ptr) {

		switch (m_chunked_content_parse_state) {
		case PARSE_CHUNK_SIZE_START:
			// we have not yet started parsing the next chunk size
			if (isHexDigit(*m_read_ptr)) {
				m_chunk_size_str.erase();
				m_chunk_size_str.push_back(*m_read_ptr);
				m_chunked_content_parse_state = PARSE_CHUNK_SIZE;
			} else if (*m_read_ptr == ' ' || *m_read_ptr == '\x09' || *m_read_ptr == '\x0D' || *m_read_ptr == '\x0A') {
				// Ignore leading whitespace.  Technically, the standard probably doesn't allow white space here, 
				// but we'll be flexible, since there's no ambiguity.
				break;
			} else {
				setError(ec, ERROR_CHUNK_CHAR);
				return false;
			}
			break;

		case PARSE_CHUNK_SIZE:
			if (isHexDigit(*m_read_ptr)) {
				m_chunk_size_str.push_back(*m_read_ptr);
			} else if (*m_read_ptr == '\x0D') {
				m_chunked_content_parse_state = PARSE_EXPECTING_LF_AFTER_CHUNK_SIZE;
			} else if (*m_read_ptr == ' ' || *m_read_ptr == '\x09') {
				// Ignore trailing tabs or spaces.  Technically, the standard probably doesn't allow this, 
				// but we'll be flexible, since there's no ambiguity.
				m_chunked_content_parse_state = PARSE_EXPECTING_CR_AFTER_CHUNK_SIZE;
			} else {
				setError(ec, ERROR_CHUNK_CHAR);
				return false;
			}
			break;

		case PARSE_EXPECTING_CR_AFTER_CHUNK_SIZE:
			if (*m_read_ptr == '\x0D') {
				m_chunked_content_parse_state = PARSE_EXPECTING_LF_AFTER_CHUNK_SIZE;
			} else if (*m_read_ptr == ' ' || *m_read_ptr == '\x09') {
				// Ignore trailing tabs or spaces.  Technically, the standard probably doesn't allow this, 
				// but we'll be flexible, since there's no ambiguity.
				break;
			} else {
				setError(ec, ERROR_CHUNK_CHAR);
				return false;
			}
			break;

		case PARSE_EXPECTING_LF_AFTER_CHUNK_SIZE:
			// We received a CR; expecting LF to follow.  We can't be flexible here because 
			// if we see anything other than LF, we can't be certain where the chunk starts.
			if (*m_read_ptr == '\x0A') {
				m_bytes_read_in_current_chunk = 0;
				m_size_of_current_chunk = strtol(m_chunk_size_str.c_str(), 0, 16);
				if (m_size_of_current_chunk == 0) {
					m_chunked_content_parse_state = PARSE_EXPECTING_FINAL_CR_AFTER_LAST_CHUNK;
				} else {
					m_chunked_content_parse_state = PARSE_CHUNK;
				}
			} else {
				setError(ec, ERROR_CHUNK_CHAR);
				return false;
			}
			break;

		case PARSE_CHUNK:
			if (m_bytes_read_in_current_chunk < m_size_of_current_chunk) {
				if (chunk_cache.size() < m_max_content_length)
					chunk_cache.push_back(*m_read_ptr);
				m_bytes_read_in_current_chunk++;
			}
			if (m_bytes_read_in_current_chunk == m_size_of_current_chunk) {
				m_chunked_content_parse_state = PARSE_EXPECTING_CR_AFTER_CHUNK;
			}
			break;

		case PARSE_EXPECTING_CR_AFTER_CHUNK:
			// we've read exactly m_size_of_current_chunk bytes since starting the current chunk
			if (*m_read_ptr == '\x0D') {
				m_chunked_content_parse_state = PARSE_EXPECTING_LF_AFTER_CHUNK;
			} else {
				setError(ec, ERROR_CHUNK_CHAR);
				return false;
			}
			break;

		case PARSE_EXPECTING_LF_AFTER_CHUNK:
			// we received a CR; expecting LF to follow
			if (*m_read_ptr == '\x0A') {
				m_chunked_content_parse_state = PARSE_CHUNK_SIZE_START;
			} else {
				setError(ec, ERROR_CHUNK_CHAR);
				return false;
			}
			break;

		case PARSE_EXPECTING_FINAL_CR_AFTER_LAST_CHUNK:
			// we've read the final chunk; expecting final CRLF
			if (*m_read_ptr == '\x0D') {
				m_chunked_content_parse_state = PARSE_EXPECTING_FINAL_LF_AFTER_LAST_CHUNK;
			} else {
				setError(ec, ERROR_CHUNK_CHAR);
				return false;
			}
			break;

		case PARSE_EXPECTING_FINAL_LF_AFTER_LAST_CHUNK:
			// we received the final CR; expecting LF to follow
			if (*m_read_ptr == '\x0A') {
				++m_read_ptr;
				m_bytes_last_read = (m_read_ptr - read_start_ptr);
				m_bytes_total_read += m_bytes_last_read;
				m_bytes_content_read += m_bytes_last_read;
				PION_LOG_DEBUG(m_logger, "Parsed " << m_bytes_last_read << " chunked payload content bytes; chunked content complete.");
				return true;
			} else {
				setError(ec, ERROR_CHUNK_CHAR);
				return false;
			}
		}

		++m_read_ptr;
	}

	m_bytes_last_read = (m_read_ptr - read_start_ptr);
	m_bytes_total_read += m_bytes_last_read;
	m_bytes_content_read += m_bytes_last_read;
	return boost::indeterminate;
}

boost::tribool HTTPParser::consumeContent(HTTPMessage& http_msg,
	boost::system::error_code& ec)
{
	size_t content_bytes_to_read;
	size_t content_bytes_available = bytes_available();
	boost::tribool rc = boost::indeterminate;

	if (m_bytes_content_remaining == 0) {
		// we have all of the remaining payload content
		return true;
	} else {
		if (content_bytes_available >= m_bytes_content_remaining) {
			// we have all of the remaining payload content
			rc = true;
			content_bytes_to_read = m_bytes_content_remaining;
		} else {
			// only some of the payload content is available
			content_bytes_to_read = content_bytes_available;
		}
		m_bytes_content_remaining -= content_bytes_to_read;
	}

	// make sure content buffer is not already full
	if (m_bytes_content_read < m_max_content_length) {
		if (m_bytes_content_read + content_bytes_to_read > m_max_content_length) {
			// read would exceed maximum size for content buffer
			// copy only enough bytes to fill up the content buffer
			memcpy(http_msg.getContent() + m_bytes_content_read, m_read_ptr, 
				m_max_content_length - m_bytes_content_read);
		} else {
			// copy all bytes available
			memcpy(http_msg.getContent() + m_bytes_content_read, m_read_ptr, content_bytes_to_read);
		}
	}

	m_read_ptr += content_bytes_to_read;
	m_bytes_content_read += content_bytes_to_read;
	m_bytes_total_read += content_bytes_to_read;
	m_bytes_last_read = content_bytes_to_read;

	return rc;
}

std::size_t HTTPParser::consumeContentAsNextChunk(HTTPMessage::ChunkCache& chunk_cache)
{
	if (bytes_available() == 0) {
		m_bytes_last_read = 0;
	} else {
		m_bytes_last_read = (m_read_end_ptr - m_read_ptr);
		while (m_read_ptr < m_read_end_ptr) {
			if (chunk_cache.size() < m_max_content_length)
				chunk_cache.push_back(*m_read_ptr);
			++m_read_ptr;
		}
		m_bytes_total_read += m_bytes_last_read;
		m_bytes_content_read += m_bytes_last_read;
	}
	return m_bytes_last_read;
}

void HTTPParser::finish(HTTPMessage& http_msg) const
{
	switch (m_message_parse_state) {
	case PARSE_START:
		http_msg.setIsValid(false);
		http_msg.setContentLength(0);
		http_msg.createContentBuffer();
		return;
	case PARSE_END:
		http_msg.setIsValid(true);
		break;
	case PARSE_HEADERS:
		http_msg.setIsValid(false);
		updateMessageWithHeaderData(http_msg);
		http_msg.setContentLength(0);
		http_msg.createContentBuffer();
		break;
	case PARSE_CONTENT:
		http_msg.setIsValid(false);
		http_msg.setContentLength(getContentBytesRead());
		break;
	case PARSE_CHUNKS:
		http_msg.setIsValid(m_chunked_content_parse_state==PARSE_CHUNK_SIZE_START);
		http_msg.concatenateChunks();
		break;
	case PARSE_CONTENT_NO_LENGTH:
		http_msg.setIsValid(true);
		http_msg.concatenateChunks();
		break;
	}

	computeMsgStatus(http_msg, http_msg.isValid());

	if (isParsingRequest()) {
		// Parse query pairs from post content if content type is x-www-form-urlencoded.
		// Type could be followed by parameters (as defined in section 3.6 of RFC 2616)
		// e.g. Content-Type: application/x-www-form-urlencoded; charset=UTF-8
		HTTPRequest& http_request(dynamic_cast<HTTPRequest&>(http_msg));
		const std::string& content_type_header = http_request.getHeader(HTTPTypes::HEADER_CONTENT_TYPE);
		if (content_type_header.compare(0, HTTPTypes::CONTENT_TYPE_URLENCODED.length(),
										HTTPTypes::CONTENT_TYPE_URLENCODED) == 0)
		{
			if (! parseURLEncoded(http_request.getQueryParams(),
								  http_request.getContent(),
								  http_request.getContentLength())) 
				PION_LOG_WARN(m_logger, "Request query string parsing failed (POST content)");
		}
	}
}

void HTTPParser::computeMsgStatus(HTTPMessage& http_msg, bool msg_parsed_ok )
{
	HTTPMessage::DataStatus st = HTTPMessage::STATUS_NONE;

	if(http_msg.hasMissingPackets()) {
		st = http_msg.hasDataAfterMissingPackets() ?
						HTTPMessage::STATUS_PARTIAL : HTTPMessage::STATUS_TRUNCATED;
	} else {
		st = msg_parsed_ok ? HTTPMessage::STATUS_OK : HTTPMessage::STATUS_TRUNCATED;
	}

	http_msg.setStatus(st);
}

bool HTTPParser::parseForwardedFor(const std::string& header, std::string& public_ip)
{
	// static regex's used to check for ipv4 address
	static const boost::regex IPV4_ADDR_RX("[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}");

	/// static regex used to check for private/local networks:
	/// 10.*
	/// 127.*
	/// 192.168.*
	/// 172.16-31.*
	static const boost::regex PRIVATE_NET_RX("(10\\.[0-9]{1,3}|127\\.[0-9]{1,3}|192\\.168|172\\.1[6-9]|172\\.2[0-9]|172\\.3[0-1])\\.[0-9]{1,3}\\.[0-9]{1,3}");

	// sanity check
	if (header.empty())
		return false;

	// local variables re-used by while loop
	boost::match_results<std::string::const_iterator> m;
	std::string::const_iterator start_it = header.begin();

	// search for next ip address within the header
	while (boost::regex_search(start_it, header.end(), m, IPV4_ADDR_RX)) {
		// get ip that matched
		std::string ip_str(m[0].first, m[0].second);
		// check if public network ip address
		if (! boost::regex_match(ip_str, PRIVATE_NET_RX) ) {
			// match found!
			public_ip = ip_str;
			return true;
		}
		// update search starting position
		start_it = m[0].second;
	}

	// no matches found
	return false;
}

}	// end namespace net
}	// end namespace pion

