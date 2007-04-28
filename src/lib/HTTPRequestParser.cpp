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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//

#include "HTTPRequestParser.hpp"
#include <boost/bind.hpp>
#include <boost/asio.hpp>


namespace pion {	// begin namespace pion

	
// static members of HTTPRequestParser

const unsigned int	HTTPRequestParser::RESOURCE_MAX = 256 * 1024;	// 256 KB
const unsigned int	HTTPRequestParser::METHOD_MAX = 1024;	// 1 KB
const unsigned int	HTTPRequestParser::HEADER_NAME_MAX = 1024;	// 1 KB
const unsigned int	HTTPRequestParser::HEADER_VALUE_MAX = 1024 * 1024;	// 1 MB
	
	
// HTTPRequestParser member functions

void HTTPRequestParser::readRequest(void)
{
	m_tcp_conn->getSocket().async_read_some(boost::asio::buffer(m_tcp_conn->getReadBuffer()),
											boost::bind(&HTTPRequestParser::readHandler,
														shared_from_this(),
														boost::asio::placeholders::error,
														boost::asio::placeholders::bytes_transferred));
}

void HTTPRequestParser::readHandler(const boost::asio::error& read_error,
									std::size_t bytes_read)
{
	if (!read_error) {

		LOG4CXX_DEBUG(m_logger, "Read " << bytes_read << " bytes from HTTP request");
		
		// parse the bytes read from the last operation
		boost::tribool result = parseRequest(bytes_read);

		if (result) {
			// finished parsing the request and it is valid
			m_http_request->setIsValid(true);
			m_request_handler(m_http_request, m_tcp_conn);
		} else if (!result) {
			// the request is invalid or an error occured
			m_http_request->setIsValid(false);
			m_request_handler(m_http_request, m_tcp_conn);
		} else {
			// not yet finished parsing the request -> read more data
			readRequest();
		}
		
	} else {
		// a read error occured

		if (read_error == boost::asio::error::operation_aborted) {
			// if the operation was aborted, the acceptor was stopped,
			// which means another thread is shutting-down the server
			LOG4CXX_INFO(m_logger, "HTTP request parsing aborted (shutting down)");
		} else {
			LOG4CXX_INFO(m_logger, "HTTP request parsing aborted due to I/O error");
		}
		
		m_tcp_conn->finish();
	}
}

boost::tribool HTTPRequestParser::parseRequest(std::size_t bytes_read) 
{
	register const char *ptr = m_tcp_conn->getReadBuffer().data();
	const char * const end = ptr + bytes_read;

	// parse characters available in the read buffer

	while (ptr < end) {

		switch (m_parse_state) {
		case PARSE_METHOD_START:
			// we have not yet started parsing the HTTP method string
			if (!isChar(*ptr) || isControl(*ptr) || isSpecial(*ptr))
				return false;
			m_parse_state = PARSE_METHOD;
			m_method.erase();
			m_method.push_back(*ptr);
			break;

		case PARSE_METHOD:
			// we have started parsing the HTTP method string
			if (*ptr == ' ') {
				m_http_request->setMethod(m_method);
				m_resource.erase();
				m_parse_state = PARSE_URI;
			} else if (!isChar(*ptr) || isControl(*ptr) || isSpecial(*ptr)) {
				return false;
			} else if (m_method.size() >= METHOD_MAX) {
				return false;
			} else {
				m_method.push_back(*ptr);
			}
			break;

		case PARSE_URI:
			// we have started parsing the URI requested (or resource name)
			if (*ptr == ' ') {
				m_http_request->setResource(m_resource);
				m_parse_state = PARSE_HTTP_VERSION_H;
			} else if (isControl(*ptr)) {
				return false;
			} else if (m_resource.size() >= RESOURCE_MAX) {
				return false;
			} else {
				m_resource.push_back(*ptr);
			}
			break;

		case PARSE_HTTP_VERSION_H:
			// parsing "HTTP"
			if (*ptr != 'H') return false;
			m_parse_state = PARSE_HTTP_VERSION_T_1;
			break;

		case PARSE_HTTP_VERSION_T_1:
			// parsing "HTTP"
			if (*ptr != 'T') return false;
			m_parse_state = PARSE_HTTP_VERSION_T_2;
			break;

		case PARSE_HTTP_VERSION_T_2:
			// parsing "HTTP"
			if (*ptr != 'T') return false;
			m_parse_state = PARSE_HTTP_VERSION_P;
			break;

		case PARSE_HTTP_VERSION_P:
			// parsing "HTTP"
			if (*ptr != 'P') return false;
			m_parse_state = PARSE_HTTP_VERSION_SLASH;
			break;

		case PARSE_HTTP_VERSION_SLASH:
			// parsing slash after "HTTP"
			if (*ptr != '/') return false;
			m_parse_state = PARSE_HTTP_VERSION_MAJOR_START;
			break;

		case PARSE_HTTP_VERSION_MAJOR_START:
			// parsing the first digit of the major version number
			if (!isDigit(*ptr)) return false;
			m_http_request->setVersionMajor(*ptr - '0');
			m_parse_state = PARSE_HTTP_VERSION_MAJOR;
			break;

		case PARSE_HTTP_VERSION_MAJOR:
			// parsing the major version number (not first digit)
			if (*ptr == '.') {
				m_parse_state = PARSE_HTTP_VERSION_MINOR_START;
			} else if (isDigit(*ptr)) {
				m_http_request->setVersionMajor( (m_http_request->getVersionMajor() * 10)
												 + (*ptr - '0') );
			} else {
				return false;
			}
			break;

		case PARSE_HTTP_VERSION_MINOR_START:
			// parsing the first digit of the minor version number
			if (!isDigit(*ptr)) return false;
			m_http_request->setVersionMinor(*ptr - '0');
			m_parse_state = PARSE_HTTP_VERSION_MINOR;
			break;

		case PARSE_HTTP_VERSION_MINOR:
			// parsing the major version number (not first digit)
			if (*ptr == '\r') {
				m_parse_state = PARSE_EXPECTING_NEWLINE;
			} else if (*ptr == '\n') {
				m_parse_state = PARSE_EXPECTING_CR;
			} else if (isDigit(*ptr)) {
				m_http_request->setVersionMinor( (m_http_request->getVersionMinor() * 10)
												 + (*ptr - '0') );
			} else {
				return false;
			}
			break;

		case PARSE_EXPECTING_NEWLINE:
			// we received a CR; expecting a newline to follow
			if (*ptr == '\n') {
				m_parse_state = PARSE_HEADER_START;
			} else if (*ptr == '\r') {
				// we received two CR's in a row
				// assume CR only is (incorrectly) being used for line termination
				// therefore, the request is finished
				++ptr;
				return true;
			} else if (*ptr == '\t' || *ptr == ' ') {
				m_parse_state = PARSE_HEADER_WHITESPACE;
			} else if (!isChar(*ptr) || isControl(*ptr) || isSpecial(*ptr)) {
				return false;
			} else {
				// assume it is the first character for the name of a header
				m_header_name.erase();
				m_header_name.push_back(*ptr);
				m_parse_state = PARSE_HEADER_NAME;
			}
			break;

		case PARSE_EXPECTING_CR:
			// we received a newline without a CR
			if (*ptr == '\r') {
				m_parse_state = PARSE_HEADER_START;
			} else if (*ptr == '\n') {
				// we received two newlines in a row
				// assume newline only is (incorrectly) being used for line termination
				// therefore, the request is finished
				++ptr;
				return true;
			} else if (*ptr == '\t' || *ptr == ' ') {
				m_parse_state = PARSE_HEADER_WHITESPACE;
			} else if (!isChar(*ptr) || isControl(*ptr) || isSpecial(*ptr)) {
				return false;
			} else {
				// assume it is the first character for the name of a header
				m_header_name.erase();
				m_header_name.push_back(*ptr);
				m_parse_state = PARSE_HEADER_NAME;
			}
			break;

		case PARSE_HEADER_WHITESPACE:
			// parsing whitespace before a header name
			if (*ptr == '\r') {
				m_parse_state = PARSE_EXPECTING_NEWLINE;
			} else if (*ptr == '\n') {
				m_parse_state = PARSE_EXPECTING_CR;
			} else if (*ptr != '\t' && *ptr != ' ') {
				if (!isChar(*ptr) || isControl(*ptr) || isSpecial(*ptr))
					return false;
				// assume it is the first character for the name of a header
				m_header_name.erase();
				m_header_name.push_back(*ptr);
				m_parse_state = PARSE_HEADER_NAME;
			}
			break;

		case PARSE_HEADER_START:
			// parsing the start of a new header
			if (*ptr == '\r') {
				m_parse_state = PARSE_EXPECTING_FINAL_NEWLINE;
			} else if (*ptr == '\n') {
				m_parse_state = PARSE_EXPECTING_FINAL_CR;
			} else if (*ptr == '\t' || *ptr == ' ') {
				m_parse_state = PARSE_HEADER_WHITESPACE;
			} else if (!isChar(*ptr) || isControl(*ptr) || isSpecial(*ptr)) {
				return false;
			} else {
				// first character for the name of a header
				m_header_name.erase();
				m_header_name.push_back(*ptr);
				m_parse_state = PARSE_HEADER_NAME;
			}
			break;

		case PARSE_HEADER_NAME:
			// parsing the name of a header
			if (*ptr == ':') {
				m_header_value.erase();
				m_parse_state = PARSE_SPACE_BEFORE_HEADER_VALUE;
			} else if (!isChar(*ptr) || isControl(*ptr) || isSpecial(*ptr)) {
				return false;
			} else if (m_header_name.size() >= HEADER_NAME_MAX) {
				return false;
			} else {
				// character (not first) for the name of a header
				m_header_name.push_back(*ptr);
			}
			break;

		case PARSE_SPACE_BEFORE_HEADER_VALUE:
			// parsing space character before a header's value
			if (*ptr == ' ') {
				m_parse_state = PARSE_HEADER_VALUE;
			} else if (*ptr == '\r') {
				m_http_request->addHeader(m_header_name, m_header_value);
				m_parse_state = PARSE_EXPECTING_NEWLINE;
			} else if (*ptr == '\n') {
				m_http_request->addHeader(m_header_name, m_header_value);
				m_parse_state = PARSE_EXPECTING_CR;
			} else if (!isChar(*ptr) || isControl(*ptr) || isSpecial(*ptr)) {
				return false;
			} else {
				// assume it is the first character for the value of a header
				m_header_value.push_back(*ptr);
				m_parse_state = PARSE_HEADER_VALUE;
			}
			break;

		case PARSE_HEADER_VALUE:
			// parsing the value of a header
			if (*ptr == '\r') {
				m_http_request->addHeader(m_header_name, m_header_value);
				m_parse_state = PARSE_EXPECTING_NEWLINE;
			} else if (*ptr == '\n') {
				m_http_request->addHeader(m_header_name, m_header_value);
				m_parse_state = PARSE_EXPECTING_CR;
			} else if (isControl(*ptr)) {
				return false;
			} else if (m_header_value.size() >= HEADER_VALUE_MAX) {
				return false;
			} else {
				// character (not first) for the value of a header
				m_header_value.push_back(*ptr);
			}
			break;

		case PARSE_EXPECTING_FINAL_NEWLINE:
			if (*ptr == '\n') ++ptr;
			return true;

		case PARSE_EXPECTING_FINAL_CR:
			if (*ptr == '\r') ++ptr;
			return true;
		}

		++ptr;
	}

	return boost::indeterminate;
}

bool HTTPRequestParser::parseURLEncoded(HTTPTypes::StringDictionary& dict,
										const std::string& encoded_string)
{
	/// not yet implemented
	return false;
}

bool HTTPRequestParser::parseCookieEncoded(HTTPTypes::StringDictionary& dict,
										   const std::string& encoded_string)
{
	/// not yet implemented
	return false;
}

bool HTTPRequestParser::parseMultipartEncoded(HTTPTypes::StringDictionary& dict,
											  TCPConnectionPtr& conn)
{
	/// not yet implemented
	return false;
}


}	// end namespace pion

