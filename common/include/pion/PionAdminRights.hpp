// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PIONADMINRIGHTS_HEADER__
#define __PION_PIONADMINRIGHTS_HEADER__

#include <pion/PionConfig.hpp>
#include <pion/PionLogger.hpp>
#include <boost/cstdint.hpp>
#include <boost/thread/mutex.hpp>


namespace pion {	// begin namespace pion


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


private:

	/// adminisitrator or root user identifier
	static const boost::int16_t			ADMIN_USER_ID;

	/// mutex used to prevent multiple threads from corrupting user id
	static boost::mutex					m_mutex;

	/// primary logging interface used by this class        
	PionLogger							m_logger;

	/// lock used to prevent multiple threads from corrupting user id
	boost::unique_lock<boost::mutex>	m_lock;

	/// saved user identifier before upgrading to administrator
	boost::int16_t						m_user_id;

	/// true if the class currently holds administrative rights
	bool								m_has_rights;

	/// if false, then no logging will be performed
	bool								m_use_log;
};


}	// end namespace pion

#endif

