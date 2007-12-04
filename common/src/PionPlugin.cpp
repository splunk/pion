// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/filesystem.hpp>
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
std::vector<std::string>	PionPlugin::m_plugin_dirs;
PionPlugin::PluginMap		PionPlugin::m_plugin_map;
boost::mutex				PionPlugin::m_plugin_mutex;
PionPlugin::StaticEntryPointList	*PionPlugin::m_entry_points_ptr = NULL;

	
// PionPlugin member functions
	
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
	boost::mutex::scoped_lock plugin_lock(m_plugin_mutex);
	m_plugin_dirs.push_back(plugin_path.directory_string());
}

void PionPlugin::resetPluginDirectories(void)
{
	boost::mutex::scoped_lock plugin_lock(m_plugin_mutex);
	m_plugin_dirs.clear();
}

void PionPlugin::open(const std::string& plugin_name)
{
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
	boost::mutex::scoped_lock plugin_lock(m_plugin_mutex);
	PluginMap::iterator itr = m_plugin_map.find(plugin_data.m_plugin_name);
	if (itr == m_plugin_map.end()) {
		// no plug-ins found with the same name
		
		// open up the shared library using our temporary data object
		openPlugin(plugin_file, plugin_data);	// may throw
		
		// all is good -> insert it into the plug-in map
		m_plugin_data = new PionPluginData(plugin_data);
		m_plugin_map.insert( std::make_pair(m_plugin_data->m_plugin_name,
											m_plugin_data) );
	} else {
		// found an existing plug-in with the same name
		m_plugin_data = itr->second;
	}
	
	// increment the number of references
	++ m_plugin_data->m_references;
}

void PionPlugin::openStaticLinked(const std::string& plugin_name,
								  void *create_func,
								  void *destroy_func)
{
	releaseData();	// make sure we're not already pointing to something

	// check to see if we already have a matching shared library
	boost::mutex::scoped_lock plugin_lock(m_plugin_mutex);
	PluginMap::iterator itr = m_plugin_map.find(plugin_name);
	if (itr == m_plugin_map.end()) {
		// no plug-ins found with the same name

		// all is good -> insert it into the plug-in map
		m_plugin_data = new PionPluginData(plugin_name);
		m_plugin_data->m_lib_handle = NULL; // this will indicate that we are using statically linked plug-in
		m_plugin_data->m_create_func = create_func;
		m_plugin_data->m_destroy_func = destroy_func;
		m_plugin_map.insert(std::make_pair(m_plugin_data->m_plugin_name,
										   m_plugin_data));
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
		boost::mutex::scoped_lock plugin_lock(m_plugin_mutex);
		// double-check after locking mutex
		if (m_plugin_data != NULL && --m_plugin_data->m_references == 0) {
			// no more references to the plug-in library
			
			// release the shared object
			closeDynamicLibrary(m_plugin_data->m_lib_handle);
			
			// remove it from the plug-in map
			PluginMap::iterator itr = m_plugin_map.find(m_plugin_data->m_plugin_name);
			// check itr just to be safe (it SHOULD always find a match)
			if (itr != m_plugin_map.end())
				m_plugin_map.erase(itr);
			
			// release the heap object
			delete m_plugin_data;
		}
		m_plugin_data = NULL;
	}
}

void PionPlugin::grabData(const PionPlugin& p)
{
	releaseData();	// make sure we're not already pointing to something
	boost::mutex::scoped_lock plugin_lock(m_plugin_mutex);
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
	boost::mutex::scoped_lock plugin_lock(m_plugin_mutex);
	for (std::vector<std::string>::iterator i = m_plugin_dirs.begin();
		 i != m_plugin_dirs.end(); ++i)
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
	if (boost::filesystem::is_regular(test_path)) {
		final_path = test_path.file_string();
		return true;
	}
		
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
	if (boost::filesystem::is_regular(test_path)) {
		final_path = test_path.file_string();
		return true;
	}
	
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
	if (plugin_data.m_lib_handle == NULL)
		throw PluginNotFoundException(plugin_file);
	
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
	return dlopen(full_path.file_string().c_str(), RTLD_LAZY);
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

bool PionPlugin::findStaticEntryPoint(const std::string& plugin_name,
									  void **create_func,
									  void **destroy_func)
{
	// check simple case first: no entry points exist
	if (m_entry_points_ptr == NULL || m_entry_points_ptr->empty())
		return false;

	// try to find the entry point for the plugin
	for (std::list<StaticEntryPoint>::const_iterator i = m_entry_points_ptr->begin();
		 i != m_entry_points_ptr->end(); ++i) {
			if (i->m_plugin_name==plugin_name) {
				*create_func  = i->m_create_func;
				*destroy_func = i->m_destroy_func;
				return true;
			}
	}
	return false;
}

void PionPlugin::addStaticEntryPoint(const std::string& plugin_name,
									 void *create_func,
									 void *destroy_func)
{
	// make sure that this function can only be called by one thread at a time
	static boost::mutex			entrypoint_mutex;
	boost::mutex::scoped_lock	entrypoint_lock(entrypoint_mutex);

	// create the entry point list if it doesn't already exist
	if (m_entry_points_ptr == NULL)
		m_entry_points_ptr = new StaticEntryPointList;
	
	// insert it into the entry point list
	m_entry_points_ptr->push_back(StaticEntryPoint(plugin_name, create_func, destroy_func));
}

}	// end namespace pion
