// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTPREQUEST_HEADER__
#define __PION_HTTPREQUEST_HEADER__

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <boost/asio.hpp>
#include <pion/net/PionConfig.hpp>
#include <pion/net/HTTPTypes.hpp>
#include <string>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)

///
/// HTTPRequest: container for HTTP request information
/// 
class HTTPRequest :
	private boost::noncopyable
{
public:

	/// creates new HTTPRequest objects
	static inline boost::shared_ptr<HTTPRequest> create(void) {
		return boost::shared_ptr<HTTPRequest>(new HTTPRequest);
	}

	/// virtual destructor
	virtual ~HTTPRequest() {}

	
	/// returns the request method (i.e. GET, POST, PUT)
	inline const std::string& getMethod(void) const { return m_method; }
	
	/// returns the resource uri-stem requested
	inline const std::string& getResource(void) const { return m_resource; }
	
	/// returns the uri-query or query string requested
	inline const std::string& getQueryString(void) const { return m_query_string; }
	
	/// returns the request's major HTTP version number
	inline unsigned int getVersionMajor(void) const { return m_version_major; }
	
	/// returns the request's minor HTTP version number
	inline unsigned int getVersionMinor(void) const { return m_version_minor; }
	
	/// returns the length of the POST content (in bytes)
	inline size_t getContentLength(void) const { return m_content_length; }
	
	/// returns a buffer containing the POST content, or NULL if the request
	/// has no POST content
	inline const char *getPostContent(void) { return m_post_content.get(); }
	
	/// returns a value for the header if any are defined; otherwise, an empty string
	inline const std::string& getHeader(const std::string& key) const {
		return getValue(m_headers, key);
	}
	
	/// returns a value for the query key if any are defined; otherwise, an empty string
	inline const std::string& getQuery(const std::string& key) const {
		return getValue(m_query_params, key);
	}

	/// returns a value for the cookie if any are defined; otherwise, an empty string
	/// since cookie names are insensitve, key should use lowercase alpha chars
	inline const std::string& getCookie(const std::string& key) const {
		return getValue(m_cookie_params, key);
	}
	
	/// returns the HTTP request headers
	inline HTTPTypes::Headers& getHeaders(void) {
		return m_headers;
	}
	
	/// returns the query parameters
	inline HTTPTypes::QueryParams& getQueryParams(void) {
		return m_query_params;
	}
	
	/// returns the cookie parameters
	inline HTTPTypes::CookieParams& getCookieParams(void) {
		return m_cookie_params;
	}

	/// returns remote requester IP address
	inline boost::asio::ip::address& getRemoteIp(void) {
		return m_remote_ip;
	}

	/// returns true if at least one value for the header is defined
	inline bool hasHeader(const std::string& key) const {
		return(m_headers.find(key) != m_headers.end());
	}
	
	/// returns true if at least one value for the query key is defined
	inline bool hasQuery(const std::string& key) const {
		return(m_query_params.find(key) != m_query_params.end());
	}
	
	/// returns true if at least one value for the cookie is defined
	/// since cookie names are insensitve, key should use lowercase alpha chars
	inline bool hasCookie(const std::string& key) const {
		return(m_cookie_params.find(key) != m_cookie_params.end());
	}

	/// returns true if the request is valid
	inline bool isValid(void) const { return m_is_valid; }
	
	
	/// sets the HTTP request method (i.e. GET, POST, PUT)
	inline void setMethod(const std::string& str) { m_method = str; }
	
	/// sets the resource or uri-stem requested
	inline void setResource(const std::string& str) { m_resource = str; }
	
	/// sets the uri-query or query string requested
	inline void setQueryString(const std::string& str) { m_query_string = str; }
	
	/// sets the request's major HTTP version number
	inline void setVersionMajor(const unsigned int n) { m_version_major = n; }

	/// sets the request's minor HTTP version number
	inline void setVersionMinor(const unsigned int n) { m_version_minor = n; }

	/// sets the length of the POST content (in bytes)
	inline void setContentLength(const size_t n) { m_content_length = n; }
	
	/// sets requester IP address
	inline void setRemoteIp(const boost::asio::ip::address& ip) { m_remote_ip = ip; }

	/// creates a new POST content buffer of size m_content_length and returns
	/// a pointer to the new buffer (memory is managed by HTTPRequest class)
	inline char *createPostContentBuffer(void) {
		m_post_content.reset(new char[m_content_length]);
		return m_post_content.get();
	}
	
	/// adds a value for the HTTP request header key
	inline void addHeader(const std::string& key, const std::string& value) {
		m_headers.insert(std::make_pair(key, value));
	}
	
	/// adds a value for the query key
	inline void addQuery(const std::string& key, const std::string& value) {
		m_query_params.insert(std::make_pair(key, value));
	}
	
	/// adds a value for the cookie
	/// since cookie names are insensitve, key should use lowercase alpha chars
	inline void addCookie(const std::string& key, const std::string& value) {
		m_cookie_params.insert(std::make_pair(key, value));
	}

	/// sets whether or not the request is valid
	inline void setIsValid(bool b = true) { m_is_valid = b; }
	
	/// clears all request data
	inline void clear(void) {
		m_resource.erase();
		m_query_string.erase();
		m_method.erase();
		m_version_major = m_version_minor = 0;
		m_content_length = 0;
		m_post_content.reset();
		m_headers.clear();
		m_query_params.clear();
		m_cookie_params.clear();
	}

	/// returns true if the HTTP connection may be kept alive
	inline bool checkKeepAlive(void) const {
		return (getHeader(HTTPTypes::HEADER_CONNECTION) != "close"
				&& (getVersionMajor() > 1
					|| (getVersionMajor() >= 1 && getVersionMinor() >= 1)) );
	}	
	
	
protected:
	
	/// protected constructor restricts creation of objects (use create())
	HTTPRequest(void)
		: m_version_major(0), m_version_minor(0),
		m_content_length(0), m_is_valid(false)
	{}

	/**
	 * returns the first value in a dictionary if key is found; or an empty
	 * string if no values are found
	 *
	 * @param dict the dictionary to search for key
	 * @param key the key to search for
	 * @return value if found; empty string if not
	 */
	inline static const std::string& getValue(const HTTPTypes::StringDictionary& dict,
									   const std::string& key)
	{
		HTTPTypes::StringDictionary::const_iterator i = dict.find(key);
		return ( (i==dict.end()) ? HTTPTypes::STRING_EMPTY : i->second );
	}

	
private:
	
	/// request method (GET, POST, PUT, etc.)
	std::string						m_method;

	/// name of the resource being requested, or uri-stem
	std::string						m_resource;
	
	/// query string portion of the URI
	std::string						m_query_string;
	
	/// HTTP major version number for the request
	unsigned int					m_version_major;

	/// HTTP major version number for the request
	unsigned int					m_version_minor;
	
	/// the length of the POST content (in bytes)
	size_t							m_content_length;

	/// the POST content, if any was sent with the request
	boost::scoped_array<char>		m_post_content;
	
	/// HTTP request headers
	HTTPTypes::Headers				m_headers;

	/// HTTP query parameters parsed from the request line and post content
	HTTPTypes::QueryParams			m_query_params;

	/// HTTP cookie parameters parsed from the "Cookie" request headers
	HTTPTypes::CookieParams			m_cookie_params;
	
	/// True if the HTTP request is valid
	bool							m_is_valid;

	/// keep requester IP address
	boost::asio::ip::address		m_remote_ip;
};


/// data type for a HTTP request pointer
typedef boost::shared_ptr<HTTPRequest>		HTTPRequestPtr;


}	// end namespace net
}	// end namespace pion

#endif
