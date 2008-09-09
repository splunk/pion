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
#endif


namespace pion {	// begin namespace pion


// static members of PionAdminRights

const boost::int16_t			PionAdminRights::ADMIN_USER_ID = 0;
boost::mutex					PionAdminRights::m_mutex;


// PionAdminRights member functions

PionAdminRights::PionAdminRights(void)
	: m_logger(PION_GET_LOGGER("pion.PionAdminRights")),
	m_lock(m_mutex), m_user_id(-1), m_has_rights(false)
{
#ifndef _MSC_VER
	m_user_id = geteuid();
	if ( seteuid(ADMIN_USER_ID) != 0 ) {
		PION_LOG_ERROR(m_logger, "Unable to upgrade to administrative rights");
		m_lock.unlock();
		return;
	} else {
		m_has_rights = true;
		PION_LOG_DEBUG(m_logger, "Upgraded to administrative rights");
	}
#endif
}

void PionAdminRights::release(void)
{
#ifndef _MSC_VER
	if (m_has_rights) {
		if ( seteuid(m_user_id) == 0 ) {
			PION_LOG_DEBUG(m_logger, "Released administrative rights");
		} else {
			PION_LOG_ERROR(m_logger, "Unable to release administrative rights");
		}
		m_has_rights = false;
		m_lock.unlock();
	}
#endif
}

	
}	// end namespace pion

