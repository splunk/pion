// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/filesystem/operations.hpp>
#include <pion/net/PionConfig.hpp>
#include <pion/net/PionPlugin.hpp>

#ifdef PION_WIN32
	#include <windows.h>
#else
	#include <dlfcn.h>
#endif


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)
	
// static members of PionEngine
	
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

	
// PionEngine member functions
	
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
#ifdef PION_WIN32
	// work around bug in boost::filesystem on Windows -> do not plugin directories
	// basically, if you create a path object on Windows, then convert it to
	// directory_string() or file_string(), then try to construct
	// a new path object based on the string, it throws an exception (ugh!!!)
	boost::mutex::scoped_lock plugin_lock(m_plugin_mutex);
	m_plugin_dirs.push_back(dir);
#else
	boost::filesystem::path plugin_path(dir);
	checkCygwinPath(plugin_path, dir);
	if (! boost::filesystem::exists(plugin_path) )
		throw DirectoryNotFoundException(dir);
	boost::mutex::scoped_lock plugin_lock(m_plugin_mutex);
	m_plugin_dirs.push_back(plugin_path.directory_string());
#endif
}

void PionPlugin::resetPluginDirectories(void)
{
	boost::mutex::scoped_lock plugin_lock(m_plugin_mutex);
	m_plugin_dirs.clear();
}

void PionPlugin::open(const std::string& plugin_file)
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

// The handling of dynamic libraries on Windows is EXTREMELY buggy, so if
// we are running on Windows, never release the shared libraries
#ifndef PION_WIN32
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
#endif
			
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

	// check for existence of plug-in (without extension)		
	if (boost::filesystem::exists(test_path)) {
		final_path = test_path.file_string();
		return true;
	}
		
	// next, try appending the plug-in extension		
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

	// re-check for existence of plug-in (after adding extension)		
	if (boost::filesystem::exists(test_path)) {
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
	// strip path
#ifdef PION_WIN32
	std::string::size_type pos = plugin_file.find_last_of('\\');
#else
	std::string::size_type pos = plugin_file.find_last_of('/');
#endif
	std::string plugin_name = (pos == std::string::npos ?
							   plugin_file : plugin_file.substr(pos+1));
	pos = plugin_name.find('.');

	// truncate extension
	if (pos != std::string::npos)
		plugin_name.resize(pos);
							
	return plugin_name;						
}

void *PionPlugin::loadDynamicLibrary(const std::string& plugin_file)
{
#ifdef PION_WIN32
	#ifdef BOOST_MSVC
		return LoadLibraryA(plugin_file.c_str());
	#else
		return LoadLibrary(plugin_file.c_str());
	#endif
#else
	return dlopen(plugin_file.c_str(), RTLD_LAZY);
#endif
}

void PionPlugin::closeDynamicLibrary(void *lib_handle)
{
#ifdef PION_WIN32
	FreeLibrary((HINSTANCE) lib_handle);
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

}	// end namespace net
}	// end namespace pion
