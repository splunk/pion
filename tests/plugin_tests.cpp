// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef PION_STATIC_LINKING

#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>
#include <pion/config.hpp>
#include <pion/plugin.hpp>
#include <pion/test/unit_test.hpp>
#include <boost/test/unit_test.hpp>
#include "plugins/InterfaceStub.hpp"

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

class EmptyPluginPtr_F : public plugin_ptr<InterfaceStub> {
public:
    EmptyPluginPtr_F() { 
        BOOST_REQUIRE(GET_DIRECTORY(m_old_cwd, DIRECTORY_MAX_SIZE) != NULL);
        BOOST_REQUIRE(CHANGE_DIRECTORY(directoryOfPluginsForTests.c_str()) == 0);
    }
    ~EmptyPluginPtr_F() {
        BOOST_CHECK(CHANGE_DIRECTORY(m_old_cwd) == 0);
    }

private:
    char m_old_cwd[DIRECTORY_MAX_SIZE+1];
};


BOOST_FIXTURE_TEST_SUITE(EmptyPluginPtr_S, EmptyPluginPtr_F)

BOOST_AUTO_TEST_CASE(checkIsOpenReturnsFalse) {
    BOOST_CHECK(!is_open());
}

BOOST_AUTO_TEST_CASE(checkCreateThrowsException) {
    BOOST_CHECK_THROW(create(), error::plugin_undefined);
}

BOOST_AUTO_TEST_CASE(checkDestroyThrowsException) {
    InterfaceStub* s = NULL;
    BOOST_CHECK_THROW(destroy(s), error::plugin_undefined);
}

BOOST_AUTO_TEST_CASE(checkOpenThrowsExceptionForNonExistentPlugin) {
    BOOST_REQUIRE(!boost::filesystem::exists("NoSuchPlugin" + sharedLibExt));
    BOOST_CHECK_THROW(open("NoSuchPlugin"), error::plugin_not_found);
}

BOOST_AUTO_TEST_CASE(checkGetPluginNameReturnsEmptyString) {
    BOOST_CHECK_EQUAL(getPluginName(), "");
}

BOOST_AUTO_TEST_CASE(checkPluginInstancePtrCreate) {
    plugin_instance_ptr<InterfaceStub> m_instance_ptr;
    BOOST_CHECK(m_instance_ptr.empty());
    BOOST_CHECK(m_instance_ptr.get() == NULL);
    BOOST_REQUIRE(boost::filesystem::exists("hasCreateAndDestroy" + sharedLibExt));
    BOOST_CHECK_NO_THROW(m_instance_ptr.create("hasCreateAndDestroy"));
    BOOST_CHECK(! m_instance_ptr.empty());
    BOOST_CHECK(m_instance_ptr.get() != NULL);
}

BOOST_AUTO_TEST_CASE(checkPluginInstancePtrDereferencing) {
    plugin_instance_ptr<InterfaceStub> m_instance_ptr;
    BOOST_CHECK_NO_THROW(m_instance_ptr.create("hasCreateAndDestroy"));
    const plugin_instance_ptr<InterfaceStub>& const_ref = m_instance_ptr;
    InterfaceStub &a = *m_instance_ptr;
    const InterfaceStub &b = *const_ref;
    a.method();
    b.const_method();
    m_instance_ptr->method();
    const_ref->const_method();
}

BOOST_AUTO_TEST_CASE(checkOpenThrowsExceptionForNonPluginDll) {
    BOOST_REQUIRE(boost::filesystem::exists("hasNoCreate" + sharedLibExt));
    BOOST_CHECK_THROW(open("hasNoCreate"), error::plugin_missing_symbol);
}

BOOST_AUTO_TEST_CASE(checkOpenThrowsExceptionForPluginWithoutDestroy) {
    BOOST_REQUIRE(boost::filesystem::exists("hasCreateButNoDestroy" + sharedLibExt));
    BOOST_CHECK_THROW(open("hasCreateButNoDestroy"), error::plugin_missing_symbol);
}

BOOST_AUTO_TEST_CASE(checkOpenDoesntThrowExceptionForValidPlugin) {
    BOOST_REQUIRE(boost::filesystem::exists("hasCreateAndDestroy" + sharedLibExt));
    BOOST_CHECK_NO_THROW(open("hasCreateAndDestroy"));
}

BOOST_AUTO_TEST_CASE(checkOpenFileDoesntThrowExceptionForValidPlugin) {
    BOOST_REQUIRE(boost::filesystem::exists("hasCreateAndDestroy" + sharedLibExt));
    BOOST_CHECK_NO_THROW(openFile("hasCreateAndDestroy" + sharedLibExt));
}

