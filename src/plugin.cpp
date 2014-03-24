// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/thread/mutex.hpp>
#include <pion/config.hpp>
#include <pion/error.hpp>
#include <pion/plugin.hpp>

#ifdef PION_WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif


namespace pion {    // begin namespace pion
    
// static members of plugin
    
const std::string           plugin::PION_PLUGIN_CREATE("pion_create_");
const std::string           plugin::PION_PLUGIN_DESTROY("pion_destroy_");
#ifdef PION_WIN32
    const std::string           plugin::PION_PLUGIN_EXTENSION(".dll");
#else
    const std::string           plugin::PION_PLUGIN_EXTENSION(".so");
#endif
const std::string           plugin::PION_CONFIG_EXTENSION(".conf");
boost::once_flag            plugin::m_instance_flag = BOOST_ONCE_INIT;
plugin::config_type    *plugin::m_config_ptr = NULL;

    
// plugin member functions
    
void plugin::create_plugin_config(void)
{
    static config_type UNIQUE_PION_PLUGIN_CONFIG;
    m_config_ptr = &UNIQUE_PION_PLUGIN_CONFIG;
}

void plugin::check_cygwin_path(boost::filesystem::path& final_path,
                                 const std::string& start_path)
{
#if defined(PION_WIN32) && defined(PION_CYGWIN_DIRECTORY)
    // try prepending PION_CYGWIN_DIRECTORY if not complete
    if (! final_path.is_complete() && final_path.has_root_directory()) {
        final_path = boost::filesystem::path(std::string(PION_CYGWIN_DIRECTORY) + start_path);
    }
#endif
}

void plugin::add_plugin_directory(const std::string& dir)
{
    boost::filesystem::path plugin_path = boost::filesystem::system_complete(dir);
    check_cygwin_path(plugin_path, dir);
    if (! boost::filesystem::exists(plugin_path) )
        BOOST_THROW_EXCEPTION( error::directory_not_found() << error::errinfo_dir_name(dir) );
    config_type& cfg = get_plugin_config();
    boost::mutex::scoped_lock plugin_lock(cfg.m_plugin_mutex);
# if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
    cfg.m_plugin_dirs.push_back(plugin_path.string());
#else
    cfg.m_plugin_dirs.push_back(plugin_path.directory_string());
#endif 
    
}

void plugin::reset_plugin_directories(void)
{
    config_type& cfg = get_plugin_config();
    boost::mutex::scoped_lock plugin_lock(cfg.m_plugin_mutex);
    cfg.m_plugin_dirs.clear();
}

void plugin::open(const std::string& plugin_name)
{
    // check first if name matches an existing plugin name
    {
        config_type& cfg = get_plugin_config();
        boost::mutex::scoped_lock plugin_lock(cfg.m_plugin_mutex);
        map_type::iterator itr = cfg.m_plugin_map.find(plugin_name);
        if (itr != cfg.m_plugin_map.end()) {
            release_data();  // make sure we're not already pointing to something
            m_plugin_data = itr->second;
            ++ m_plugin_data->m_references;
            return;
        }
    }
    
    // nope, look for shared library file
    std::string plugin_file;

    if (!find_plugin_file(plugin_file, plugin_name))
        BOOST_THROW_EXCEPTION( error::plugin_not_found() << error::errinfo_plugin_name(plugin_name) );
        
    open_file(plugin_file);
}

void plugin::open_file(const std::string& plugin_file)
{
    release_data();  // make sure we're not already pointing to something
    
    // use a temporary object first since open_plugin() may throw
    data_type plugin_data(get_plugin_name(plugin_file));
    
    // check to see if we already have a matching shared library
    config_type& cfg = get_plugin_config();
    boost::mutex::scoped_lock plugin_lock(cfg.m_plugin_mutex);
    map_type::iterator itr = cfg.m_plugin_map.find(plugin_data.m_plugin_name);
    if (itr == cfg.m_plugin_map.end()) {
        // no plug-ins found with the same name
        
        // open up the shared library using our temporary data object
        open_plugin(plugin_file, plugin_data);   // may throw
        
        // all is good -> insert it into the plug-in map
        m_plugin_data = new data_type(plugin_data);
        cfg.m_plugin_map.insert( std::make_pair(m_plugin_data->m_plugin_name,
                                            m_plugin_data) );
    } else {
        // found an existing plug-in with the same name
        m_plugin_data = itr->second;
    }
    
    // increment the number of references
    ++ m_plugin_data->m_references;
}

void plugin::release_data(void)
{
    if (m_plugin_data != NULL) {
        config_type& cfg = get_plugin_config();
        boost::mutex::scoped_lock plugin_lock(cfg.m_plugin_mutex);
        // double-check after locking mutex
        if (m_plugin_data != NULL && --m_plugin_data->m_references == 0) {
            // no more references to the plug-in library
            
            // make sure it's not a static library
            if (m_plugin_data->m_lib_handle != NULL) {
            
                // release the shared object
                close_dynamic_library(m_plugin_data->m_lib_handle);
            
                // remove it from the plug-in map
                map_type::iterator itr = cfg.m_plugin_map.find(m_plugin_data->m_plugin_name);
                // check itr just to be safe (it SHOULD always find a match)
                if (itr != cfg.m_plugin_map.end())
                    cfg.m_plugin_map.erase(itr);
            
                // release the heap object
                delete m_plugin_data;
            }
        }
        m_plugin_data = NULL;
    }
}

void plugin::grab_data(const plugin& p)
{
    release_data();  // make sure we're not already pointing to something
    config_type& cfg = get_plugin_config();
    boost::mutex::scoped_lock plugin_lock(cfg.m_plugin_mutex);
    m_plugin_data = const_cast<data_type*>(p.m_plugin_data);
    if (m_plugin_data != NULL) {
        ++ m_plugin_data->m_references;
    }
}

bool plugin::find_file(std::string& path_to_file, const std::string& name,
                          const std::string& extension)
{
    // first, try the name as-is
    if (check_for_file(path_to_file, name, "", extension))
        return true;

    // nope, check search paths
    config_type& cfg = get_plugin_config();
    boost::mutex::scoped_lock plugin_lock(cfg.m_plugin_mutex);
    for (std::vector<std::string>::iterator i = cfg.m_plugin_dirs.begin();
         i != cfg.m_plugin_dirs.end(); ++i)
    {
        if (check_for_file(path_to_file, *i, name, extension))
            return true;
    }
    
    // no plug-in file found
    return false;
}

bool plugin::check_for_file(std::string& final_path, const std::string& start_path,
                              const std::string& name, const std::string& extension)
{
    // check for cygwin path oddities
    boost::filesystem::path cygwin_safe_path(start_path);
    check_cygwin_path(cygwin_safe_path, start_path);
    boost::filesystem::path test_path(cygwin_safe_path);

    // if a name is specified, append it to the test path
    if (! name.empty())
        test_path /= name;

    // check for existence of file (without extension)
    try {
        // is_regular may throw if directory is not readable
        if (boost::filesystem::is_regular(test_path)) {
# if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
            final_path = test_path.string();
#else
            final_path = test_path.file_string();
#endif 
            return true;
        }
    } catch (...) {}

    // next, try appending the extension
    if (name.empty()) {
        // no "name" specified -> append it directly to start_path
        test_path = boost::filesystem::path(start_path + extension);
        // in this case, we need to re-check for the cygwin oddities
        check_cygwin_path(test_path, start_path + extension);
    } else {
        // name is specified, so we can just re-use cygwin_safe_path
        test_path = cygwin_safe_path /
            boost::filesystem::path(name + extension);
    }

    // re-check for existence of file (after adding extension)
    try {
        // is_regular may throw if directory is not readable
        if (boost::filesystem::is_regular(test_path)) {
# if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
            final_path = test_path.string();
#else
            final_path = test_path.file_string();
#endif 
            return true;
        }
    } catch (...) {}

    // no plug-in file found
    return false;
}

void plugin::open_plugin(const std::string& plugin_file,
                            data_type& plugin_data)
{
    // get the name of the plugin (for create/destroy symbol names)
    plugin_data.m_plugin_name = get_plugin_name(plugin_file);
    
    // attempt to open the plugin; note that this tries all search paths
    // and also tries a variety of platform-specific extensions
    plugin_data.m_lib_handle = load_dynamic_library(plugin_file.c_str());
    if (plugin_data.m_lib_handle == NULL) {
#ifndef PION_WIN32
        const char *error_msg = dlerror();
        if (error_msg != NULL) {
            std::string error_str(plugin_file);
            error_str += " (";
            error_str += error_msg;
            error_str += ')';
            BOOST_THROW_EXCEPTION( error::open_plugin()
                                  << error::errinfo_plugin_name(plugin_data.m_plugin_name)
                                  << error::errinfo_message(error_str) );
        } else
#endif
            BOOST_THROW_EXCEPTION( error::open_plugin()
                                  << error::errinfo_plugin_name(plugin_data.m_plugin_name) );
    }
    
    // find the function used to create new plugin objects
    plugin_data.m_create_func =
        get_library_symbol(plugin_data.m_lib_handle,
                         PION_PLUGIN_CREATE + plugin_data.m_plugin_name);
    if (plugin_data.m_create_func == NULL) {
        close_dynamic_library(plugin_data.m_lib_handle);
        BOOST_THROW_EXCEPTION( error::plugin_missing_symbol()
                              << error::errinfo_plugin_name(plugin_data.m_plugin_name)
                              << error::errinfo_symbol_name(PION_PLUGIN_CREATE + plugin_data.m_plugin_name) );
    }

    // find the function used to destroy existing plugin objects
    plugin_data.m_destroy_func =
        get_library_symbol(plugin_data.m_lib_handle,
                         PION_PLUGIN_DESTROY + plugin_data.m_plugin_name);
    if (plugin_data.m_destroy_func == NULL) {
        close_dynamic_library(plugin_data.m_lib_handle);
        BOOST_THROW_EXCEPTION( error::plugin_missing_symbol()
                              << error::errinfo_plugin_name(plugin_data.m_plugin_name)
                              << error::errinfo_symbol_name(PION_PLUGIN_DESTROY + plugin_data.m_plugin_name) );
    }
}

std::string plugin::get_plugin_name(const std::string& plugin_file)
{
    return boost::filesystem::basename(boost::filesystem::path(plugin_file));
}

void plugin::get_all_plugin_names(std::vector<std::string>& plugin_names)
{
    // Iterate through all the Plugin directories.
    std::vector<std::string>::iterator it;
    config_type& cfg = get_plugin_config();
    boost::mutex::scoped_lock plugin_lock(cfg.m_plugin_mutex);
    for (it = cfg.m_plugin_dirs.begin(); it != cfg.m_plugin_dirs.end(); ++it) {
        // Find all shared libraries in the directory and add them to the list of Plugin names.
        boost::filesystem::directory_iterator end;
        for (boost::filesystem::directory_iterator it2(*it); it2 != end; ++it2) {
            if (boost::filesystem::is_regular(*it2)) {
                if (boost::filesystem::extension(it2->path()) == plugin::PION_PLUGIN_EXTENSION) {
# if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
                    plugin_names.push_back(plugin::get_plugin_name(it2->path().filename().string()));
#else
                    plugin_names.push_back(plugin::get_plugin_name(it2->path().leaf()));
#endif 
                }
            }
        }
    }
    
    // Append static-linked libraries
    for (map_type::const_iterator itr = cfg.m_plugin_map.begin(); itr != cfg.m_plugin_map.end(); ++itr) {
        const data_type& plugin_data = *(itr->second);
        if (plugin_data.m_lib_handle == NULL) {
            plugin_names.push_back(plugin_data.m_plugin_name);
        }
    }
}

void *plugin::load_dynamic_library(const std::string& plugin_file)
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
# if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
    const boost::filesystem::path full_path = boost::filesystem::absolute(plugin_file);
#else
    const boost::filesystem::path full_path = boost::filesystem::complete(plugin_file);
#endif 
    // NOTE: you must load shared libraries using RTLD_GLOBAL on Unix platforms
    // due to a bug in GCC (or Boost::any, depending on which crowd you want to believe).
    // see: http://svn.boost.org/trac/boost/ticket/754
# if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
    return dlopen(full_path.string().c_str(), RTLD_LAZY | RTLD_GLOBAL);
#else
    return dlopen(full_path.file_string().c_str(), RTLD_LAZY | RTLD_GLOBAL);
#endif 
#endif
}

