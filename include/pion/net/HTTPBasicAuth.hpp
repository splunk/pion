// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTPBASICAUTH_HEADER__
#define __PION_HTTPBASICAUTH_HEADER__

#include <map>
#include <string>
#include <pion/PionConfig.hpp>
#include <pion/net/HTTPAuth.hpp>
#include <pion/PionDateTime.hpp>  // order important , otherwise compiling error under win32


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)

///
/// HTTPBasicAuth: a base class for handling HTTP Authentication and session management
/// in accordance with RFC 2617 http://tools.ietf.org/html/rfc2617 
///
class PION_NET_API HTTPBasicAuth :
	public HTTPAuth
{
public:
	
	/// default constructor
	HTTPBasicAuth(PionUserManagerPtr userManager, const std::string& realm="PION:NET");
	
	/// virtual destructor
	virtual ~HTTPBasicAuth() {}
	
	/**
	 * attempts to validate authentication of a new HTTP request. 
	 * If request valid, pointer to user identity object (if any) will be preserved in 
	 * the request and return "true". 
	 * If request not authenticated, appropriate response is sent over tcp_conn
	 * and return "false";
	 *
	 * @param request the new HTTP request to handle
	 * @param tcp_conn the TCP connection that has the new request
	 *
	 * @return true if request valid and user identity inserted into request 
	 */
	virtual bool handleRequest(HTTPRequestPtr& request, TCPConnectionPtr& tcp_conn);
	
	/**
	 * sets a configuration option
	 * Valid options:
	 *    - "domain" - name of authentication domain
	 *
	 * @param name the name of the option to change
	 * @param value the value of the option
	 */
	virtual void setOption(const std::string& name, const std::string& value);

	
protected:

	/**
	 * used to send responses when access to resource is not authorized
	 *
	 * @param http_request the new HTTP request to handle
	 * @param tcp_conn the TCP connection that has the new request
	 */
	void handleUnauthorized(HTTPRequestPtr& http_request, TCPConnectionPtr& tcp_conn);
	
	/**
	 * extracts base64 user credentials from authorization string
	 *
	 * @param authorization value of the HEADER_AUTHORIZATION
	 */
	static bool parseAuthorization(std::string const &authorization, std::string &credentials);
	
	/**
	 * parse base64 credentials and extract username/password
	 */
	static bool parseCredentials(std::string const &credentials, std::string &username, std::string &password);

	
private:
	
	/// data type used to map authentication credentials to PionUser objects
	typedef std::map<std::string,std::pair<PionDateTime,PionUserPtr> >  PionUserCache;
	
	/// number of seconds after which entires in the user cache will be expired
	static const unsigned int	CACHE_EXPIRATION;


	/// authentication realm ( "PION:NET" by default)
	std::string					m_realm; 

	/// time of the last cache clean up
	PionDateTime				m_cache_cleanup_time;
		
	/// cache of users that are currently active
	PionUserCache				m_user_cache;
	
	/// mutex used to protect access to the user cache
	mutable boost::mutex		m_cache_mutex;
};

	
}	// end namespace net
}	// end namespace pion

#endif
