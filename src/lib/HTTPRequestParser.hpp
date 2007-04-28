// -----------------------------------------------------------------
// libpion: a C++ framework for building lightweight HTTP interfaces
// -----------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.
// 
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
//

#ifndef __PION_HTTPREQUESTPARSER_HEADER__
#define __PION_HTTPREQUESTPARSER_HEADER__

#include "PionLogger.hpp"
#include "HTTPRequest.hpp"
#include "TCPConnection.hpp"
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/logic/tribool.hpp>
#include <string>


namespace pion {	// begin namespace pion

///
/// HTTPRequestParser: parses HTTP Requests
/// 
class HTTPRequestParser
	: public boost::enable_shared_from_this<HTTPRequestParser>,
	private boost::noncopyable
{

public:

	// function that handles new HTTP requests
	typedef boost::function2<void, HTTPRequestPtr, TCPConnectionPtr>	RequestHandler;

	// default destructor
	virtual ~HTTPRequestParser() {}

	/**
	 * constructor to create a new HTTP request parser
	 *
	 * @param handler HTTP request handler used to process new requests
	 * @param tcp_conn TCP connection containing a new request to parse
	 */
	explicit HTTPRequestParser(RequestHandler handler, TCPConnectionPtr& tcp_conn)
		: m_logger(log4cxx::Logger::getLogger("Pion.HTTPRequestParser")),
		m_request_handler(handler), m_tcp_conn(tcp_conn),
		m_http_request(new HTTPRequest), m_parse_state(PARSE_METHOD_START)
	{}
	
	/// Incrementally reads & parses a new HTTP request
	void readRequest(void);

	/// sets the logger to be used
	inline void setLogger(log4cxx::LoggerPtr log_ptr) { m_logger = log_ptr; }
	
	/// returns the logger currently in use
	inline log4cxx::LoggerPtr getLogger(void) { return m_logger; }
	
	
protected:

	/**
	 * Called after new request bytes have been read
	 * 
	 * @param read_error error status from the last read operation
	 * @param bytes_read number of bytes consumed by the last read operation
	 */
	void readHandler(const boost::asio::error& read_error, std::size_t bytes_read);

	/**
	 * parses bytes from the last read operation
	 * 
	 * @param bytes_read number of bytes available in the read buffer
	 * 
	 * @return boost::tribool result of parsing
	 */
	boost::tribool parseRequest(std::size_t bytes_read);

	/**
	 * parse key-value pairs out of a url-encoded string
	 * (i.e. this=that&a=value)
	 * 
	 * @param dict dictionary for key-values pairs
	 * @param encoded_string string to be parsed
	 * 
	 * @return bool true if successful
	 */
	static bool parseURLEncoded(HTTPTypes::StringDictionary& dict,
								const std::string& encoded_string);

	/**
	 * parse key-value pairs out of a cookie-encoded string
	 * (i.e. this=that; a=value)
	 * 
	 * @param dict dictionary for key-values pairs
	 * @param encoded_string string to be parsed
	 * 
	 * @return bool true if successful
	 */
	static bool parseCookieEncoded(HTTPTypes::StringDictionary& dict,
								   const std::string& encoded_string);

	/**
	 * parse key-value pairs out of a multipart encoded request
	 * 
	 * @param dict dictionary for key-values pairs
	 * @param conn the tcp connection for the request
	 * 
	 * @return bool true if successful
	 */
	static bool parseMultipartEncoded(HTTPTypes::StringDictionary& dict,
									  TCPConnectionPtr& conn);
	
	// misc functions used by parseRequest()
	inline static bool isChar(int c);
	inline static bool isControl(int c);
	inline static bool isSpecial(int c);
	inline static bool isDigit(int c);

	
private:

	/// maximum length for the resource requested
	static const unsigned int	RESOURCE_MAX;

	/// maximum length for the request method
	static const unsigned int	METHOD_MAX;
	
	/// maximum length for an HTTP header name
	static const unsigned int	HEADER_NAME_MAX;

	/// maximum length for an HTTP header value
	static const unsigned int	HEADER_VALUE_MAX;

	/// state used to keep track of where we are in parsing the request
	enum ParseState {
		PARSE_METHOD_START, PARSE_METHOD, PARSE_URI,
		PARSE_HTTP_VERSION_H, PARSE_HTTP_VERSION_T_1, PARSE_HTTP_VERSION_T_2,
		PARSE_HTTP_VERSION_P, PARSE_HTTP_VERSION_SLASH,
		PARSE_HTTP_VERSION_MAJOR_START, PARSE_HTTP_VERSION_MAJOR,
		PARSE_HTTP_VERSION_MINOR_START, PARSE_HTTP_VERSION_MINOR,
		PARSE_EXPECTING_NEWLINE, PARSE_EXPECTING_CR,
		PARSE_HEADER_WHITESPACE, PARSE_HEADER_START, PARSE_HEADER_NAME,
		PARSE_SPACE_BEFORE_HEADER_VALUE, PARSE_HEADER_VALUE,
		PARSE_EXPECTING_FINAL_NEWLINE, PARSE_EXPECTING_FINAL_CR
	};

	/// primary logging interface used by this class
	log4cxx::LoggerPtr			m_logger;

	/// A function that handles the request after it has been parsed
	RequestHandler				m_request_handler;
	
	/// The HTTP connection that has a new request to parse
	TCPConnectionPtr			m_tcp_conn;
	
	/// The new HTTP request container being created
	HTTPRequestPtr				m_http_request;
	
	/// The current state of parsing the request
	ParseState					m_parse_state;

	/// Used for parsing the name of resource requested
	std::string					m_resource;
	
	/// Used for parsing the request method
	std::string					m_method;

	/// Used for parsing the name of HTTP headers
	std::string					m_header_name;

	/// Used for parsing the value of HTTP headers
	std::string					m_header_value;
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
