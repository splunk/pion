// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTPTYPES_HEADER__
#define __PION_HTTPTYPES_HEADER__

#include <pion/PionConfig.hpp>
#include <pion/PionHashMap.hpp>
#include <string>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)

///
/// HTTPTypes: common data types used by HTTP
/// 
struct PION_NET_API HTTPTypes
{
	
	// generic strings used by HTTP
	static const std::string	STRING_EMPTY;
	static const std::string	STRING_CRLF;
	static const std::string	STRING_HTTP_VERSION;
	static const std::string	HEADER_NAME_VALUE_DELIMITER;
	
	// common HTTP header names
	static const std::string	HEADER_HOST;
	static const std::string	HEADER_COOKIE;
	static const std::string	HEADER_SET_COOKIE;
	static const std::string	HEADER_CONNECTION;
	static const std::string	HEADER_CONTENT_TYPE;
	static const std::string	HEADER_CONTENT_LENGTH;
	static const std::string	HEADER_CONTENT_LOCATION;
	static const std::string	HEADER_LAST_MODIFIED;
	static const std::string	HEADER_IF_MODIFIED_SINCE;
    static const std::string	HEADER_TRANSFER_ENCODING;
	static const std::string	HEADER_LOCATION;

	// common HTTP content types
	static const std::string	CONTENT_TYPE_HTML;
	static const std::string	CONTENT_TYPE_TEXT;
	static const std::string	CONTENT_TYPE_XML;
	static const std::string	CONTENT_TYPE_URLENCODED;
	
	// common HTTP request methods
	static const std::string	REQUEST_METHOD_HEAD;
	static const std::string	REQUEST_METHOD_GET;
	static const std::string	REQUEST_METHOD_PUT;
	static const std::string	REQUEST_METHOD_POST;
	static const std::string	REQUEST_METHOD_DELETE;
	
	// common HTTP response messages
	static const std::string	RESPONSE_MESSAGE_OK;
	static const std::string	RESPONSE_MESSAGE_CREATED;
	static const std::string	RESPONSE_MESSAGE_NO_CONTENT;
	static const std::string	RESPONSE_MESSAGE_FORBIDDEN;
	static const std::string	RESPONSE_MESSAGE_NOT_FOUND;
	static const std::string	RESPONSE_MESSAGE_METHOD_NOT_ALLOWED;
	static const std::string	RESPONSE_MESSAGE_NOT_MODIFIED;
	static const std::string	RESPONSE_MESSAGE_BAD_REQUEST;
	static const std::string	RESPONSE_MESSAGE_SERVER_ERROR;
	static const std::string	RESPONSE_MESSAGE_NOT_IMPLEMENTED;

	// common HTTP response codes
	static const unsigned int	RESPONSE_CODE_OK;
	static const unsigned int	RESPONSE_CODE_CREATED;
	static const unsigned int	RESPONSE_CODE_NO_CONTENT;
	static const unsigned int	RESPONSE_CODE_FORBIDDEN;
	static const unsigned int	RESPONSE_CODE_NOT_FOUND;
	static const unsigned int	RESPONSE_CODE_METHOD_NOT_ALLOWED;
	static const unsigned int	RESPONSE_CODE_NOT_MODIFIED;
	static const unsigned int	RESPONSE_CODE_BAD_REQUEST;
	static const unsigned int	RESPONSE_CODE_SERVER_ERROR;
	static const unsigned int	RESPONSE_CODE_NOT_IMPLEMENTED;
	
	/// data type for a dictionary of strings (used for HTTP headers)
	typedef PION_HASH_MULTIMAP<std::string, std::string, PION_HASH_STRING >	StringDictionary;

	/// data type for HTTP headers
	typedef StringDictionary	Headers;
	
	/// data type for HTTP query parameters
	typedef StringDictionary	QueryParams;
	
	/// data type for HTTP cookie parameters
	typedef StringDictionary	CookieParams;
	
	/// escapes URL-encoded strings (a%20value+with%20spaces)
	static std::string url_decode(const std::string& str);

	/// converts time_t format into an HTTP-date string
	static std::string get_date_string(const time_t t);
};

}	// end namespace net
}	// end namespace pion

#endif
