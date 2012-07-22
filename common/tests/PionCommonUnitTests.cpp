// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <iostream>
#include <pion/PionConfig.hpp>

#define BOOST_TEST_MODULE pion-common-unit-tests
#include <boost/test/unit_test.hpp>

#include <pion/PionUnitTestDefs.hpp>

struct PionCommonUnitTestsConfig {
	PionCommonUnitTestsConfig() {
		std::cout << "global setup specific to pion-common\n";

		// argc and argv do not include parameters handled by the boost unit test framework, such as --log_level.
		int argc = boost::unit_test::framework::master_test_suite().argc;
		char** argv = boost::unit_test::framework::master_test_suite().argv;

		std::cout << "argc = " << argc << std::endl;
		for (int i = 0; i < argc; ++i)
			std::cout << "argv[" << i << "] = " << argv[i] << std::endl;
	}
	~PionCommonUnitTestsConfig() {
		std::cout << "global teardown specific to pion-common\n";
	}
};

BOOST_GLOBAL_FIXTURE(PionUnitTestsConfig);
BOOST_GLOBAL_FIXTURE(PionCommonUnitTestsConfig);
