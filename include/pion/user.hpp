// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_USER_HEADER__
#define __PION_USER_HEADER__

#include <map>
#include <string>
#include <cstdio>
#include <cstring>
#include <boost/noncopyable.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <pion/config.hpp>
#include <pion/error.hpp>
#include <pion/stdx/mutex.hpp>
#include <pion/stdx/memory.hpp>

#ifdef PION_HAVE_SSL
    #if defined(__APPLE__)
        // suppress warnings about OpenSSL being deprecated in OSX
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    #endif
    #include <openssl/sha.h>
#endif


namespace pion {    // begin namespace pion


///
/// user: base class to store user credentials
///
class user :
    private boost::noncopyable
{
public:

    /// construct a new user object
    user(std::string const &username) :
        m_username(username)
#ifdef PION_HAVE_SSL
        , m_password_hash_type(EMPTY)
#endif
    {}

    /// construct a new user object
    user(std::string const &username, std::string const &password) :
        m_username(username)
#ifdef PION_HAVE_SSL
        , m_password_hash_type(EMPTY)
#endif
    {
        set_password(password);
    }

    /// virtual destructor
    virtual ~user() {}

    /// returns user name as a string
    std::string const & get_username() const { return m_username; }

    /// returns password for the user (encrypted if SSL is enabled)
    std::string const & get_password() const { return m_password; }

    /**
     * matches password credential for given user
     *
     * @param password password credentials
     */
    virtual bool match_password(const std::string& password) const {
#ifdef PION_HAVE_SSL
        if (m_password_hash_type == SHA_256) {
            unsigned char sha256_hash[SHA256_DIGEST_LENGTH];
            SHA256(reinterpret_cast<const unsigned char *>(password.data()), password.size(), sha256_hash);
            return (memcmp(sha256_hash, m_password_hash, SHA256_DIGEST_LENGTH) == 0);
        } else if (m_password_hash_type == SHA_1) {
            unsigned char sha1_hash[SHA_DIGEST_LENGTH];
            SHA1(reinterpret_cast<const unsigned char *>(password.data()), password.size(), sha1_hash);
            return (memcmp(sha1_hash, m_password_hash, SHA_DIGEST_LENGTH) == 0);
        } else
            return false;
#else
        return m_password == password;
#endif
    }

    /// sets password credentials for given user
    virtual void set_password(const std::string& password) { 
#ifdef PION_HAVE_SSL
        // store encrypted hash value
        SHA256((const unsigned char *)password.data(), password.size(), m_password_hash);
        m_password_hash_type = SHA_256;

        // update password string (convert binary to hex)
        m_password.clear();
        char buf[3];
        for (unsigned int n = 0; n < SHA256_DIGEST_LENGTH; ++n) {
            sprintf(buf, "%.2x", static_cast<unsigned int>(m_password_hash[n]));
            m_password += buf;
        }
#else
        m_password = password; 
#endif
    }

#ifdef PION_HAVE_SSL
    /// sets encrypted password credentials for given user
    virtual void set_password_hash(const std::string& password_hash) {
        // update password string representation
        if (password_hash.size() == SHA256_DIGEST_LENGTH * 2) {
            m_password_hash_type = SHA_256;
        } else if (password_hash.size() == SHA_DIGEST_LENGTH * 2) {
            m_password_hash_type = SHA_1;
        } else {
            BOOST_THROW_EXCEPTION( error::bad_password_hash() );
        }
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
            m_password_hash[hash_pos++] = boost::numeric_cast<unsigned char>(strtoul(buf, 0, 16));
        }
    }
#endif


protected:

    /// username string
    const std::string   m_username;

    /// password string (actual contents depends on implementation)
    std::string         m_password;

#ifdef PION_HAVE_SSL
    enum password_hash_type_t {EMPTY, SHA_1, SHA_256};

    /// SHA_256 when hash created by set_password, determined by length of hash when hash is given
    password_hash_type_t   m_password_hash_type;

    /// SHA256_DIGEST_LENGTH is sufficient for SHA-256 or SHA-1
    unsigned char          m_password_hash[SHA256_DIGEST_LENGTH];
#endif
};

/// data type for a user  pointer
typedef stdx::shared_ptr<user> user_ptr;


///
/// user_manager base class for user container/manager
///
class user_manager :
    private boost::noncopyable
{
public:

    /// construct a new user_manager object
    user_manager(void) {}

    /// virtual destructor
    virtual ~user_manager() {}

    /// returns true if no users are defined
    inline bool empty(void) const {
        stdx::lock_guard<stdx::mutex> lock(m_mutex);
        return m_users.empty();
    }

    /**
     * used to add a new user with plaintext password
     *
     * @param username name or identifier of the user to add
     * @param password plaintext password of the user to add
     *
     * @return false if user with such a name already exists
     */
    virtual bool add_user(const std::string &username,
        const std::string &password)
    {
        stdx::lock_guard<stdx::mutex> lock(m_mutex);
        user_map_t::iterator i = m_users.find(username);
        if (i!=m_users.end())
            return false;
        user_ptr user_ptr(new user(username, password));
        m_users.insert(std::make_pair(username, user_ptr));
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
    virtual bool update_user(const std::string &username,
        const std::string &password)
    {
        stdx::lock_guard<stdx::mutex> lock(m_mutex);
        user_map_t::iterator i = m_users.find(username);
        if (i==m_users.end())
            return false;
        i->second->set_password(password);
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
    virtual bool add_user_hash(const std::string &username,
        const std::string &password_hash)
    {
        stdx::lock_guard<stdx::mutex> lock(m_mutex);
        user_map_t::iterator i = m_users.find(username);
        if (i!=m_users.end())
            return false;
        user_ptr user_ptr(new user(username));
        user_ptr->set_password_hash(password_hash);
        m_users.insert(std::make_pair(username, user_ptr));
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
    virtual bool update_user_hash(const std::string &username,
        const std::string &password_hash)
    {
        stdx::lock_guard<stdx::mutex> lock(m_mutex);
        user_map_t::iterator i = m_users.find(username);
        if (i==m_users.end())
            return false;
        i->second->set_password_hash(password_hash);
        return true;
    }
#endif

    /**
     * used to remove given user 
     *
     * @return false if no user with such username
     */
    virtual bool remove_user(const std::string &username) {
        stdx::lock_guard<stdx::mutex> lock(m_mutex);
        user_map_t::iterator i = m_users.find(username);
        if (i==m_users.end())
            return false;
        m_users.erase(i);
        return true;
    }

    /**
     * Used to locate user object by username
     */
    virtual user_ptr get_user(const std::string &username) {
        stdx::lock_guard<stdx::mutex> lock(m_mutex);
        user_map_t::const_iterator i = m_users.find(username);
        if (i==m_users.end())
            return user_ptr();
        else
            return i->second;
    }

    /**
     * Used to locate user object by username and password
     */
    virtual user_ptr get_user(const std::string& username, const std::string& password) {
        stdx::lock_guard<stdx::mutex> lock(m_mutex);
        user_map_t::const_iterator i = m_users.find(username);
        if (i==m_users.end() || !i->second->match_password(password))
            return user_ptr();
        else
            return i->second;
    }


protected:

    /// data type for a map of usernames to user objects
    typedef std::map<std::string, user_ptr>  user_map_t;


    /// mutex used to protect access to the user list
    mutable stdx::mutex    m_mutex;

    /// user records container
    user_map_t              m_users;
};

/// data type for a user_manager pointer
typedef stdx::shared_ptr<user_manager>  user_manager_ptr;


}   // end namespace pion

#endif
