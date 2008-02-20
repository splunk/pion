// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTPTYPES_HEADER__
#define __PION_HTTPTYPES_HEADER__

#include <string>
#include <cctype>
#include <pion/PionConfig.hpp>
#include <pion/PionHashMap.hpp>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)

///
/// HTTPTypes: common data types used by HTTP
/// 
struct PION_NET_API HTTPTypes
{
	/// virtual destructor
	virtual ~HTTPTypes() {}
	
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
	
	/// used for case-insensitive comparisons of HTTP header names
	struct HeadersAreEqual {
		inline bool operator()(const std::string& str1, const std::string& str2) const {
			if (str1.size() != str2.size())
				return false;
			std::string::const_iterator it1 = str1.begin();
			std::string::const_iterator it2 = str2.begin();
			while ( (it1!=str1.end()) && (it2!=str2.end()) ) {
				if (tolower(*it1) != tolower(*it2))
					return false;
				++it1;
				++it2;
			}
			return true;
		}
	};

	/// your plain old string hash algorithm, except case insensitive
	struct HashHeader {
		inline unsigned long operator()(const std::string& str) const {
			unsigned long value = 0;
			for (std::string::const_iterator i = str.begin(); i!= str.end(); ++i)
				value = static_cast<unsigned char>(tolower(*i)) + (value << 6) + (value << 16) - value;
			return value;
		}
	};

	struct CaseInsensitiveLess {
		bool operator()(const std::string& str1, const std::string& str2) const {
			if (str1.size() < str2.size())
				return true;
			std::string::const_iterator it1 = str1.begin();
			std::string::const_iterator it2 = str2.begin();
			while ( (it1 != str1.end()) && (it2 != str2.end()) ) {
				if (tolower(*it1) < tolower(*it2))
					return true;
				++it1;
				++it2;
			}
			return false;
		}
	};

	/// use case-insensitive comparisons for HTTP header names
#ifdef _MSC_VER
	typedef PION_HASH_MULTIMAP<std::string, std::string, stdext::hash_compare<std::string, CaseInsensitiveLess> >	Headers;
#else
	typedef PION_HASH_MULTIMAP<std::string, std::string, HashHeader, HeadersAreEqual >	Headers;
#endif

	/// data type for a dictionary of strings (used for HTTP headers)
	typedef PION_HASH_MULTIMAP<std::string, std::string, PION_HASH_STRING >	StringDictionary;

	/// data type for HTTP query parameters
	typedef StringDictionary	QueryParams;
	
	/// data type for HTTP cookie parameters
	typedef StringDictionary	CookieParams;
	
	/// escapes URL-encoded strings (a%20value+with%20spaces)
	static std::string url_decode(const std::string& str);

	/// encodes strings so that they are safe for URLs (with%20spaces)
	static std::string url_encode(const std::string& str);

	/// converts time_t format into an HTTP-date string
	static std::string get_date_string(const time_t t);

	/// builds an HTTP query string from a collection of query parameters
	static std::string make_query_string(const QueryParams& query_params);
	
	/**
	 * creates a "Set-Cookie" header
	 *
	 * @param name the name of the cookie
	 * @param value the value of the cookie
	 * @param path the path of the cookie
	 * @param has_max_age true if the max_age value should be set
	 * @param max_age the life of the cookie, in seconds (0 = discard)
	 *
	 * @return the new "Set-Cookie" header
	 */
	static std::string make_set_cookie_header(const std::string& name,
											  const std::string& value,
											  const std::string& path,
											  const bool has_max_age = false,
											  const unsigned long max_age = 0);		
};

}	// end namespace net
}	// end namespace pion

#endif
