// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifdef WIN32
	#include <direct.h>
	#define CHANGE_DIRECTORY _chdir
	#define GET_DIRECTORY _getcwd
#else
	#include <unistd.h>
	#define CHANGE_DIRECTORY chdir
	#define GET_DIRECTORY getcwd
#endif
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>
#include <pion/PionConfig.hpp>
#include <pion/PionPlugin.hpp>
#include <boost/test/unit_test.hpp>

using namespace pion;


#if defined(PION_WIN32)
	#if defined(_MSC_VER)
		static const std::string directoryOfPluginsForTests = "PluginsUsedByUnitTests\\bin";
	#else
		static const std::string directoryOfPluginsForTests = "PluginsUsedByUnitTests/.libs";
	#endif
	static const std::string sharedLibExt = ".dll";
#else
	static const std::string directoryOfPluginsForTests = "PluginsUsedByUnitTests/.libs";
	static const std::string sharedLibExt = ".so";
#endif


class InterfaceStub {
};

class EmptyPluginPtr_F : public PionPluginPtr<InterfaceStub> {
public:
	EmptyPluginPtr_F() { 
		BOOST_REQUIRE((m_old_cwd = GET_DIRECTORY(NULL, 0)) != NULL);
		BOOST_REQUIRE(CHANGE_DIRECTORY(directoryOfPluginsForTests.c_str()) == 0);
	}
	~EmptyPluginPtr_F() {
		BOOST_CHECK(CHANGE_DIRECTORY(m_old_cwd) == 0);
		free(m_old_cwd);
	}

private:
	char* m_old_cwd;
};

BOOST_FIXTURE_TEST_SUITE(EmptyPluginPtr_S, EmptyPluginPtr_F)

BOOST_AUTO_TEST_CASE(checkIsOpenReturnsFalse) {
	BOOST_CHECK(!is_open());
}

BOOST_AUTO_TEST_CASE(checkCreateThrowsException) {
	BOOST_CHECK_THROW(create(), PluginUndefinedException);
}

BOOST_AUTO_TEST_CASE(checkDestroyThrowsException) {
	InterfaceStub* s = NULL;
	BOOST_CHECK_THROW(destroy(s), PluginUndefinedException);
}

BOOST_AUTO_TEST_CASE(checkOpenThrowsExceptionForNonExistentPlugin) {
	BOOST_REQUIRE(!boost::filesystem::exists("NoSuchPlugin" + sharedLibExt));
	BOOST_CHECK_THROW(open("NoSuchPlugin"), PluginNotFoundException);
}

BOOST_AUTO_TEST_CASE(checkGetPluginNameReturnsEmptyString) {
	BOOST_CHECK_EQUAL(getPluginName(), "");
}

#ifndef PION_STATIC_LINKING

BOOST_AUTO_TEST_CASE(checkOpenThrowsExceptionForNonPluginDll) {
	BOOST_REQUIRE(boost::filesystem::exists("hasNoCreate" + sharedLibExt));
	BOOST_CHECK_THROW(open("hasNoCreate"), PluginMissingCreateException);
}

BOOST_AUTO_TEST_CASE(checkOpenThrowsExceptionForPluginWithoutDestroy) {
	BOOST_REQUIRE(boost::filesystem::exists("hasCreateButNoDestroy" + sharedLibExt));
	BOOST_CHECK_THROW(open("hasCreateButNoDestroy"), PluginMissingDestroyException);
}

BOOST_AUTO_TEST_CASE(checkOpenDoesntThrowExceptionForValidPlugin) {
	BOOST_REQUIRE(boost::filesystem::exists("hasCreateAndDestroy" + sharedLibExt));
	BOOST_CHECK_NO_THROW(open("hasCreateAndDestroy"));
}

BOOST_AUTO_TEST_CASE(checkOpenFileDoesntThrowExceptionForValidPlugin) {
	BOOST_REQUIRE(boost::filesystem::exists("hasCreateAndDestroy" + sharedLibExt));
	BOOST_CHECK_NO_THROW(openFile("hasCreateAndDestroy" + sharedLibExt));
}

