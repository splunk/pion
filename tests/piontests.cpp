// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <iostream>
#include <pion/config.hpp>
#include <pion/logger.hpp>

#define BOOST_TEST_MODULE pion-net-unit-tests
#include <boost/test/unit_test.hpp>

#include <pion/unit_test_defs.hpp>

struct PionNetUnitTestsConfig {
    PionNetUnitTestsConfig() {
        std::cout << "global setup specific to pion-net\n";

        // argc and argv do not include parameters handled by the boost unit test framework, such as --log_level.
        int argc = boost::unit_test::framework::master_test_suite().argc;
        char** argv = boost::unit_test::framework::master_test_suite().argv;

        std::cout << "argc = " << argc << std::endl;
        for (int i = 0; i < argc; ++i)
            std::cout << "argv[" << i << "] = " << argv[i] << std::endl;
    }
    ~PionNetUnitTestsConfig() {
        std::cout << "global teardown specific to pion-net\n";
    }
};

BOOST_GLOBAL_FIXTURE(PionUnitTestsConfig);
BOOST_GLOBAL_FIXTURE(PionNetUnitTestsConfig);