BOOST_AUTO_TEST_SUITE_END()

class EmptyPluginPtrWithPluginInSubdirectory_F : public EmptyPluginPtr_F {
public:
    EmptyPluginPtrWithPluginInSubdirectory_F() { 
        boost::filesystem::remove_all("dir1");
        boost::filesystem::create_directory("dir1");
        boost::filesystem::create_directory("dir1/dir2");
        boost::filesystem::rename("hasCreateAndDestroy" + sharedLibExt, "dir1/dir2/hasCreateAndDestroy" + sharedLibExt);
    }
    ~EmptyPluginPtrWithPluginInSubdirectory_F() {
        boost::filesystem::rename("dir1/dir2/hasCreateAndDestroy" + sharedLibExt, "hasCreateAndDestroy" + sharedLibExt);
        boost::filesystem::remove_all("dir1");
    }
};

BOOST_FIXTURE_TEST_SUITE(EmptyPluginPtrWithPluginInSubdirectory_S, EmptyPluginPtrWithPluginInSubdirectory_F)

BOOST_AUTO_TEST_CASE(checkOpenFileWithPathWithForwardSlashes) {
    BOOST_CHECK_NO_THROW(openFile("dir1/dir2/hasCreateAndDestroy" + sharedLibExt));
}

#ifdef PION_WIN32
BOOST_AUTO_TEST_CASE(checkOpenFileWithPathWithBackslashes) {
    BOOST_CHECK_NO_THROW(openFile("dir1\\dir2\\hasCreateAndDestroy" + sharedLibExt));
}
#endif

#ifdef PION_WIN32
BOOST_AUTO_TEST_CASE(checkOpenFileWithPathWithMixedSlashes) {
    BOOST_CHECK_NO_THROW(openFile("dir1\\dir2/hasCreateAndDestroy" + sharedLibExt));
}
#endif

BOOST_AUTO_TEST_SUITE_END()

/*
This is an example of a slightly different style of tests, where the object being
tested is included in the fixture, rather than inherited by the fixture.
These tests are a subset of those in the previous suite, for comparison purposes.
*/
struct EmptyPluginPtr2_F {
    EmptyPluginPtr2_F() : m_pluginPtr() { 
        BOOST_REQUIRE(GET_DIRECTORY(m_old_cwd, DIRECTORY_MAX_SIZE) != NULL);
        BOOST_REQUIRE(CHANGE_DIRECTORY(directoryOfPluginsForTests.c_str()) == 0);
    }
    ~EmptyPluginPtr2_F() {
        BOOST_CHECK(CHANGE_DIRECTORY(m_old_cwd) == 0);
    }

    char m_old_cwd[DIRECTORY_MAX_SIZE+1];
    plugin_ptr<InterfaceStub> m_pluginPtr;
};

BOOST_FIXTURE_TEST_SUITE(EmptyPluginPtr2_S, EmptyPluginPtr2_F)

BOOST_AUTO_TEST_CASE(checkIsOpenReturnsFalse) {
    BOOST_CHECK(!m_pluginPtr.is_open());
}

BOOST_AUTO_TEST_CASE(checkCreateThrowsException) {
    BOOST_CHECK_THROW(m_pluginPtr.create(), error::plugin_undefined);
}

BOOST_AUTO_TEST_CASE(checkDestroyThrowsException) {
    InterfaceStub* s = NULL;
    BOOST_CHECK_THROW(m_pluginPtr.destroy(s), error::plugin_undefined);
}

BOOST_AUTO_TEST_CASE(checkOpenThrowsExceptionForNonExistentPlugin) {
    BOOST_CHECK_THROW(m_pluginPtr.open("NoSuchPlugin"), error::plugin_not_found);
}

BOOST_AUTO_TEST_CASE(checkGetPluginNameReturnsEmptyString) {
    BOOST_CHECK_EQUAL(m_pluginPtr.getPluginName(), "");
}

BOOST_AUTO_TEST_CASE(checkOpenDoesntThrowExceptionForValidPlugin) {
    BOOST_REQUIRE(boost::filesystem::exists("hasCreateAndDestroy" + sharedLibExt));
    BOOST_CHECK_NO_THROW(m_pluginPtr.open("hasCreateAndDestroy"));
}

BOOST_AUTO_TEST_SUITE_END()


