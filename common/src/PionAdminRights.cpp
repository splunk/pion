// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/PionAdminRights.hpp>

#ifndef _MSC_VER
	#include <sys/types.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <boost/regex.hpp>
	#include <boost/tokenizer.hpp>
	#include <boost/lexical_cast.hpp>
	#include <fstream>
#endif


namespace pion {	// begin namespace pion


// static members of PionAdminRights

const boost::int16_t			PionAdminRights::ADMIN_USER_ID = 0;
boost::mutex					PionAdminRights::m_mutex;


// PionAdminRights member functions

#ifdef _MSC_VER

PionAdminRights::PionAdminRights(bool use_log)
	: m_logger(PION_GET_LOGGER("pion.PionAdminRights")),
	m_lock(m_mutex), m_user_id(-1), m_has_rights(false), m_use_log(use_log)
{}

void PionAdminRights::release(void)
{}

long PionAdminRights::runAsUser(const std::string& user_name)
{
	return -1;
}

long PionAdminRights::runAsGroup(const std::string& group_name)
{
	return -1;
}

long PionAdminRights::findSystemId(const std::string& name,
	const std::string& file)
{
	return -1;
}

#else	// NOT #ifdef _MSC_VER

PionAdminRights::PionAdminRights(bool use_log)
	: m_logger(PION_GET_LOGGER("pion.PionAdminRights")),
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

void PionAdminRights::release(void)
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

long PionAdminRights::runAsUser(const std::string& user_name)
{
	long user_id = findSystemId(user_name, "/etc/passwd");
	if (user_id != -1) {
		if ( seteuid(user_id) != 0 )
			user_id = -1;
	} else {
		user_id = geteuid();
	}
	return user_id;
}

long PionAdminRights::runAsGroup(const std::string& group_name)
{
	long group_id = findSystemId(group_name, "/etc/group");
	if (group_id != -1) {
		if ( setegid(group_id) != 0 )
			group_id = -1;
	} else {
		group_id = getegid();
	}
	return group_id;
}

long PionAdminRights::findSystemId(const std::string& name,
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

#endif	// #ifdef _MSC_VER
	
}	// end namespace pion

