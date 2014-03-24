// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/config.hpp>
#include <pion/test/unit_test.hpp>
#include <boost/test/unit_test.hpp>
#include <pion/process.hpp>

using namespace pion;

class ProcessTest_F : public process
{
public:
};

BOOST_FIXTURE_TEST_SUITE(ProcessTest_S, ProcessTest_F)

#ifdef _MSC_VER
BOOST_AUTO_TEST_CASE(checkSetDumpfileDirectory)
{
    // check that invalid input results in an exception
    BOOST_CHECK_THROW(set_dumpfile_directory("::invalid_dir_name::"), dumpfile_init_exception);

    // grab temp path
    char temp_path[MAX_PATH];
    ::GetTempPath(MAX_PATH, temp_path);

    // check that the call succeeds with valid parameters
    BOOST_CHECK_NO_THROW(set_dumpfile_directory(temp_path));

    // check some expected post-conditions
    config_type& cfg = get_config();

    // dump file directory must be the one we just set
    BOOST_CHECK(cfg.dumpfile_dir == temp_path);

    // MiniDumpWriteDump proc address must be not null after successfull call
    BOOST_CHECK(cfg.p_dump_proc != NULL);
}

BOOST_AUTO_TEST_CASE(checkResetDumpfileDirectory)
{
    // check that the call succeeds
    BOOST_CHECK_NO_THROW(set_dumpfile_directory(""));

    // check some expected post-conditions
    config_type& cfg = get_config();

    // dump file directory must be the one we just set
    BOOST_CHECK(cfg.dumpfile_dir == "");

    // MiniDumpWriteDump proc address must be null
    BOOST_CHECK(cfg.p_dump_proc == NULL);
}

BOOST_AUTO_TEST_CASE(checkGenerateDumpFileName)
{
    // grab temp path
    char temp_path[MAX_PATH];
    ::GetTempPath(MAX_PATH, temp_path);

    // set dumpfile directory
    BOOST_CHECK_NO_THROW(set_dumpfile_directory(temp_path));

    std::string dumpfile_name = generate_dumpfile_name();

    BOOST_ASSERT(!dumpfile_name.empty());

    // temp file must be in a specified directory
    BOOST_ASSERT(dumpfile_name.find(temp_path) != std::string::npos);
}
#endif

BOOST_AUTO_TEST_SUITE_END()