struct PluginPtrWithPluginLoaded_F : EmptyPluginPtr_F {
    PluginPtrWithPluginLoaded_F() { 
        s = NULL;
        open("hasCreateAndDestroy");
    }
    ~PluginPtrWithPluginLoaded_F() {
        if (s) destroy(s);
    }

    InterfaceStub* s;
};

BOOST_FIXTURE_TEST_SUITE(PluginPtrWithPluginLoaded_S, PluginPtrWithPluginLoaded_F)

BOOST_AUTO_TEST_CASE(checkIsOpenReturnsTrue) {
    BOOST_CHECK(is_open());
}

BOOST_AUTO_TEST_CASE(checkGetPluginNameReturnsPluginName) {
    BOOST_CHECK_EQUAL(getPluginName(), "hasCreateAndDestroy");
}

BOOST_AUTO_TEST_CASE(checkCreateReturnsSomething) {
    BOOST_CHECK((s = create()) != NULL);
}

BOOST_AUTO_TEST_CASE(checkDestroyDoesntThrowExceptionAfterCreate) {
    s = create();
    BOOST_CHECK_NO_THROW(destroy(s));
    s = NULL;
}

BOOST_AUTO_TEST_SUITE_END()


#ifdef PION_WIN32
    static const std::string fakePluginInSandboxWithExt = "sandbox\\fakePlugin.dll";
#else
    static const std::string fakePluginInSandboxWithExt = "sandbox/fakePlugin.so";
#endif
    static const std::string fakeConfigFileInSandboxWithExt = "sandbox/fakeConfigFile.conf";

class Sandbox_F {
public:
    Sandbox_F() {
# if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
        m_cwd = boost::filesystem::current_path().string();
#else
        m_cwd = boost::filesystem::current_path().directory_string();
#endif 
        
        boost::filesystem::remove_all("sandbox");
        BOOST_REQUIRE(boost::filesystem::create_directory("sandbox"));
        BOOST_REQUIRE(boost::filesystem::create_directory("sandbox/dir1"));
        BOOST_REQUIRE(boost::filesystem::create_directory("sandbox/dir1/dir1A"));
        BOOST_REQUIRE(boost::filesystem::create_directory("sandbox/dir2"));
        boost::filesystem::ofstream emptyFile(fakePluginInSandboxWithExt.c_str());
        emptyFile.close();
        boost::filesystem::ofstream emptyFile2(fakeConfigFileInSandboxWithExt.c_str());
        emptyFile2.close();
        m_path_to_file = "arbitraryString";
    }
    ~Sandbox_F() {
        BOOST_REQUIRE(CHANGE_DIRECTORY(m_cwd.c_str()) == 0);
        boost::filesystem::remove_all("sandbox");
    }
    std::string m_path_to_file;

private:
    std::string m_cwd;
};

BOOST_FIXTURE_TEST_SUITE(Sandbox_S, Sandbox_F)

BOOST_AUTO_TEST_CASE(checkFindPluginFileReturnsFalseForNonexistentPlugin) {
    BOOST_CHECK(!plugin::findPluginFile(m_path_to_file, "nonexistentPlugin"));
}

BOOST_AUTO_TEST_CASE(checkFindPluginFileReturnsFalseForExistingDirectory) {
    BOOST_CHECK(!plugin::findPluginFile(m_path_to_file, "sandbox"));
}

BOOST_AUTO_TEST_CASE(checkFindPluginFileLeavesPathUnchangedForNonexistentPlugin) {
    BOOST_CHECK(!plugin::findPluginFile(m_path_to_file, "nonexistentPlugin"));
    BOOST_CHECK_EQUAL(m_path_to_file, "arbitraryString");
}

BOOST_AUTO_TEST_CASE(checkFindPluginFileReturnsTrueForExistingPlugin) {
    BOOST_CHECK(plugin::findPluginFile(m_path_to_file, "sandbox/fakePlugin"));
}

BOOST_AUTO_TEST_CASE(checkFindPluginFileReturnsCorrectPathForExistingPlugin) {
    BOOST_CHECK(plugin::findPluginFile(m_path_to_file, "sandbox/fakePlugin"));
    BOOST_CHECK_EQUAL(m_path_to_file, fakePluginInSandboxWithExt);
}

BOOST_AUTO_TEST_CASE(checkFindPluginFileReturnsFalseForPluginNotOnSearchPath) {
    BOOST_CHECK(!plugin::findPluginFile(m_path_to_file, "fakePlugin"));
}

BOOST_AUTO_TEST_CASE(checkFindConfigFileReturnsFalseForNonexistentConfigFile) {
    BOOST_CHECK(!plugin::findConfigFile(m_path_to_file, "nonexistentConfigFile"));
}

