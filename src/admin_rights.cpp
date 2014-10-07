// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/admin_rights.hpp>

#ifndef _MSC_VER
    #include <sys/types.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <boost/regex.hpp>
    #include <boost/tokenizer.hpp>
    #include <boost/lexical_cast.hpp>
    #include <fstream>
#endif


namespace pion {    // begin namespace pion


// static members of admin_rights

const boost::int16_t    admin_rights::ADMIN_USER_ID = 0;
boost::mutex            admin_rights::m_mutex;


// admin_rights member functions

#ifdef _MSC_VER

admin_rights::admin_rights(bool use_log)
    : m_logger(PION_GET_LOGGER("pion.admin_rights")),
    m_lock(m_mutex), m_user_id(-1), m_has_rights(false), m_use_log(use_log)
{}

void admin_rights::release(void)
{}

long admin_rights::run_as_user(const std::string& /* user_name */)
{
    return -1;
}

long admin_rights::run_as_group(const std::string& /* group_name */)
{
    return -1;
}

long admin_rights::find_system_id(const std::string& /* name */,
    const std::string& /* file */)
{
    return -1;
}

#else   // NOT #ifdef _MSC_VER

admin_rights::admin_rights(bool use_log)
    : m_logger(PION_GET_LOGGER("pion.admin_rights")),
    m_lock(m_mutex), m_user_id(-1), m_has_rights(false), m_use_log(use_log)
{
    m_user_id = geteuid();
    if ( seteuid(ADMIN_USER_ID) != 0 ) {
        if (m_use_log)
            PION_LOG_ERROR(m_logger, "Unable to upgrade to administrative rights");
        m_lock.unlock();
        return;
    } else {
        m_has_rights = true;
        if (m_use_log)
            PION_LOG_DEBUG(m_logger, "Upgraded to administrative rights");
    }
}

void admin_rights::release(void)
{
    if (m_has_rights) {
        if ( seteuid(m_user_id) == 0 ) {
            if (m_use_log)
                PION_LOG_DEBUG(m_logger, "Released administrative rights");
        } else {
            if (m_use_log)
                PION_LOG_ERROR(m_logger, "Unable to release administrative rights");
        }
        m_has_rights = false;
        m_lock.unlock();
    }
}

long admin_rights::run_as_user(const std::string& user_name)
{
    long user_id = find_system_id(user_name, "/etc/passwd");
    if (user_id != -1) {
        if ( seteuid(user_id) != 0 )
            user_id = -1;
    } else {
        user_id = geteuid();
    }
    return user_id;
}

long admin_rights::run_as_group(const std::string& group_name)
{
    long group_id = find_system_id(group_name, "/etc/group");
    if (group_id != -1) {
        if ( setegid(group_id) != 0 )
            group_id = -1;
    } else {
        group_id = getegid();
    }
    return group_id;
}

long admin_rights::find_system_id(const std::string& name,
    const std::string& file)
{
    // check if name is the system id
    const boost::regex just_numbers("\\d+");
    if (boost::regex_match(name, just_numbers)) {
        return boost::lexical_cast<boost::int32_t>(name);
    }

    // open system file
    std::ifstream system_file(file.c_str());
    if (! system_file.is_open()) {
        return -1;
    }

    // find id in system file
    typedef boost::tokenizer<boost::char_separator<char> > Tok;
    boost::char_separator<char> sep(":");
    std::string line;
    boost::int32_t system_id = -1;

    while (std::getline(system_file, line, '\n')) {
        Tok tokens(line, sep);
        Tok::const_iterator token_it = tokens.begin();
        if (token_it != tokens.end() && *token_it == name) {
            // found line matching name
            if (++token_it != tokens.end() && ++token_it != tokens.end()
                && boost::regex_match(*token_it, just_numbers))
            {
                // found id as third parameter
                system_id = boost::lexical_cast<boost::int32_t>(*token_it);
            }
            break;
        }
    }

    return system_id;
}

#endif  // #ifdef _MSC_VER
    
}   // end namespace pion

