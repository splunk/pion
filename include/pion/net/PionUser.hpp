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
#include <pion/PionConfig.hpp>
#include <pion/PionException.hpp>

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

	/// exception thrown if a bad password hash is given to setPasswordHash()
	class BadPasswordHash : public std::exception {
	public:
		virtual const char* what() const throw() {
			return "Invalid password hash provided";
		}
	};


	/// construct a new PionUser object
	PionUser(std::string const &username) :
		m_username(username)
	{}
	
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
	
	/// returns password for the user (encrypted if SSL is enabled)
	std::string const & getPassword() const { return m_password; }
	
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
		// store encrypted hash value
		SHA1((const unsigned char *)password.data(), password.size(), m_password_hash);
		m_password_hash[SHA_DIGEST_LENGTH] = '\0';

		// update password string (convert binary to hex)
		m_password.clear();
		char buf[3];
		buf[2] = '\0';
		for (unsigned int n = 0; n < SHA_DIGEST_LENGTH; ++n) {
			sprintf(buf, "%2X", static_cast<unsigned int>(m_password_hash[n]));
			m_password += buf;
		}
#else
		m_password = password; 
#endif
	}

#ifdef PION_HAVE_SSL
	/// sets encrypted password credentials for given user
	virtual void setPasswordHash(const std::string& password_hash) {
		// update password string representation
		if (password_hash.size() != SHA_DIGEST_LENGTH*2)
			throw BadPasswordHash();
		m_password = password_hash;

		// convert string from hex to binary value
		char buf[3];
		buf[2] = '\0';
		unsigned int hash_pos = 0;
		std::string::iterator str_it = m_password.begin();
		while (str_it != m_password.end()) {
			buf[0] = *str_it;
			++str_it;
			buf[1] = *str_it;
			++str_it;
			m_password_hash[hash_pos++] = strtoul(buf, 0, 16);
		}
	}
#endif

	
protected:

	/// username string
	const std::string   m_username;
	
	/// password string (actual contents depends on implementation)
	std::string         m_password;

#ifdef PION_HAVE_SSL
	/// SHA1 hash of the password
	unsigned char       m_password_hash[SHA_DIGEST_LENGTH];
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
	 * used to add a new user with plaintext password
	 *
	 * @param username name or identifier of the user to add
	 * @param password plaintext password of the user to add
	 *
	 * @return false if user with such a name already exists
	 */
	virtual bool addUser(const std::string &username,
		const std::string &password)
	{
		boost::mutex::scoped_lock lock(m_mutex);
		UserMap::iterator i = m_users.find(username);
		if (i!=m_users.end())
			return false;
		PionUserPtr user(new PionUser(username, password));
		m_users.insert(std::make_pair(username, user));
		return true;
	}
	
	/**
	 * update password for given user
	 *
	 * @param username name or identifier of the user to update
	 * @param password plaintext password of the user to update
	 *
	 * @return false if user with such a name doesn't exist
	 */
	virtual bool updateUser(const std::string &username,
		const std::string &password)
	{
		boost::mutex::scoped_lock lock(m_mutex);
		UserMap::iterator i = m_users.find(username);
		if (i==m_users.end())
			return false;
		i->second->setPassword(password);
		return true;
	}

#ifdef PION_HAVE_SSL
	/**
	 * used to add a new user with encrypted password
	 *
	 * @param username name or identifier of the user to add
	 * @param password_hash encrypted password of the user to add
	 *
	 * @return false if user with such a name already exists
	 */
	virtual bool addUserHash(const std::string &username,
		const std::string &password_hash)
	{
		boost::mutex::scoped_lock lock(m_mutex);
		UserMap::iterator i = m_users.find(username);
		if (i!=m_users.end())
			return false;
		PionUserPtr user(new PionUser(username));
		user->setPasswordHash(password_hash);
		m_users.insert(std::make_pair(username, user));
		return true;
	}
	
	/**
	 * update password for given user with encrypted password
	 *
	 * @param username name or identifier of the user to update
	 * @param password_hash encrypted password of the user to update
	 *
	 * @return false if user with such a name doesn't exist
	 */
	virtual bool updateUserHash(const std::string &username,
		const std::string &password_hash)
	{
		boost::mutex::scoped_lock lock(m_mutex);
		UserMap::iterator i = m_users.find(username);
		if (i==m_users.end())
			return false;
		i->second->setPasswordHash(password_hash);
		return true;
	}
#endif

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
