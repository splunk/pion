// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTPPARSER_HEADER__
#define __PION_HTTPPARSER_HEADER__

#include <string>
#include <boost/noncopyable.hpp>
#include <boost/logic/tribool.hpp>
#include <pion/PionConfig.hpp>
#include <pion/PionLogger.hpp>
#include <pion/net/HTTPMessage.hpp>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)

// foward declarations used for finishing HTTP messages
class HTTPRequest;
class HTTPResponse;
	
///
/// HTTPParser: parses HTTP messages
///
class HTTPParser :
	private boost::noncopyable
{

public:
	
	/**
	 * creates new HTTPParser objects
	 *
	 * @param is_request if true, the message is parsed as an HTTP request;
	 *                   if false, the message is parsed as an HTTP response
	 */
	HTTPParser(const bool is_request)
		: m_logger(PION_GET_LOGGER("pion.net.HTTPParser")), m_is_request(is_request),		
		m_read_ptr(NULL), m_read_end_ptr(NULL),
		m_parse_state(is_request ? PARSE_METHOD_START : PARSE_HTTP_VERSION_H),
		m_status_code(0), m_bytes_last_read(0), m_bytes_total_read(0)
	{}
	
	// default destructor
	virtual ~HTTPParser() {}

	
	/**
	 * parses an HTTP message using bytes available in the read buffer
	 *
	 * @param http_msg the HTTP message object to populate from parsing
	 *
	 * @return boost::tribool result of parsing:
	 *                        false = message has an error,
	 *                        true = finished parsing message,
	 *                        indeterminate = message is not yet finished
	 */
	boost::tribool parseMessage(HTTPMessage& http_msg);
	

	/**
	 * prepares the payload content buffer and consumes any content remaining
	 * in the parser's read buffer
	 *
	 * @param http_msg the HTTP message object to consume content for
	 * @return unsigned long number of content bytes consumed, if any
	 */
	std::size_t consumeContent(HTTPMessage& http_msg);
		
	/**
	 * finishes an HTTP request message (copies over request-only data)
	 *
	 * @param http_request the HTTP request object to finish
	 */
	void finishRequest(HTTPRequest& http_request);
	
	/**
	 * finishes an HTTP response message (copies over response-only data)
	 *
	 * @param http_request the HTTP response object to finish
	 */
	void finishResponse(HTTPResponse& http_response);

	/**
	 * resets the location and size of the read buffer
	 *
	 * @param ptr pointer to the first bytes available to be read
	 * @param len number of bytes available to be read
	 */
	inline void setReadBuffer(const char *ptr, size_t len) {
		m_read_ptr = ptr;
		m_read_end_ptr = ptr + len;
	}
	
	/**
	 * loads a read position bookmark
	 *
	 * @param read_ptr points to the next character to be consumed in the read_buffer
	 * @param read_end_ptr points to the end of the read_buffer (last byte + 1)
	 */
	inline void loadReadPosition(const char *&read_ptr, const char *&read_end_ptr) const {
		read_ptr = m_read_ptr;
		read_end_ptr = m_read_end_ptr;
	}
	
	/// resets the parser to it's initial state
	inline void reset(void) {
		m_parse_state = (m_is_request ? PARSE_METHOD_START : PARSE_HTTP_VERSION_H);
		m_status_code = 0;
		m_status_message.erase();
		m_method.erase();
		m_resource.erase();
		m_query_string.erase();
		m_bytes_last_read = m_bytes_total_read = 0;
	}
	
	/// returns true if there are no more bytes available in the read buffer
	inline bool eof(void) const { return m_read_ptr == NULL || m_read_ptr >= m_read_end_ptr; }

	/// returns the number of bytes available in the read buffer
	inline unsigned long bytes_available(void) const { return (eof() ? 0 : (m_read_end_ptr - m_read_ptr)); } 
	
	/// returns the number of bytes read during the last parse operation
	inline std::size_t gcount(void) const { return m_bytes_last_read; }
	
	/// returns the total number of bytes read while parsing the HTTP message
	inline std::size_t getTotalBytesRead(void) const { return m_bytes_total_read; }

	/// sets the logger to be used
	inline void setLogger(PionLogger log_ptr) { m_logger = log_ptr; }
	
	/// returns the logger currently in use
	inline PionLogger getLogger(void) { return m_logger; }
	
	
protected:
		
	/**
	 * parse key-value pairs out of a url-encoded string
	 * (i.e. this=that&a=value)
	 * 
	 * @param dict dictionary for key-values pairs
	 * @param ptr points to the start of the encoded string
	 * @param len length of the encoded string, in bytes
	 * 
	 * @return bool true if successful
	 */
	static bool parseURLEncoded(HTTPTypes::StringDictionary& dict,
								const char *ptr, const size_t len);

	/**
	 * parse key-value pairs out of a "Cookie" request header
	 * (i.e. this=that; a=value)
	 * 
	 * @param dict dictionary for key-values pairs
	 * @param cookie_header header string to be parsed
	 * 
	 * @return bool true if successful
	 */
	static bool parseCookieHeader(HTTPTypes::StringDictionary& dict,
								  const std::string& cookie_header);

	// misc functions used by parseRequest()
	inline static bool isChar(int c);
	inline static bool isControl(int c);
	inline static bool isSpecial(int c);
	inline static bool isDigit(int c);

	
	/// maximum length for response status message
	static const unsigned long			STATUS_MESSAGE_MAX;
	
	/// maximum length for the request method
	static const unsigned long			METHOD_MAX;
	
	/// maximum length for the resource requested
	static const unsigned long			RESOURCE_MAX;
	
	/// maximum length for the query string
	static const unsigned long			QUERY_STRING_MAX;
	
	/// maximum length for an HTTP header name
	static const unsigned long			HEADER_NAME_MAX;
	
	/// maximum length for an HTTP header value
	static const unsigned long			HEADER_VALUE_MAX;
	
	/// maximum length for the name of a query string variable
	static const unsigned long			QUERY_NAME_MAX;
	
	/// maximum length for the value of a query string variable
	static const unsigned long			QUERY_VALUE_MAX;
	
	/// maximum length for the name of a cookie name
	static const unsigned long			COOKIE_NAME_MAX;
	
	/// maximum length for the value of a cookie; also used for path and domain
	static const unsigned long			COOKIE_VALUE_MAX;
	
	/// maximum length for HTTP post content
	static const unsigned long			POST_CONTENT_MAX;
	
	
	/// primary logging interface used by this class
	PionLogger							m_logger;
	
	/// true if the message is an HTTP request; false if it is an HTTP response
	const bool							m_is_request;
	
	/// points to the next character to be consumed in the read_buffer
	const char *						m_read_ptr;
	
	/// points to the end of the read_buffer (last byte + 1)
	const char *						m_read_end_ptr;

	
private:

	/// state used to keep track of where we are in parsing the request
	enum ParseState {
		PARSE_METHOD_START, PARSE_METHOD, PARSE_URI_STEM, PARSE_URI_QUERY,
		PARSE_HTTP_VERSION_H, PARSE_HTTP_VERSION_T_1, PARSE_HTTP_VERSION_T_2,
		PARSE_HTTP_VERSION_P, PARSE_HTTP_VERSION_SLASH,
		PARSE_HTTP_VERSION_MAJOR_START, PARSE_HTTP_VERSION_MAJOR,
		PARSE_HTTP_VERSION_MINOR_START, PARSE_HTTP_VERSION_MINOR,
		PARSE_STATUS_CODE_START, PARSE_STATUS_CODE, PARSE_STATUS_MESSAGE,
		PARSE_EXPECTING_NEWLINE, PARSE_EXPECTING_CR,
		PARSE_HEADER_WHITESPACE, PARSE_HEADER_START, PARSE_HEADER_NAME,
		PARSE_SPACE_BEFORE_HEADER_VALUE, PARSE_HEADER_VALUE,
		PARSE_EXPECTING_FINAL_NEWLINE, PARSE_EXPECTING_FINAL_CR
	};

	
	/// the current state of parsing the request
	ParseState							m_parse_state;

	/// Used for parsing the HTTP response status code
	unsigned int						m_status_code;
	
	/// Used for parsing the HTTP response status message
	std::string							m_status_message;
	
	/// Used for parsing the request method
	std::string							m_method;
	
	/// Used for parsing the name of resource requested
	std::string							m_resource;
	
	/// Used for parsing the query string portion of a URI
	std::string							m_query_string;
	
	/// Used for parsing the name of HTTP headers
	std::string							m_header_name;

	/// Used for parsing the value of HTTP headers
	std::string							m_header_value;

	/// number of bytes read during last parse operation
	std::size_t 						m_bytes_last_read;
	
	/// total number of bytes read while parsing the HTTP message
	std::size_t 						m_bytes_total_read;
};


// inline functions for HTTPParser

inline bool HTTPParser::isChar(int c)
{
	return(c >= 0 && c <= 127);
}

inline bool HTTPParser::isControl(int c)
{
	return( (c >= 0 && c <= 31) || c == 127);
}

inline bool HTTPParser::isSpecial(int c)
{
	switch (c) {
	case '(': case ')': case '<': case '>': case '@':
	case ',': case ';': case ':': case '\\': case '"':
	case '/': case '[': case ']': case '?': case '=':
	case '{': case '}': case ' ': case '\t':
		return true;
	default:
		return false;
	}
}

inline bool HTTPParser::isDigit(int c)
{
	return(c >= '0' && c <= '9');
}

}	// end namespace net
}	// end namespace pion

#endif
