// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/thread/mutex.hpp>
#include <pion/PionConfig.hpp>
#include <pion/PionPlugin.hpp>

#ifdef PION_WIN32
	#include <windows.h>
#else
	#include <dlfcn.h>
#endif


namespace pion {	// begin namespace pion
	
// static members of PionPlugin
	
const std::string			PionPlugin::PION_PLUGIN_CREATE("pion_create_");
const std::string			PionPlugin::PION_PLUGIN_DESTROY("pion_destroy_");
#ifdef PION_WIN32
	const std::string			PionPlugin::PION_PLUGIN_EXTENSION(".dll");
#else
	const std::string			PionPlugin::PION_PLUGIN_EXTENSION(".so");
#endif
const std::string			PionPlugin::PION_CONFIG_EXTENSION(".conf");
boost::once_flag			PionPlugin::m_instance_flag = BOOST_ONCE_INIT;
PionPlugin::PionPluginConfig	*PionPlugin::m_config_ptr = NULL;

	
// PionPlugin member functions
	
void PionPlugin::createPionPluginConfig(void)
{
	static PionPluginConfig UNIQUE_PION_PLUGIN_CONFIG;
	m_config_ptr = &UNIQUE_PION_PLUGIN_CONFIG;
}

void PionPlugin::checkCygwinPath(boost::filesystem::path& final_path,
								 const std::string& start_path)
{
#if defined(PION_WIN32) && defined(PION_CYGWIN_DIRECTORY)
	// try prepending PION_CYGWIN_DIRECTORY if not complete
	if (! final_path.is_complete() && final_path.has_root_directory()) {
		final_path = boost::filesystem::path(std::string(PION_CYGWIN_DIRECTORY) + start_path);
	}
#endif
}

void PionPlugin::addPluginDirectory(const std::string& dir)
{
	boost::filesystem::path plugin_path = boost::filesystem::system_complete(dir);
	checkCygwinPath(plugin_path, dir);
	if (! boost::filesystem::exists(plugin_path) )
		throw DirectoryNotFoundException(dir);
	PionPluginConfig& cfg = getPionPluginConfig();
	boost::mutex::scoped_lock plugin_lock(cfg.plugin_mutex);
	cfg.plugin_dirs.push_back(plugin_path.directory_string());
}

void PionPlugin::resetPluginDirectories(void)
{
	PionPluginConfig& cfg = getPionPluginConfig();
	boost::mutex::scoped_lock plugin_lock(cfg.plugin_mutex);
	cfg.plugin_dirs.clear();
}

void PionPlugin::open(const std::string& plugin_name)
{
	// check first if name matches an existing plugin name
	{
		PionPluginConfig& cfg = getPionPluginConfig();
		boost::mutex::scoped_lock plugin_lock(cfg.plugin_mutex);
		PluginMap::iterator itr = cfg.plugin_map.find(plugin_name);
		if (itr != cfg.plugin_map.end()) {
			releaseData();	// make sure we're not already pointing to something
			m_plugin_data = itr->second;
			++ m_plugin_data->m_references;
			return;
		}
	}
	
	// nope, look for shared library file
	std::string plugin_file;

	if (!findPluginFile(plugin_file, plugin_name))
		throw PluginNotFoundException(plugin_name);
		
	openFile(plugin_file);
}

void PionPlugin::openFile(const std::string& plugin_file)
{
	releaseData();	// make sure we're not already pointing to something
	
	// use a temporary object first since openPlugin() may throw
	PionPluginData plugin_data(getPluginName(plugin_file));
	
	// check to see if we already have a matching shared library
	PionPluginConfig& cfg = getPionPluginConfig();
	boost::mutex::scoped_lock plugin_lock(cfg.plugin_mutex);
	PluginMap::iterator itr = cfg.plugin_map.find(plugin_data.m_plugin_name);
	if (itr == cfg.plugin_map.end()) {
		// no plug-ins found with the same name
		
		// open up the shared library using our temporary data object
		openPlugin(plugin_file, plugin_data);	// may throw
		
		// all is good -> insert it into the plug-in map
		m_plugin_data = new PionPluginData(plugin_data);
		cfg.plugin_map.insert( std::make_pair(m_plugin_data->m_plugin_name,
											m_plugin_data) );
	} else {
		// found an existing plug-in with the same name
		m_plugin_data = itr->second;
	}
	
	// increment the number of references
	++ m_plugin_data->m_references;
}

void PionPlugin::releaseData(void)
{
	if (m_plugin_data != NULL) {
		PionPluginConfig& cfg = getPionPluginConfig();
		boost::mutex::scoped_lock plugin_lock(cfg.plugin_mutex);
		// double-check after locking mutex
		if (m_plugin_data != NULL && --m_plugin_data->m_references == 0) {
			// no more references to the plug-in library
			
			// make sure it's not a static library
			if (m_plugin_data->m_lib_handle != NULL) {
			
				// release the shared object
				closeDynamicLibrary(m_plugin_data->m_lib_handle);
			
				// remove it from the plug-in map
				PluginMap::iterator itr = cfg.plugin_map.find(m_plugin_data->m_plugin_name);
				// check itr just to be safe (it SHOULD always find a match)
				if (itr != cfg.plugin_map.end())
					cfg.plugin_map.erase(itr);
			
				// release the heap object
				delete m_plugin_data;
			}
		}
		m_plugin_data = NULL;
	}
}

void PionPlugin::grabData(const PionPlugin& p)
{
	releaseData();	// make sure we're not already pointing to something
	PionPluginConfig& cfg = getPionPluginConfig();
	boost::mutex::scoped_lock plugin_lock(cfg.plugin_mutex);
	m_plugin_data = const_cast<PionPluginData*>(p.m_plugin_data);
	if (m_plugin_data != NULL) {
		++ m_plugin_data->m_references;
	}
}

bool PionPlugin::findFile(std::string& path_to_file, const std::string& name,
						  const std::string& extension)
{
	// first, try the name as-is
	if (checkForFile(path_to_file, name, "", extension))
		return true;

	// nope, check search paths
	PionPluginConfig& cfg = getPionPluginConfig();
	boost::mutex::scoped_lock plugin_lock(cfg.plugin_mutex);
	for (std::vector<std::string>::iterator i = cfg.plugin_dirs.begin();
		 i != cfg.plugin_dirs.end(); ++i)
	{
		if (checkForFile(path_to_file, *i, name, extension))
			return true;
	}
	
	// no plug-in file found
	return false;
}

bool PionPlugin::checkForFile(std::string& final_path, const std::string& start_path,
							  const std::string& name, const std::string& extension)
{
	// check for cygwin path oddities
	boost::filesystem::path cygwin_safe_path(start_path);
	checkCygwinPath(cygwin_safe_path, start_path);
	boost::filesystem::path test_path(cygwin_safe_path);

	// if a name is specified, append it to the test path
	if (! name.empty())
		test_path /= name;

	// check for existence of file (without extension)
	try {
		// is_regular may throw if directory is not readable
		if (boost::filesystem::is_regular(test_path)) {
			final_path = test_path.file_string();
			return true;
		}
	} catch (...) {}

	// next, try appending the extension
	if (name.empty()) {
		// no "name" specified -> append it directly to start_path
		test_path = boost::filesystem::path(start_path + extension);
		// in this case, we need to re-check for the cygwin oddities
		checkCygwinPath(test_path, start_path + extension);
	} else {
		// name is specified, so we can just re-use cygwin_safe_path
		test_path = cygwin_safe_path /
			boost::filesystem::path(name + extension);
	}

	// re-check for existence of file (after adding extension)
	try {
		// is_regular may throw if directory is not readable
		if (boost::filesystem::is_regular(test_path)) {
			final_path = test_path.file_string();
			return true;
		}
	} catch (...) {}

	// no plug-in file found
	return false;
}

void PionPlugin::openPlugin(const std::string& plugin_file,
							PionPluginData& plugin_data)
{
	// get the name of the plugin (for create/destroy symbol names)
	plugin_data.m_plugin_name = getPluginName(plugin_file);
	
	// attempt to open the plugin; note that this tries all search paths
	// and also tries a variety of platform-specific extensions
	plugin_data.m_lib_handle = loadDynamicLibrary(plugin_file.c_str());
	if (plugin_data.m_lib_handle == NULL) {
#ifndef PION_WIN32
		const char *error_msg = dlerror();
		if (error_msg != NULL) {
			std::string error_str(plugin_file);
			error_str += " (";
			error_str += error_msg;
			error_str += ')';
			throw OpenPluginException(error_str);
		} else
#endif
		throw OpenPluginException(plugin_file);
	}
	
	// find the function used to create new plugin objects
	plugin_data.m_create_func =
		getLibrarySymbol(plugin_data.m_lib_handle,
						 PION_PLUGIN_CREATE + plugin_data.m_plugin_name);
	if (plugin_data.m_create_func == NULL) {
		closeDynamicLibrary(plugin_data.m_lib_handle);
		throw PluginMissingCreateException(plugin_file);
	}

	// find the function used to destroy existing plugin objects
	plugin_data.m_destroy_func =
		getLibrarySymbol(plugin_data.m_lib_handle,
						 PION_PLUGIN_DESTROY + plugin_data.m_plugin_name);
	if (plugin_data.m_destroy_func == NULL) {
		closeDynamicLibrary(plugin_data.m_lib_handle);
		throw PluginMissingDestroyException(plugin_file);
	}
}

std::string PionPlugin::getPluginName(const std::string& plugin_file)
{
	return boost::filesystem::basename(boost::filesystem::path(plugin_file));
}

void PionPlugin::getAllPluginNames(std::vector<std::string>& plugin_names)
{
	// Iterate through all the Plugin directories.
	std::vector<std::string>::iterator it;
	PionPluginConfig& cfg = getPionPluginConfig();
	boost::mutex::scoped_lock plugin_lock(cfg.plugin_mutex);
	for (it = cfg.plugin_dirs.begin(); it != cfg.plugin_dirs.end(); ++it) {
		// Find all shared libraries in the directory and add them to the list of Plugin names.
		boost::filesystem::directory_iterator end;
		for (boost::filesystem::directory_iterator it2(*it); it2 != end; ++it2) {
			if (boost::filesystem::is_regular(*it2)) {
				if (boost::filesystem::extension(it2->path()) == PionPlugin::PION_PLUGIN_EXTENSION) {
					plugin_names.push_back(PionPlugin::getPluginName(it2->path().leaf()));
				}
			}
		}
	}
	
	// Append static-linked libraries
	for (PluginMap::const_iterator itr = cfg.plugin_map.begin(); itr != cfg.plugin_map.end(); ++itr) {
		const PionPluginData& plugin_data = *(itr->second);
		if (plugin_data.m_lib_handle == NULL) {
			plugin_names.push_back(plugin_data.m_plugin_name);
		}
	}
}

void *PionPlugin::loadDynamicLibrary(const std::string& plugin_file)
{
#ifdef PION_WIN32
	#ifdef _MSC_VER
		return LoadLibraryA(plugin_file.c_str());
	#else
		return LoadLibrary(plugin_file.c_str());
	#endif
#else
	// convert into a full/absolute/complete path since dlopen()
	// does not always search the CWD on some operating systems
	const boost::filesystem::path full_path = boost::filesystem::complete(plugin_file);
	// NOTE: you must load shared libraries using RTLD_GLOBAL on Unix platforms
	// due to a bug in GCC (or Boost::any, depending on which crowd you want to believe).
	// see: http://svn.boost.org/trac/boost/ticket/754
	return dlopen(full_path.file_string().c_str(), RTLD_LAZY | RTLD_GLOBAL);
#endif
}

void PionPlugin::closeDynamicLibrary(void *lib_handle)
{
#ifdef PION_WIN32
	// Apparently, FreeLibrary sometimes causes crashes when running 
	// pion-net-unit-tests under Windows.
	// It's hard to pin down, because many things can suppress the crashes,
	// such as enabling logging or setting breakpoints (i.e. things that 
	// might help pin it down.)  Also, it's very intermittent, and can be 
	// strongly affected by other processes that are running.
	// So, please don't call FreeLibrary here unless you've been able to 
	// reproduce and fix the crashing of the unit tests.

	//FreeLibrary((HINSTANCE) lib_handle);
#else
	dlclose(lib_handle);
#endif
}

void *PionPlugin::getLibrarySymbol(void *lib_handle, const std::string& symbol)
{
#ifdef PION_WIN32
	return (void*)GetProcAddress((HINSTANCE) lib_handle, symbol.c_str());
#else
	return dlsym(lib_handle, symbol.c_str());
#endif
}

void PionPlugin::addStaticEntryPoint(const std::string& plugin_name,
									 void *create_func,
									 void *destroy_func)
{
	// check for duplicate
	PionPluginConfig& cfg = getPionPluginConfig();
	boost::mutex::scoped_lock plugin_lock(cfg.plugin_mutex);
	PluginMap::iterator itr = cfg.plugin_map.find(plugin_name);
	if (itr == cfg.plugin_map.end()) {
		// no plug-ins found with the same name
		// all is good -> insert it into the plug-in map
		PionPluginData *plugin_data = new PionPluginData(plugin_name);
		plugin_data->m_lib_handle = NULL; // this will indicate that we are using statically linked plug-in
		plugin_data->m_create_func = create_func;
		plugin_data->m_destroy_func = destroy_func;
		cfg.plugin_map.insert(std::make_pair(plugin_name, plugin_data));
	}
}

}	// end namespace pion
