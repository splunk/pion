// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTPAUTH_HEADER__
#define __PION_HTTPAUTH_HEADER__

#include <set>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <pion/config.hpp>
#include <pion/logger.hpp>
#include <pion/exception.hpp>
#include <pion/tcp/connection.hpp>
#include <pion/http/user.hpp>
#include <pion/http/request.hpp>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)

///
/// HTTPAuth: a base class for handling HTTP Authentication and session management
///
class PION_NET_API HTTPAuth :
	private boost::noncopyable
{
public:
	
	/// exception thrown if the service does not recognize a configuration option
	class UnknownOptionException : public PionException {
	public:
		UnknownOptionException(const std::string& name)
			: PionException("Option not recognized by authentication service: ", name) {}
	};
	
	
	/// default constructor
	HTTPAuth(PionUserManagerPtr userManager) 
		: m_logger(PION_GET_LOGGER("pion.net.HTTPAuth")),
		m_user_manager(userManager)
	{}
	
	/// virtual destructor
	virtual ~HTTPAuth() {}
	
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
	virtual bool handleRequest(HTTPRequestPtr& request, TCPConnectionPtr& tcp_conn) = 0;
	
	/**
	 * sets a configuration option
	 *
	 * @param name the name of the option to change
	 * @param value the value of the option
	 */
	virtual void setOption(const std::string& name, const std::string& value) {
		throw UnknownOptionException(name);
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
	typedef std::set<std::string>	AuthResourceSet;

	
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
	bool findResource(const AuthResourceSet& resource_set,
					  const std::string& resource) const;

	/// sets the logger to be used
	inline void setLogger(PionLogger log_ptr) { m_logger = log_ptr; }
	

	/// primary logging interface used by this class
	mutable PionLogger				m_logger;
	
	/// container used to manager user objects
	PionUserManagerPtr			m_user_manager;
	
	/// collection of resources that require authentication 
	AuthResourceSet				m_restrict_list;

	/// collection of resources that do NOT require authentication 
	AuthResourceSet				m_white_list;

	/// mutex used to protect access to the resources
	mutable boost::mutex		m_resource_mutex;
};

/// data type for a HTTPAuth pointer
typedef boost::shared_ptr<HTTPAuth>	HTTPAuthPtr;


}	// end namespace net
}	// end namespace pion

#endif
