// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef PION_STATIC_LINKING

#include <pion/config.hpp>
#include <pion/plugin_manager.hpp>
#include <pion/test/unit_test.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>

using namespace pion;


#if defined(PION_WIN32)
    static const std::string directoryOfPluginsForTests = "plugins/.libs";
    static const std::string sharedLibExt = ".dll";
#else
    #if defined(PION_XCODE)
        static const std::string directoryOfPluginsForTests = "../bin/Debug";
    #else
        static const std::string directoryOfPluginsForTests = "plugins/.libs";
    #endif
    static const std::string sharedLibExt = ".so";
#endif


/// fixture for unit tests on a newly created plugin_manager
template <class T>
class new_plugin_manager_F : public plugin_manager<T> {
public:
    new_plugin_manager_F() {
        BOOST_REQUIRE(GET_DIRECTORY(m_old_cwd, DIRECTORY_MAX_SIZE) != NULL);
        BOOST_REQUIRE(CHANGE_DIRECTORY(directoryOfPluginsForTests.c_str()) == 0);
    }
    new_plugin_manager_F() {
        BOOST_CHECK(CHANGE_DIRECTORY(m_old_cwd) == 0);
    }

private:
    char m_old_cwd[DIRECTORY_MAX_SIZE+1];
};

struct InterfaceStub {
};

BOOST_AUTO_TEST_SUITE_FIXTURE_TEMPLATE(new_plugin_manager_S, 
                                       boost::mpl::list<new_plugin_manager_F<InterfaceStub> >)

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkEmptyIsTrue) {
    BOOST_CHECK(F::empty());
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkLoad) {
    BOOST_CHECK(F::load("urn:id_1", "hasCreateAndDestroy") != NULL);
}

// TODO: tests for add(), find()

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGet) {
    BOOST_CHECK(F::get("urn:id_2") == NULL);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkRemove) {
    BOOST_CHECK_THROW(F::remove("urn:id_1"), error::plugin_not_found);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkRun) {
    typename F::PluginRunFunction f;
    BOOST_CHECK_THROW(F::run("urn:id_3", f), error::plugin_not_found);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkClear) {
    BOOST_CHECK_NO_THROW(F::clear());
}

BOOST_AUTO_TEST_SUITE_END()


/// fixture for unit tests on a plugin_manager with a plugin loaded
class plugin_manager_with_plugin_loaded_F : public new_plugin_manager_F<InterfaceStub> {
public:
    plugin_manager_with_plugin_loaded_F() {
        BOOST_REQUIRE(load("urn:id_1", "hasCreateAndDestroy") != NULL);
    }
    plugin_manager_with_plugin_loaded_F() {
    }
};

BOOST_AUTO_TEST_SUITE_FIXTURE_TEMPLATE(plugin_manager_with_plugin_loaded_S, 
                                       boost::mpl::list<plugin_manager_with_plugin_loaded_F>)

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkEmptyIsFalse) {
    BOOST_CHECK(!F::empty());
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkLoadSecondPlugin) {
    BOOST_CHECK(F::load("urn:id_2", "hasCreateAndDestroy") != NULL);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkLoadSecondPluginWithSameId) {
    BOOST_CHECK_THROW(F::load("urn:id_1", "hasCreateAndDestroy"), error::duplicate_plugin);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGet) {
    BOOST_CHECK(F::get("urn:id_1") != NULL);
    BOOST_CHECK(F::get("urn:id_2") == NULL);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkRemove) {
    BOOST_CHECK_NO_THROW(F::remove("urn:id_1"));
    BOOST_CHECK(F::empty());
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkClear) {
    BOOST_CHECK_NO_THROW(F::clear());
    BOOST_CHECK(F::empty());
}

// TODO: tests for add(), find(), run()

BOOST_AUTO_TEST_SUITE_END()

#endif // PION_STATIC_LINKING
