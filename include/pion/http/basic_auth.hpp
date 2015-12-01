// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTP_BASIC_AUTH_HEADER__
#define __PION_HTTP_BASIC_AUTH_HEADER__

#include <map>
#include <string>
#include <pion/config.hpp>
#include <pion/http/auth.hpp>


namespace pion {    // begin namespace pion
namespace http {    // begin namespace http

///
/// basic_auth: a base class for handling HTTP Authentication and session management
/// in accordance with RFC 2617 http://tools.ietf.org/html/rfc2617 
///
class PION_API basic_auth :
    public http::auth
{
public:
    
    /// default constructor
    basic_auth(user_manager_ptr userManager, const std::string& realm="PION");
    
    /// virtual destructor
    virtual ~basic_auth() {}
    
    /**
     * attempts to validate authentication of a new HTTP request. 
     * If request valid, pointer to user identity object (if any) will be preserved in 
     * the request and return "true". 
     * If request not authenticated, appropriate response is sent over tcp_conn
     * and return "false";
     *
     * @param http_request_ptr the new HTTP request to handle
     * @param tcp_conn the TCP connection that has the new request
     *
     * @return true if request valid and user identity inserted into request 
     */
    virtual bool handle_request(const http::request_ptr& http_request_ptr, const tcp::connection_ptr& tcp_conn);
    
    /**
     * sets a configuration option
     * Valid options:
     *    - "domain" - name of authentication domain
     *
     * @param name the name of the option to change
     * @param value the value of the option
     */
    virtual void set_option(const std::string& name, const std::string& value);

    
protected:

    /**
     * used to send responses when access to resource is not authorized
     *
     * @param http_request_ptr the new HTTP request to handle
     * @param tcp_conn the TCP connection that has the new request
     */
    void handle_unauthorized(const http::request_ptr& http_request_ptr, const tcp::connection_ptr& tcp_conn);
    
    /**
     * extracts base64 user credentials from authorization string
     *
     * @param authorization value of the HEADER_AUTHORIZATION
     */
    static bool parse_authorization(std::string const &authorization, std::string &credentials);
    
    /**
     * parse base64 credentials and extract username/password
     */
    static bool parse_credentials(std::string const &credentials, std::string &username, std::string &password);

    
private:
    
    /// number of seconds after which entires in the user cache will be expired
    static const unsigned int   CACHE_EXPIRATION;


    /// authentication realm ( "PION" by default)
    std::string                 m_realm; 

    /// time of the last cache clean up
    boost::posix_time::ptime    m_cache_cleanup_time;
        
    /// cache of users that are currently active
    user_cache_type             m_user_cache;
    
    /// mutex used to protect access to the user cache
    mutable std::mutex        m_cache_mutex;
};

    
}   // end namespace http
}   // end namespace pion

#endif
