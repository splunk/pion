// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTP_AUTH_HEADER__
#define __PION_HTTP_AUTH_HEADER__

#include <set>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <pion/config.hpp>
#include <pion/error.hpp>
#include <pion/logger.hpp>
#include <pion/hash_map.hpp>
#include <pion/tcp/connection.hpp>
#include <pion/http/user.hpp>
#include <pion/http/request.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>    // order important, otherwise compiling error under win32


namespace pion {    // begin namespace pion
namespace http {    // begin namespace http


///
/// auth: a base class for handling HTTP Authentication and session management
///
class PION_API auth :
    private boost::noncopyable
{
public:
    
    /// default constructor
    auth(PionUserManagerPtr userManager) 
        : m_logger(PION_GET_LOGGER("pion.http.auth")),
        m_user_manager(userManager)
    {}
    
    /// virtual destructor
    virtual ~auth() {}
    
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
    virtual bool handleRequest(HTTPRequestPtr& request, tcp::connection_ptr& tcp_conn) = 0;
    
    /**
     * sets a configuration option
     *
     * @param name the name of the option to change
     * @param value the value of the option
     */
    virtual void setOption(const std::string& name, const std::string& value) {
        BOOST_THROW_EXCEPTION( error::bad_arg() << error::errinfo_arg_name(name) );
    }
    
    /**
     * adds a resource that requires authentication
     *
     * @param resource the resource name or uri-stem that requires authentication
     */
    void addRestrict(const std::string& resource);
    
    /**
     * adds a resource that does NOT require authentication
     *
     * @param resource the resource name or uri-stem that does not require authentication
     */
    void addPermit(const std::string& resource);

    /**
     * used to add a new user
     *
     * @ return false if user with such name already exists
     */
    virtual bool addUser(std::string const &username, std::string const &password) {
        return m_user_manager->addUser(username, password);
    }
    
    /**
     * update password for given user
     *
     * @return false if user with such a name doesn't exist
     */
    virtual bool updateUser(std::string const &username, std::string const &password) {
        return m_user_manager->updateUser(username, password);
    }
    
    /**
     * used to remove given user 
     *
     * @return false if no user with such username
     */
    virtual bool removeUser(std::string const &username) {
        return m_user_manager->removeUser(username);
    };
    
    /**
     * Used to locate user object by username
     */
    virtual PionUserPtr getUser(std::string const &username) {
        return m_user_manager->getUser(username);
    }

    
protected:

    /// data type for a set of resources to be authenticated
    typedef std::set<std::string>   resource_set_type;

    /// data type used to map authentication credentials to PionUser objects
    typedef PION_HASH_MAP<std::string,std::pair<boost::posix_time::ptime,PionUserPtr> >  user_cache_type;
    
    
    /**
     * check if given HTTP request requires authentication
     *
     * @param http_request the HTTP request to check
     */
    bool needAuthentication(HTTPRequestPtr const& http_request) const;
    
    /**
     * tries to find a resource in a given collection
     * 
     * @param resource_set the collection of resource to look in
     * @param resource the resource to look for
     *
     * @return true if the resource was found
     */
    bool findResource(const resource_set_type& resource_set,
                      const std::string& resource) const;

    /// sets the logger to be used
    inline void setLogger(logger log_ptr) { m_logger = log_ptr; }
    

    /// primary logging interface used by this class
    mutable logger              m_logger;
    
    /// container used to manager user objects
    PionUserManagerPtr          m_user_manager;
    
    /// collection of resources that require authentication 
    resource_set_type             m_restrict_list;

    /// collection of resources that do NOT require authentication 
    resource_set_type             m_white_list;

    /// mutex used to protect access to the resources
    mutable boost::mutex        m_resource_mutex;
};

/// data type for a auth pointer
typedef boost::shared_ptr<auth> auth_ptr;


}   // end namespace http
}   // end namespace pion

#endif