#endif // PION_STATIC_LINKING

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
		BOOST_REQUIRE((m_old_cwd = GET_DIRECTORY(NULL, 0)) != NULL);
		BOOST_REQUIRE(CHANGE_DIRECTORY(directoryOfPluginsForTests.c_str()) == 0);
	}
	~EmptyPluginPtr2_F() {
		BOOST_CHECK(CHANGE_DIRECTORY(m_old_cwd) == 0);
		free(m_old_cwd);
	}

	char* m_old_cwd;
	PionPluginPtr<InterfaceStub> m_pluginPtr;
};

BOOST_FIXTURE_TEST_SUITE(EmptyPluginPtr2_S, EmptyPluginPtr2_F)

BOOST_AUTO_TEST_CASE(checkIsOpenReturnsFalse) {
	BOOST_CHECK(!m_pluginPtr.is_open());
}

BOOST_AUTO_TEST_CASE(checkCreateThrowsException) {
	BOOST_CHECK_THROW(m_pluginPtr.create(), PionPlugin::PluginUndefinedException);
}

BOOST_AUTO_TEST_CASE(checkDestroyThrowsException) {
	InterfaceStub* s = NULL;
	BOOST_CHECK_THROW(m_pluginPtr.destroy(s), PionPlugin::PluginUndefinedException);
}

BOOST_AUTO_TEST_CASE(checkOpenThrowsExceptionForNonExistentPlugin) {
	BOOST_CHECK_THROW(m_pluginPtr.open("NoSuchPlugin"), PionPlugin::PluginNotFoundException);
}

BOOST_AUTO_TEST_CASE(checkGetPluginNameReturnsEmptyString) {
	BOOST_CHECK_EQUAL(m_pluginPtr.getPluginName(), "");
}

#ifndef PION_STATIC_LINKING
BOOST_AUTO_TEST_CASE(checkOpenDoesntThrowExceptionForValidPlugin) {
	BOOST_REQUIRE(boost::filesystem::exists("hasCreateAndDestroy" + sharedLibExt));
	BOOST_CHECK_NO_THROW(m_pluginPtr.open("hasCreateAndDestroy"));
}
#endif // PION_STATIC_LINKING

BOOST_AUTO_TEST_SUITE_END()

#ifndef PION_STATIC_LINKING

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

#endif // PION_STATIC_LINKING

#ifdef PION_WIN32
	static const std::string fakePluginInSandboxWithExt = "sandbox\\fakePlugin.dll";
#else
	static const std::string fakePluginInSandboxWithExt = "sandbox/fakePlugin.so";
#endif

class Sandbox_F {
public:
	Sandbox_F() {
		m_cwd = boost::filesystem::current_path().directory_string();
		boost::filesystem::remove_all("sandbox");
		BOOST_REQUIRE(boost::filesystem::create_directory("sandbox"));
		BOOST_REQUIRE(boost::filesystem::create_directory("sandbox/dir1"));
		BOOST_REQUIRE(boost::filesystem::create_directory("sandbox/dir1/dir1A"));
		BOOST_REQUIRE(boost::filesystem::create_directory("sandbox/dir2"));
		boost::filesystem::ofstream emptyFile(fakePluginInSandboxWithExt.c_str());
		emptyFile.close();
		m_path_to_file = "arbitraryString";
	}
	~Sandbox_F() {
		CHANGE_DIRECTORY(m_cwd.c_str());
		boost::filesystem::remove_all("sandbox");
	}
	std::string m_path_to_file;

private:
	std::string m_cwd;
};

BOOST_FIXTURE_TEST_SUITE(Sandbox_S, Sandbox_F)

BOOST_AUTO_TEST_CASE(checkFindPluginFileReturnsFalseForNonexistentPlugin) {
	BOOST_CHECK(!PionPlugin::findPluginFile(m_path_to_file, "nonexistentPlugin"));
}

BOOST_AUTO_TEST_CASE(checkFindPluginFileLeavesPathUnchangedForNonexistentPlugin) {
	BOOST_CHECK(!PionPlugin::findPluginFile(m_path_to_file, "nonexistentPlugin"));
	BOOST_CHECK_EQUAL(m_path_to_file, "arbitraryString");
}

BOOST_AUTO_TEST_CASE(checkFindPluginFileReturnsTrueForExistingPlugin) {
	BOOST_CHECK(PionPlugin::findPluginFile(m_path_to_file, "sandbox/fakePlugin"));
}

