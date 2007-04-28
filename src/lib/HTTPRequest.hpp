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

#ifndef __PION_HTTPREQUEST_HEADER__
#define __PION_HTTPREQUEST_HEADER__

#include "HTTPTypes.hpp"
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <string>


namespace pion {	// begin namespace pion

///
/// HTTPRequest: container for HTTP request information
/// 
class HTTPRequest : private boost::noncopyable {
public:

	/// default constructor & destructor
	virtual ~HTTPRequest() {}
	explicit HTTPRequest(void) : m_version_major(0), m_version_minor(0),
		m_is_valid(false) {}
	
	// public accessor methods (const)
	inline bool isValid(void) const { return m_is_valid; }
	inline bool hasHeader(const std::string& name) const { return(m_headers.find(name) != m_headers.end()); }
	inline bool hasQuery(const std::string& name) const { return(m_query_params.find(name) != m_query_params.end()); }
	inline bool hasCookie(const std::string& name) const { return(m_cookie_params.find(name) != m_cookie_params.end()); }
	inline unsigned int getVersionMajor(void) const { return m_version_major; }
	inline unsigned int getVersionMinor(void) const { return m_version_minor; }
	inline const std::string& getResource(void) const { return m_resource; }
	inline const std::string& getMethod(void) const { return m_method; }
	inline const std::string& getHeader(const std::string& name) const { return getValue(m_headers, name); }
	inline const std::string& getQuery(const std::string& name) const { return getValue(m_query_params, name); }
	inline const std::string& getCookie(const std::string& name) const { return getValue(m_cookie_params, name); }
	inline const HTTPTypes::Headers& getHeaders(void) const { return m_headers; }
	inline const HTTPTypes::QueryParams& getQueryParams(void) const { return m_query_params; }
	inline const HTTPTypes::CookieParams& getCookieParams(void) const { return m_cookie_params; }

	// public accessor methods
	inline void setResource(const std::string& str) { m_resource = str; }
	inline void setMethod(const std::string& str) { m_method = str; }
	inline void setVersionMajor(const unsigned int n) { m_version_major = n; }
	inline void setVersionMinor(const unsigned int n) { m_version_minor = n; }
	inline void addHeader(const std::string& key, const std::string& value) { m_headers.insert(std::make_pair(key, value)); }
	inline void addQuery(const std::string& key, const std::string& value) { m_query_params.insert(std::make_pair(key, value)); }
	inline void addCookie(const std::string& key, const std::string& value) { m_cookie_params.insert(std::make_pair(key, value)); }
	inline void setIsValid(bool b = true) { m_is_valid = b; }
	
	/// clears all request data
	inline void clear(void)
	{
		m_resource.erase();
		m_method.erase();
		m_version_major = m_version_minor = 0;
		m_headers.clear();
		m_query_params.clear();
		m_cookie_params.clear();
	}

	/// returns true if the HTTP connection may be kept alive
	inline bool checkKeepAlive(void) const
	{
		return (getHeader(HTTPTypes::HEADER_CONNECTION) != "close"
				&& (getVersionMajor() > 1
					|| (getVersionMajor() >= 1 && getVersionMinor() >= 1)) );
	}	
	
protected:
		
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

	/// name of the resource being requested, or uri-stem
	std::string					m_resource;

	/// request method (GET, POST, PUT, etc.)
	std::string					m_method;

	/// HTTP major version number for the request
	unsigned int				m_version_major;

	/// HTTP major version number for the request
	unsigned int				m_version_minor;

	/// HTTP request headers
	HTTPTypes::Headers			m_headers;

	/// HTTP query parameters parsed from the request line and post content
	HTTPTypes::QueryParams		m_query_params;

	/// HTTP cookie parameters parsed from the "Cookie" request headers
	HTTPTypes::CookieParams		m_cookie_params;

	/// True if the HTTP request is valid
	bool						m_is_valid;
};


/// data type for a HTTP request pointer
typedef boost::shared_ptr<HTTPRequest>		HTTPRequestPtr;


}	// end namespace pion

#endif
