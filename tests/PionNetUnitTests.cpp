// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/PionConfig.hpp>
#include <pion/PionLogger.hpp>

#define BOOST_TEST_MODULE pion-net-unit-tests
#include <boost/test/unit_test.hpp>

/// sets up logging (run once only)
void setup_logging_for_unit_tests(void)
{
	static bool first_run = true;
	if (first_run) {
		first_run = false;
		// configure logging
		PION_LOG_CONFIG_BASIC;
		pion::PionLogger log_ptr = PION_GET_LOGGER("pion");
		PION_LOG_SETLEVEL_WARN(log_ptr);
	}
}