BOOST_AUTO_TEST_CASE(checkFindPluginFileReturnsCorrectPathForExistingPlugin) {
	BOOST_CHECK(PionPlugin::findPluginFile(m_path_to_file, "sandbox/fakePlugin"));
	BOOST_CHECK_EQUAL(m_path_to_file, fakePluginInSandboxWithExt);
}

BOOST_AUTO_TEST_CASE(checkFindPluginFileReturnsFalseForPluginNotOnSearchPath) {
	BOOST_CHECK(!PionPlugin::findPluginFile(m_path_to_file, "fakePlugin"));
}

BOOST_AUTO_TEST_CASE(checkAddPluginDirectoryThrowsExceptionForNonexistentDirectory) {
	BOOST_CHECK_THROW(PionPlugin::addPluginDirectory("nonexistentDir"), PionPlugin::DirectoryNotFoundException);
}

BOOST_AUTO_TEST_CASE(checkAddPluginDirectoryWithCurrentDirectory) {
	BOOST_CHECK_NO_THROW(PionPlugin::addPluginDirectory("."));
}

BOOST_AUTO_TEST_CASE(checkAddPluginDirectoryWithExistingDirectory) {
	BOOST_CHECK_NO_THROW(PionPlugin::addPluginDirectory("sandbox"));
}

BOOST_AUTO_TEST_CASE(checkAddPluginDirectoryOneLevelUp) {
	BOOST_REQUIRE(CHANGE_DIRECTORY("sandbox/dir1") == 0);
	BOOST_CHECK_NO_THROW(PionPlugin::addPluginDirectory(".."));
}

// this test only works in Windows
#ifdef PION_WIN32
BOOST_AUTO_TEST_CASE(checkAddPluginDirectoryWithBackslashes) {
	BOOST_CHECK_NO_THROW(PionPlugin::addPluginDirectory("sandbox\\dir1\\dir1A"));
}
#endif

BOOST_AUTO_TEST_CASE(checkAddPluginDirectoryWithUpAndDownPath) {
	BOOST_REQUIRE(CHANGE_DIRECTORY("sandbox/dir1/dir1A") == 0);
	BOOST_CHECK_NO_THROW(PionPlugin::addPluginDirectory("../../dir2"));
}

BOOST_AUTO_TEST_CASE(checkAddPluginDirectoryThrowsExceptionForInvalidDirectory) {
	BOOST_CHECK_THROW(PionPlugin::addPluginDirectory("x:y"), PionPlugin::DirectoryNotFoundException);
}

BOOST_AUTO_TEST_CASE(checkResetPluginDirectoriesDoesntThrowException) {
	BOOST_CHECK_NO_THROW(PionPlugin::resetPluginDirectories());
}

BOOST_AUTO_TEST_SUITE_END()

class SandboxAddedAsPluginDirectory_F : public Sandbox_F {
public:
	SandboxAddedAsPluginDirectory_F() {
		PionPlugin::addPluginDirectory("sandbox");
	}
	~SandboxAddedAsPluginDirectory_F() {
		PionPlugin::resetPluginDirectories();
	}
};

BOOST_FIXTURE_TEST_SUITE(SandboxAddedAsPluginDirectory_S, SandboxAddedAsPluginDirectory_F)

BOOST_AUTO_TEST_CASE(checkFindPluginFileReturnsTrueForPluginOnSearchPath) {
	BOOST_CHECK(PionPlugin::findPluginFile(m_path_to_file, "fakePlugin"));
}

BOOST_AUTO_TEST_CASE(checkFindPluginFileReturnsTrueAfterChangingDirectory) {
	BOOST_REQUIRE(CHANGE_DIRECTORY("sandbox/dir1") == 0);
	BOOST_CHECK(PionPlugin::findPluginFile(m_path_to_file, "fakePlugin"));
}

BOOST_AUTO_TEST_CASE(checkResetPluginDirectoriesDoesntThrowException) {
	BOOST_CHECK_NO_THROW(PionPlugin::resetPluginDirectories());
}

BOOST_AUTO_TEST_CASE(checkFindPluginFileReturnsFalseForPluginOnSearchPathAfterReset) {
	PionPlugin::resetPluginDirectories();
	BOOST_CHECK(!PionPlugin::findPluginFile(m_path_to_file, "fakePlugin"));
}

BOOST_AUTO_TEST_SUITE_END()