BOOST_AUTO_TEST_CASE(checkFindConfigFileReturnsFalseForExistingDirectory) {
    BOOST_CHECK(!plugin::findConfigFile(m_path_to_file, "sandbox"));
}

BOOST_AUTO_TEST_CASE(checkFindConfigFileReturnsTrueForExistingConfigFile) {
    BOOST_CHECK(plugin::findConfigFile(m_path_to_file, "sandbox/fakeConfigFile"));
}

BOOST_AUTO_TEST_CASE(checkFindConfigFileReturnsCorrectPathForExistingConfigFile) {
    BOOST_CHECK(plugin::findConfigFile(m_path_to_file, "sandbox/fakeConfigFile"));
    BOOST_CHECK(boost::filesystem::equivalent(m_path_to_file, fakeConfigFileInSandboxWithExt));
}

BOOST_AUTO_TEST_CASE(checkAddPluginDirectoryThrowsExceptionForNonexistentDirectory) {
    BOOST_CHECK_THROW(plugin::addPluginDirectory("nonexistentDir"), error::directory_not_found);
}

BOOST_AUTO_TEST_CASE(checkAddPluginDirectoryWithCurrentDirectory) {
    BOOST_CHECK_NO_THROW(plugin::addPluginDirectory("."));
}

BOOST_AUTO_TEST_CASE(checkAddPluginDirectoryWithExistingDirectory) {
    BOOST_CHECK_NO_THROW(plugin::addPluginDirectory("sandbox"));
}

BOOST_AUTO_TEST_CASE(checkAddPluginDirectoryOneLevelUp) {
    BOOST_REQUIRE(CHANGE_DIRECTORY("sandbox/dir1") == 0);
    BOOST_CHECK_NO_THROW(plugin::addPluginDirectory(".."));
}

// this test only works in Windows
#ifdef PION_WIN32
BOOST_AUTO_TEST_CASE(checkAddPluginDirectoryWithBackslashes) {
    BOOST_CHECK_NO_THROW(plugin::addPluginDirectory("sandbox\\dir1\\dir1A"));
}
#endif

BOOST_AUTO_TEST_CASE(checkAddPluginDirectoryWithUpAndDownPath) {
    BOOST_REQUIRE(CHANGE_DIRECTORY("sandbox/dir1/dir1A") == 0);
    BOOST_CHECK_NO_THROW(plugin::addPluginDirectory("../../dir2"));
}

BOOST_AUTO_TEST_CASE(checkAddPluginDirectoryThrowsExceptionForInvalidDirectory) {
    BOOST_CHECK_THROW(plugin::addPluginDirectory("x:y"), error::directory_not_found);
}

BOOST_AUTO_TEST_CASE(checkResetPluginDirectoriesDoesntThrowException) {
    BOOST_CHECK_NO_THROW(plugin::resetPluginDirectories());
}

BOOST_AUTO_TEST_SUITE_END()

class SandboxAddedAsPluginDirectory_F : public Sandbox_F {
public:
    SandboxAddedAsPluginDirectory_F() {
        plugin::addPluginDirectory("sandbox");
    }
    ~SandboxAddedAsPluginDirectory_F() {
        plugin::resetPluginDirectories();
    }
};

BOOST_FIXTURE_TEST_SUITE(SandboxAddedAsPluginDirectory_S, SandboxAddedAsPluginDirectory_F)

BOOST_AUTO_TEST_CASE(checkFindPluginFileReturnsTrueForPluginOnSearchPath) {
    BOOST_CHECK(plugin::findPluginFile(m_path_to_file, "fakePlugin"));
}

BOOST_AUTO_TEST_CASE(checkFindPluginFileReturnsTrueAfterChangingDirectory) {
    BOOST_REQUIRE(CHANGE_DIRECTORY("sandbox/dir1") == 0);
    BOOST_CHECK(plugin::findPluginFile(m_path_to_file, "fakePlugin"));
}

BOOST_AUTO_TEST_CASE(checkResetPluginDirectoriesDoesntThrowException) {
    BOOST_CHECK_NO_THROW(plugin::resetPluginDirectories());
}

BOOST_AUTO_TEST_CASE(checkFindPluginFileReturnsFalseForPluginOnSearchPathAfterReset) {
    plugin::resetPluginDirectories();
    BOOST_CHECK(!plugin::findPluginFile(m_path_to_file, "fakePlugin"));
}

BOOST_AUTO_TEST_SUITE_END()

#endif // PION_STATIC_LINKING
