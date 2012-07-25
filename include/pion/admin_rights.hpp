// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PIONADMINRIGHTS_HEADER__
#define __PION_PIONADMINRIGHTS_HEADER__

#include <pion/config.hpp>
#include <pion/logger.hpp>
#include <boost/cstdint.hpp>
#include <boost/thread/mutex.hpp>


namespace pion {    // begin namespace pion


///
/// PionAdminRights: obtains administrative rights for the process
///
class PION_COMMON_API PionAdminRights {
public:

    /**
     * constructs object, obtaining administrative rights; will block
     * if another thread has already obtained rights
     *
     * @param use_log if false, then no logging will be performed
     */
    PionAdminRights(bool use_log = true);

    /// destructor releases administrative rights
    virtual ~PionAdminRights() { release(); }

    /// releases administrative rights
    void release(void);

    /// calculates the user id based upon the user configuration parameter
    static long runAsUser(const std::string& user_name);
    
    /// calculates the group id based upon the group configuration parameter
    static long runAsGroup(const std::string& group_name);


private:

    /**
     * finds an identifier within a system credentials file (users or groups)
     *
     * @param name descriptive name to lookup (user or group name, may be id)
     * @param file system credentials file to look within
     *
     * @return boost::int32_t identifier found, or -1 if none found
     */
    static long findSystemId(const std::string& name, const std::string& file);


    /// adminisitrator or root user identifier
    static const boost::int16_t         ADMIN_USER_ID;

    /// mutex used to prevent multiple threads from corrupting user id
    static boost::mutex                 m_mutex;

    /// primary logging interface used by this class        
    PionLogger                          m_logger;

    /// lock used to prevent multiple threads from corrupting user id
    boost::unique_lock<boost::mutex>    m_lock;

    /// saved user identifier before upgrading to administrator
    boost::int16_t                      m_user_id;

    /// true if the class currently holds administrative rights
    bool                                m_has_rights;

    /// if false, then no logging will be performed
    bool                                m_use_log;
};


}   // end namespace pion

#endif

