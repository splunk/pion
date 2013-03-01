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

#define BOOST_TEST_MODULE pion-unit-tests
#include <boost/test/unit_test.hpp>

#include <pion/test/unit_test.hpp>

namespace pion {
    namespace test {
        std::ofstream config::m_test_log_file;
        BOOST_GLOBAL_FIXTURE(config);
    }
}
