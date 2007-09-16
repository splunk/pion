// -----------------------------------------------------------------
// libpion: a C++ framework for building lightweight HTTP interfaces
// -----------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTPREQUESTPARSER_HEADER__
#define __PION_HTTPREQUESTPARSER_HEADER__

#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/logic/tribool.hpp>
#include <libpion/PionConfig.hpp>
#include <libpion/PionLogger.hpp>
#include <libpion/HTTPRequest.hpp>
#include <libpion/TCPConnection.hpp>
#include <string>


namespace pion {	// begin namespace pion

///
/// HTTPRequestParser: parses HTTP Requests
/// 
class HTTPRequestParser :
	public boost::enable_shared_from_this<HTTPRequestParser>,
	private boost::noncopyable
{

public:

	// function that handles new HTTP requests
	typedef boost::function2<void, HTTPRequestPtr, TCPConnectionPtr>	RequestHandler;

	
	/**
	 * creates new HTTPRequestParser objects
	 *
	 * @param handler HTTP request handler used to process new requests
	 * @param tcp_conn TCP connection containing a new request to parse
	 */
	static inline boost::shared_ptr<HTTPRequestParser> create(RequestHandler handler,
															  TCPConnectionPtr& tcp_conn)
	{
		return boost::shared_ptr<HTTPRequestParser>(new HTTPRequestParser(handler, tcp_conn));
	}
	
	// default destructor
	virtual ~HTTPRequestParser() {}
	
	/// Incrementally reads & parses a new HTTP request
	void readRequest(void);
	
	/// returns true if there are no more bytes available in the read buffer
	inline bool eof(void) const { return m_read_ptr == NULL || m_read_ptr >= m_read_end_ptr; }
	
	/// returns the number of bytes available in the read buffer
	inline unsigned long bytes_available(void) const { return (eof() ? 0 : (m_read_end_ptr - m_read_ptr)); } 

	/// sets the logger to be used
	inline void setLogger(PionLogger log_ptr) { m_logger = log_ptr; }
	
	/// returns the logger currently in use
	inline PionLogger getLogger(void) { return m_logger; }
	
	
protected:

	/**
	 * protected constructor restricts creation of objects (use create())
	 *
	 * @param handler HTTP request handler used to process new requests
	 * @param tcp_conn TCP connection containing a new request to parse
	 */
	HTTPRequestParser(RequestHandler handler, TCPConnectionPtr& tcp_conn)
		: m_logger(PION_GET_LOGGER("Pion.HTTPRequestParser")),
		m_request_handler(handler), m_tcp_conn(tcp_conn),
		m_http_request(HTTPRequest::create()), m_parse_state(PARSE_METHOD_START),
		m_read_ptr(NULL), m_read_end_ptr(NULL)
	{
		m_http_request->setRemoteIp(tcp_conn->getRemoteIp());
	}

	/**
	 * Called after new request header bytes have been read
	 * 
	 * @param read_error error status from the last read operation
	 * @param bytes_read number of bytes consumed by the last read operation
	 */
	void readHeaderBytes(const boost::system::error_code& read_error,
						 std::size_t bytes_read);

	/**
	 * Consumes request header bytes available in the read buffer
	 */
	void consumeHeaderBytes(void);

	/**
	 * Called after new request content bytes have been read
	 * 
	 * @param read_error error status from the last read operation
	 * @param bytes_read number of bytes consumed by the last read operation
	 */
	void readContentBytes(const boost::system::error_code& read_error,
						  std::size_t bytes_read);

	/**
	 * Handles errors that occur during read operations
	 *
	 * @param read_error error status from the last read operation
	 */
	void handleReadError(const boost::system::error_code& read_error);
	
	/**
	 * parses request header bytes from the last read operation
	 * 
	 * @return boost::tribool result of parsing:
	 *         false = request has an error,
	 *         true = finished parsing request,
	 *         intermediate = request is not yet finished
	 */
	boost::tribool parseRequestHeaders(void);

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

	
private:

	/// state used to keep track of where we are in parsing the request
	enum ParseState {
		PARSE_METHOD_START, PARSE_METHOD, PARSE_URI_STEM, PARSE_URI_QUERY,
		PARSE_HTTP_VERSION_H, PARSE_HTTP_VERSION_T_1, PARSE_HTTP_VERSION_T_2,
		PARSE_HTTP_VERSION_P, PARSE_HTTP_VERSION_SLASH,
		PARSE_HTTP_VERSION_MAJOR_START, PARSE_HTTP_VERSION_MAJOR,
		PARSE_HTTP_VERSION_MINOR_START, PARSE_HTTP_VERSION_MINOR,
		PARSE_EXPECTING_NEWLINE, PARSE_EXPECTING_CR,
		PARSE_HEADER_WHITESPACE, PARSE_HEADER_START, PARSE_HEADER_NAME,
		PARSE_SPACE_BEFORE_HEADER_VALUE, PARSE_HEADER_VALUE,
		PARSE_EXPECTING_FINAL_NEWLINE, PARSE_EXPECTING_FINAL_CR
	};

	/// maximum length for the request method
	static const unsigned int			METHOD_MAX;
	
	/// maximum length for the resource requested
	static const unsigned int			RESOURCE_MAX;

	/// maximum length for the query string
	static const unsigned int			QUERY_STRING_MAX;
	
	/// maximum length for an HTTP header name
	static const unsigned int			HEADER_NAME_MAX;

	/// maximum length for an HTTP header value
	static const unsigned int			HEADER_VALUE_MAX;

	/// maximum length for the name of a query string variable
	static const unsigned int			QUERY_NAME_MAX;
	
	/// maximum length for the value of a query string variable
	static const unsigned int			QUERY_VALUE_MAX;
	
	/// maximum length for the name of a cookie name
	static const unsigned int			COOKIE_NAME_MAX;
	
	/// maximum length for the value of a cookie; also used for path and domain
	static const unsigned int			COOKIE_VALUE_MAX;
	
	/// maximum length for an HTTP header name
	static const unsigned int			POST_CONTENT_MAX;
	
	/// primary logging interface used by this class
	PionLogger							m_logger;

	/// A function that handles the request after it has been parsed
	RequestHandler						m_request_handler;
	
	/// The HTTP connection that has a new request to parse
	TCPConnectionPtr					m_tcp_conn;
	
	/// The new HTTP request container being created
	HTTPRequestPtr						m_http_request;
	
	/// The current state of parsing the request
	ParseState							m_parse_state;

	/// points to the next character to be consumed in the read_buffer
	const char *						m_read_ptr;

	/// points to the end of the read_buffer (last byte + 1)
	const char *						m_read_end_ptr;

	/// Used for parsing the request method
	std::string							m_method;
	
	/// Used for parsing the name of resource requested
	std::string							m_resource;
	
	/// Used for parsing the query string portion of the URI
	std::string							m_query_string;
	
	/// Used for parsing the name of HTTP headers
	std::string							m_header_name;

	/// Used for parsing the value of HTTP headers
	std::string							m_header_value;
};


/// data type for a HTTPRequestParser pointer
typedef boost::shared_ptr<HTTPRequestParser>	HTTPRequestParserPtr;



// inline functions for HTTPRequestParser

inline bool HTTPRequestParser::isChar(int c)
{
	return(c >= 0 && c <= 127);
}

inline bool HTTPRequestParser::isControl(int c)
{
	return( (c >= 0 && c <= 31) || c == 127);
}

inline bool HTTPRequestParser::isSpecial(int c)
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

inline bool HTTPRequestParser::isDigit(int c)
{
	return(c >= '0' && c <= '9');
}

}	// end namespace pion

#endif
