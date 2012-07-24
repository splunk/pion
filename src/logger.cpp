// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/logger.hpp>

namespace pion {	// begin namespace pion


// static members of PionLogger
#if defined(PION_USE_OSTREAM_LOGGING)
PionLogger::PionPriorityType	PionLogger::m_priority = PionLogger::LOG_LEVEL_INFO;
#endif

	
}	// end namespace pion
