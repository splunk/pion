// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/logger.hpp>

namespace pion {    // begin namespace pion


// static members of logger
#if defined(PION_USE_OSTREAM_LOGGING)
logger::log_priority_type    logger::m_priority = logger::LOG_LEVEL_INFO;
#endif

    
}   // end namespace pion
