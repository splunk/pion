// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PIONUSER_HEADER__
#define __PION_PIONUSER_HEADER__

#include <map>
#include <string>
#include <cstring>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>

#ifdef PION_HAVE_SSL
    #include <openssl/sha.h>
#endif

namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)


///
/// PionUser: base class to store user credentials
///
class PionUser :
	private boost::noncopyable
{
public:

	/// construct a new PionUser object
	PionUser(std::string const &username, std::string const &password) :
		m_username(username)
	{
		setPassword(password);
	}
	
	/// virtual destructor
	virtual ~PionUser() {}
	
	/// returns user name as a string
	std::string const & getUsername() const { return m_username; }
	
	/**
	 * matches password credential for given user
	 *
	 * @param password password credentials
	 */
	virtual bool matchPassword(const std::string& password) const {
#ifdef PION_HAVE_SSL
		unsigned char sha1_hash[SHA_DIGEST_LENGTH];
		SHA1(reinterpret_cast<const unsigned char *>(password.data()), password.size(), sha1_hash);
		return (memcmp(sha1_hash, m_password_hash, SHA_DIGEST_LENGTH) == 0);
#else
		return m_password == password;
#endif
	}
	
	/// sets password credentials for given user
	virtual void setPassword(const std::string& password) { 
#ifdef PION_HAVE_SSL
		SHA1((const unsigned char *)password.data(), password.size(), m_password_hash);
#else
		m_password = password; 
#endif
	}
	
protected:

	/// username string
	const std::string   m_username;
	
#ifdef PION_HAVE_SSL
	/// SHA1 hash of the password
	unsigned char       m_password_hash[SHA_DIGEST_LENGTH];
#else
	/// password string (actual contents depends on implementation)
	std::string         m_password;
#endif
};

/// data type for a PionUser  pointer
typedef boost::shared_ptr<PionUser>	PionUserPtr;


///
/// PionUserManager base class for PionUser container/manager
///
class PionUserManager :
	private boost::noncopyable
{
public:

	/// construct a new PionUserManager object
	PionUserManager(void) {}
	
	/// virtual destructor
	virtual ~PionUserManager() {}
	
	/**
	 * used to add a new user
	 *
	 * @return false if user with such a name already exists
	 */
	virtual bool addUser(const std::string &username, const std::string &password) {
		boost::mutex::scoped_lock lock(m_mutex);
		UserMap::iterator i = m_users.find(username);
		if (i!=m_users.end())
			return false;
		PionUserPtr user(new PionUser(username,password));
		m_users.insert(std::make_pair(username, user));
		return true;
	}
	
	/**
	 * used to remove given user 
	 *
	 * @return false if no user with such username
	 */
	virtual bool removeUser(const std::string &username) {
		boost::mutex::scoped_lock lock(m_mutex);
		UserMap::iterator i = m_users.find(username);
		if (i==m_users.end())
			return false;
		m_users.erase(i);
		return true;
	}
	
	/**
	 * Used to locate user object by username
	 */
	virtual PionUserPtr getUser(const std::string &username) {
		boost::mutex::scoped_lock lock(m_mutex);
		UserMap::const_iterator i = m_users.find(username);
		if (i==m_users.end())
			return PionUserPtr();
		else
			return i->second;
	}
	
	/**
	 * Used to locate user object by username and password
	 */
	virtual PionUserPtr getUser(const std::string& username, const std::string& password) {
		boost::mutex::scoped_lock lock(m_mutex);
		UserMap::const_iterator i = m_users.find(username);
		if (i==m_users.end() || !i->second->matchPassword(password))
			return PionUserPtr();
		else
			return i->second;
	}

	
protected:

	/// data type for a map of usernames to user objects
	typedef std::map<std::string, PionUserPtr>	UserMap;
	
	
	/// mutex used to protect access to the user list
	mutable boost::mutex		m_mutex;
	
	/// user records container
	UserMap						m_users;
};

/// data type for a PionUserManager pointer
typedef boost::shared_ptr<PionUserManager>	PionUserManagerPtr;


}	// end namespace net
}	// end namespace pion

#endif