void plugin::close_dynamic_library(void *lib_handle)
{
#ifdef PION_WIN32
    // Apparently, FreeLibrary sometimes causes crashes when running 
    // unit tests under Windows.
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

void *plugin::get_library_symbol(void *lib_handle, const std::string& symbol)
{
#ifdef PION_WIN32
    return (void*)GetProcAddress((HINSTANCE) lib_handle, symbol.c_str());
#else
    return dlsym(lib_handle, symbol.c_str());
#endif
}

void plugin::add_static_entry_point(const std::string& plugin_name,
                                     void *create_func,
                                     void *destroy_func)
{
    // check for duplicate
    config_type& cfg = get_plugin_config();
    boost::mutex::scoped_lock plugin_lock(cfg.m_plugin_mutex);
    map_type::iterator itr = cfg.m_plugin_map.find(plugin_name);
    if (itr == cfg.m_plugin_map.end()) {
        // no plug-ins found with the same name
        // all is good -> insert it into the plug-in map
        data_type *plugin_data = new data_type(plugin_name);
        plugin_data->m_lib_handle = NULL; // this will indicate that we are using statically linked plug-in
        plugin_data->m_create_func = create_func;
        plugin_data->m_destroy_func = destroy_func;
        cfg.m_plugin_map.insert(std::make_pair(plugin_name, plugin_data));
    }
}

}   // end namespace pion
