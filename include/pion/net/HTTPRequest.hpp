// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTPREQUEST_HEADER__
#define __PION_HTTPREQUEST_HEADER__

#include <boost/shared_ptr.hpp>
#include <pion/PionConfig.hpp>
#include <pion/net/HTTPMessage.hpp>
#include <pion/net/PionUser.hpp>

namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)


///
/// HTTPRequest: container for HTTP request information
/// 
class HTTPRequest
	: public HTTPMessage
{
public:

	/**
	 * constructs a new HTTPRequest object
	 *
	 * @param resource the HTTP resource to request
	 */
	HTTPRequest(const std::string& resource)
		: m_method(REQUEST_METHOD_GET), m_resource(resource) {}
	
	/// constructs a new HTTPRequest object (default constructor)
	HTTPRequest(void) : m_method(REQUEST_METHOD_GET) {}
	
	/// virtual destructor
	virtual ~HTTPRequest() {}

	/// clears all request data
	virtual void clear(void) {
		HTTPMessage::clear();
		m_method.erase();
		m_resource.erase();
		m_original_resource.erase();
		m_query_string.erase();
		m_query_params.clear();
		m_cookie_params.clear();
		m_user_record.reset();
		m_charset.clear();
	}

	/// the content length of the message can never be implied for requests
	virtual bool isContentLengthImplied(void) const { return false; }

	/// returns the request method (i.e. GET, POST, PUT)
	inline const std::string& getMethod(void) const { return m_method; }
	
	/// returns the resource uri-stem to be delivered (possibly the result of a redirect)
	inline const std::string& getResource(void) const { return m_resource; }

	/// returns the resource uri-stem originally requested
	inline const std::string& getOriginalResource(void) const { return m_original_resource; }

	/// returns the uri-query or query string requested
	inline const std::string& getQueryString(void) const { return m_query_string; }
	
	/// returns a value for the query key if any are defined; otherwise, an empty string
	inline const std::string& getQuery(const std::string& key) const {
		return getValue(m_query_params, key);
	}

	/// returns a value for the cookie if any are defined; otherwise, an empty string
	/// since cookie names are insensitive, key should use lowercase alpha chars
	inline const std::string& getCookie(const std::string& key) const {
		return getValue(m_cookie_params, key);
	}
	
	/// returns the query parameters
	inline QueryParams& getQueryParams(void) {
		return m_query_params;
	}
	
	/// returns the cookie parameters
	inline CookieParams& getCookieParams(void) {
		return m_cookie_params;
	}

	/// returns true if at least one value for the query key is defined
	inline bool hasQuery(const std::string& key) const {
		return(m_query_params.find(key) != m_query_params.end());
	}
	
	/// returns true if at least one value for the cookie is defined
	/// since cookie names are insensitive, key should use lowercase alpha chars
	inline bool hasCookie(const std::string& key) const {
		return(m_cookie_params.find(key) != m_cookie_params.end());
	}
	
	
	/// sets the HTTP request method (i.e. GET, POST, PUT)
	inline void setMethod(const std::string& str) { 
		m_method = str;
		clearFirstLine();
	}
	
	/// sets the resource or uri-stem originally requested
	inline void setResource(const std::string& str) {
		m_resource = m_original_resource = str;
		clearFirstLine();
	}

	/// changes the resource or uri-stem to be delivered (called as the result of a redirect)
	inline void changeResource(const std::string& str) { m_resource = str; }

	/// sets the uri-query or query string requested
	inline void setQueryString(const std::string& str) {
		m_query_string = str;
		clearFirstLine();
	}
	
	/// adds a value for the query key
	inline void addQuery(const std::string& key, const std::string& value) {
		m_query_params.insert(std::make_pair(key, value));
	}
	
	/// changes the value of a query key
	inline void changeQuery(const std::string& key, const std::string& value) {
		changeValue(m_query_params, key, value);
	}
	
	/// removes all values for a query key
	inline void deleteQuery(const std::string& key) {
		deleteValue(m_query_params, key);
	}
	
	/// use the query parameters to build a query string for the request
	inline void useQueryParamsForQueryString(void) {
		setQueryString(make_query_string(m_query_params));
	}

	/// use the query parameters to build POST content for the request
	inline void useQueryParamsForPostContent(void) {
		std::string post_content(make_query_string(m_query_params));
		setContentLength(post_content.size());
		char *ptr = createContentBuffer();	// null-terminates buffer
		if (! post_content.empty())
			memcpy(ptr, post_content.c_str(), post_content.size());
		setMethod(REQUEST_METHOD_POST);
		setContentType(CONTENT_TYPE_URLENCODED);
	}
	
	/// adds a value for the cookie
	/// since cookie names are insensitive, key should use lowercase alpha chars
	inline void addCookie(const std::string& key, const std::string& value) {
		m_cookie_params.insert(std::make_pair(key, value));
	}

	/// changes the value of a cookie
	/// since cookie names are insensitive, key should use lowercase alpha chars
	inline void changeCookie(const std::string& key, const std::string& value) {
		changeValue(m_cookie_params, key, value);
	}

	/// removes all values for a cookie
	/// since cookie names are insensitive, key should use lowercase alpha chars
	inline void deleteCookie(const std::string& key) {
		deleteValue(m_cookie_params, key);
	}
	
	/// sets the user record for HTTP request after authentication
	inline void setUser(PionUserPtr user) { m_user_record = user; }
	
	/// get the user record for HTTP request after authentication
	inline PionUserPtr getUser() const { return m_user_record; }


protected:

	/// updates the string containing the first line for the HTTP message
	virtual void updateFirstLine(void) const {
		// start out with the request method
		m_first_line = m_method;
		m_first_line += ' ';
		// append the resource requested
		m_first_line += m_resource;
		if (! m_query_string.empty()) {
			// append query string if not empty
			m_first_line += '?';
			m_first_line += m_query_string;
		}
		m_first_line += ' ';
		// append HTTP version
		m_first_line += getVersionString();
	}
	
	
private:

	/// request method (GET, POST, PUT, etc.)
	std::string						m_method;

	/// name of the resource or uri-stem to be delivered
	std::string						m_resource;

	/// name of the resource or uri-stem originally requested
	std::string						m_original_resource;

	/// query string portion of the URI
	std::string						m_query_string;
	
	/// HTTP query parameters parsed from the request line and post content
	QueryParams						m_query_params;

	/// HTTP cookie parameters parsed from the "Cookie" request headers
	CookieParams					m_cookie_params;

	/// pointer to PionUser record if this request had been authenticated 
	PionUserPtr						m_user_record;

	/// charset value from 
	/// Content-Type: application/x-www-form-urlencoded; charset=<VALUE>
	std::string						m_charset;
};


/// data type for a HTTP request pointer
typedef boost::shared_ptr<HTTPRequest>		HTTPRequestPtr;


}	// end namespace net
}	// end namespace pion

#endif
