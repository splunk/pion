// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/PionLogger.hpp>

namespace pion {	// begin namespace pion


// static members of PionLogger
#if defined(PION_USING_OSTREAM_LOGGING)
PionLogger::PionPriorityType	PionLogger::m_priority = PionLogger::LOG_LEVEL_INFO;
#endif

	
}	// end namespace pion
