// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PLUGIN_HEADER__
#define __PION_PLUGIN_HEADER__

#include <vector>
#include <string>
#include <map>
#include <list>
#include <mutex>
#include <thread>
#include <boost/noncopyable.hpp>
#include <boost/filesystem/path.hpp>
#include <pion/config.hpp>
#include <pion/error.hpp>


namespace pion {    // begin namespace pion

///
/// plugin: base class for plug-in management
///
class PION_API plugin {
public:

    /**
     * searches directories for a valid plug-in file
     *
     * @param path_to_file the path to the plug-in file, if found
     * @param the name name of the plug-in to search for
     * @return true if the plug-in file was found
     */
    static inline bool find_plugin_file(std::string& path_to_file,
                                      const std::string& name)
    {
        return find_file(path_to_file, name, PION_PLUGIN_EXTENSION);
    }

    /**
     * searches directories for a valid plug-in configuration file
     *
     * @param path_to_file if found, is set to the complete path to the file
     * @param name the name of the configuration file to search for
     * @return true if the configuration file was found
     */
    static inline bool find_config_file(std::string& path_to_file,
                                      const std::string& name)
    {
        return find_file(path_to_file, name, PION_CONFIG_EXTENSION);
    }
    
    /**
     * adds an entry point for a plugin that is statically linked
     * NOTE: USE PION_DECLARE_PLUGIN() macro instead!!!
     *
     * @param plugin_name the name of the plugin to add
     * @param create_func - pointer to the function to be used in to create plugin object
     * @param destroy_func - pointer to the function to be used to release plugin object
     */
    static void add_static_entry_point(const std::string& plugin_name,
                                    void *create_func,
                                    void *destroy_func);
    
    /**
     * updates path for cygwin oddities, if necessary
     *
     * @param final_path path object for the file, will be modified if necessary
     * @param start_path original path to the file.  if final_path is not valid,
     *                   this will be appended to PION_CYGWIN_DIRECTORY to attempt
     *                   attempt correction of final_path for cygwin
     */
    static void check_cygwin_path(boost::filesystem::path& final_path,
                                const std::string& path_string);

    /// appends a directory to the plug-in search path
    static void add_plugin_directory(const std::string& dir);
    
    /// clears all directories from the plug-in search path
    static void reset_plugin_directories(void);
    

    // default destructor
    virtual ~plugin() { release_data(); }
    
    /// returns true if a shared library is loaded/open
    inline bool is_open(void) const { return (m_plugin_data != NULL); }
    
    /// returns the name of the plugin that is currently open
    inline std::string get_plugin_name(void) const {
        return (is_open() ? m_plugin_data->m_plugin_name : std::string());
    }

    /// returns a list of all Plugins found in all Plugin directories
    static void get_all_plugin_names(std::vector<std::string>& plugin_names);

    /**
     * opens plug-in library within a shared object file.  If the library is
     * already being used by another plugin object, then the existing
     * code will be re-used and the reference count will be increased.  Beware
     * that this does NOT check the plug-in's base class (InterfaceClassType),
     * so you must be careful to ensure that the namespace is unique between
     * plug-ins that have different base classes.  If the plug-in's name matches
     * an existing plug-in of a different base class, the resulting behavior is
     * undefined (it will probably crash your program).
     * 
     * @param plugin_name name of the plug-in library to open (without extension, etc.)
     */
    void open(const std::string& plugin_name);

    /**
     * opens plug-in library within a shared object file.  If the library is
     * already being used by another plugin object, then the existing
     * code will be re-used and the reference count will be increased.  Beware
     * that this does NOT check the plug-in's base class (InterfaceClassType),
     * so you must be careful to ensure that the namespace is unique between
     * plug-ins that have different base classes.  If the plug-in's name matches
     * an existing plug-in of a different base class, the resulting behavior is
     * undefined (it will probably crash your program).
     * 
     * @param plugin_file shared object file containing the plugin code
     */
    void open_file(const std::string& plugin_file);

    /// closes plug-in library
    inline void close(void) { release_data(); }

protected:
    
    ///
    /// data_type: object to hold shared library symbols
    ///
    struct data_type
    {
        /// default constructors for convenience
        data_type(void)
            : m_lib_handle(NULL), m_create_func(NULL), m_destroy_func(NULL),
            m_references(0)
        {}
        data_type(const std::string& plugin_name)
            : m_lib_handle(NULL), m_create_func(NULL), m_destroy_func(NULL),
            m_plugin_name(plugin_name), m_references(0)
        {}
        data_type(const data_type& p)
            : m_lib_handle(p.m_lib_handle), m_create_func(p.m_create_func),
            m_destroy_func(p.m_destroy_func), m_plugin_name(p.m_plugin_name),
            m_references(p.m_references)
        {}
        
        /// symbol library loaded from a shared object file
        void *          m_lib_handle;
        
        /// function used to create instances of the plug-in object
        void *          m_create_func;
        
        /// function used to destroy instances of the plug-in object
        void *          m_destroy_func;
        
        /// the name of the plugin (must be unique per process)
        std::string     m_plugin_name;
        
        /// number of references to this class
        unsigned long   m_references;
    };

    
    /// default constructor is private (use plugin_ptr class to create objects)
    plugin(void) : m_plugin_data(NULL) {}
    
    /// copy constructor
    plugin(const plugin& p) : m_plugin_data(NULL) { grab_data(p); }

    /// assignment operator
    plugin& operator=(const plugin& p) { grab_data(p); return *this; }

    /// returns a pointer to the plug-in's "create object" function
    inline void *get_create_function(void) {
        return (is_open() ? m_plugin_data->m_create_func : NULL);
    }

    /// returns a pointer to the plug-in's "destroy object" function
    inline void *get_destroy_function(void) {
        return (is_open() ? m_plugin_data->m_destroy_func : NULL);
    }

    /// releases the plug-in's shared library symbols
    void release_data(void);
    
    /// grabs a reference to another plug-in's shared library symbols
    void grab_data(const plugin& p);

    
private:

    /// data type that maps plug-in names to their shared library data
    typedef std::map<std::string, data_type*>  map_type;

    /// data type for static/global plugin configuration information
    struct config_type {
        /// directories containing plugin files
        std::vector<std::string>    m_plugin_dirs;
        
        /// maps plug-in names to shared library data
        map_type                    m_plugin_map;
        
        /// mutex to make class thread-safe
        std::mutex                m_plugin_mutex;
    };

    
    /// returns a singleton instance of config_type
    static inline config_type& get_plugin_config(void) {
        std::call_once(m_instance_flag, plugin::create_plugin_config);
        return *m_config_ptr;
    }
    
    /// creates the plugin_config singleton
    static void create_plugin_config(void);

    /**
     * searches directories for a valid plug-in file
     *
     * @param path_to_file if found, is set to the complete path to the file
     * @param name the name of the file to search for
     * @param extension will be appended to name if name is not found
     *
     * @return true if the file was found
     */
    static bool find_file(std::string& path_to_file, const std::string& name,                             
                         const std::string& extension);
    
    /**
     * normalizes complete and final path to a file while looking for it
     *
     * @param final_path if found, is set to the complete, normalized path to the file
     * @param start_path the original starting path to the file
     * @param name the name of the file to search for
     * @param extension will be appended to name if name is not found
     *
     * @return true if the file was found
     */
    static bool check_for_file(std::string& final_path, const std::string& start_path,
                             const std::string& name, const std::string& extension);
    
    /**
     * opens plug-in library within a shared object file
     * 
     * @param plugin_file shared object file containing the plugin code
     * @param plugin_data data object to load the library into
     */
    static void open_plugin(const std::string& plugin_file,
                           data_type& plugin_data);

    /// returns the name of the plugin object (based on the plugin_file name)
    static std::string get_plugin_name(const std::string& plugin_file);
    
    /// load a dynamic library from plugin_file and return its handle
    static void *load_dynamic_library(const std::string& plugin_file);
    
    /// close the dynamic library corresponding with lib_handle
    static void close_dynamic_library(void *lib_handle);
    
    /// returns the address of a library symbal
    static void *get_library_symbol(void *lib_handle, const std::string& symbol);
    
    
    /// name of function defined in object code to create a new plug-in instance
    static const std::string            PION_PLUGIN_CREATE;
    
    /// name of function defined in object code to destroy a plug-in instance
    static const std::string            PION_PLUGIN_DESTROY;
    
    /// file extension used for Pion plug-in files (platform specific)
    static const std::string            PION_PLUGIN_EXTENSION;
    
    /// file extension used for Pion configuration files
    static const std::string            PION_CONFIG_EXTENSION;
    
    /// used to ensure thread safety of the plugin_config singleton
    static std::once_flag             m_instance_flag;

    /// pointer to the plugin_config singleton
    static config_type *           m_config_ptr;

    /// points to the shared library and functions used by the plug-in
    data_type *                    m_plugin_data;
};


///
/// plugin_ptr: smart pointer that manages plug-in code loaded from shared
///                object libraries
///
template <typename InterfaceClassType>
class plugin_ptr :
    public plugin
{
protected:
    
    /// data type for a function that is used to create object instances
    typedef InterfaceClassType* CreateObjectFunction(void);
    
    /// data type for a function that is used to destroy object instances
    typedef void DestroyObjectFunction(InterfaceClassType*);

    
public:

    /// default constructor & destructor
    plugin_ptr(void) : plugin() {}
    virtual ~plugin_ptr() {}
    
    /// copy constructor
    plugin_ptr(const plugin_ptr& p) : plugin(p) {}

    /// assignment operator
    plugin_ptr& operator=(const plugin_ptr& p) { grab_data(p); return *this; }

    /// creates a new instance of the plug-in object
    inline InterfaceClassType *create(void) {
        CreateObjectFunction *create_func =
            (CreateObjectFunction*)(get_create_function());
        if (create_func == NULL)
            BOOST_THROW_EXCEPTION( error::plugin_undefined() );
        return create_func();
    }
    
    /// destroys an instance of the plug-in object
    inline void destroy(InterfaceClassType *object_ptr) {
        // fix warning ISO C++ forbids casting from pointer-to-object
        // to pointer to function
        union {
            void* v_;
            DestroyObjectFunction* f_;
        } Cast;
        Cast.v_ = get_destroy_function();
        DestroyObjectFunction *destroy_func = Cast.f_;
        if (destroy_func == NULL)
            BOOST_THROW_EXCEPTION( error::plugin_undefined() );
        destroy_func(object_ptr);
    }
};


///
/// plugin_instance_ptr: smart pointer that manages a plug-in instance
///
template <typename InterfaceClassType>
class plugin_instance_ptr :
    private boost::noncopyable
{
public:

    /// default constructor & destructor
    plugin_instance_ptr(void) : m_instance_ptr(NULL) {}
    
    /// virtual destructor / may be extended
    virtual ~plugin_instance_ptr() { reset(); }
    
    /// reset the instance pointer
    inline void reset(void) { 
        if (m_instance_ptr) {
            m_plugin_ptr.destroy(m_instance_ptr);
        }
    }
    
    /// create a new instance of the given plugin_type
    inline void create(const std::string& plugin_type) {
        reset();
        m_plugin_ptr.open(plugin_type);
        m_instance_ptr = m_plugin_ptr.create();
    }
    
    /// returns true if pointer is empty
    inline bool empty(void) const { return m_instance_ptr==NULL; }
    
    /// return a raw pointer to the instance
    inline InterfaceClassType *get(void) { return m_instance_ptr; }
    
    /// return a reference to the instance
    inline InterfaceClassType& operator*(void) { return *m_instance_ptr; }

    /// return a const reference to the instance
    inline const InterfaceClassType& operator*(void) const { return *m_instance_ptr; }

    /// return a reference to the instance
    inline InterfaceClassType* operator->(void) { return m_instance_ptr; }

    /// return a const reference to the instance
    inline const InterfaceClassType* operator->(void) const { return m_instance_ptr; }
    
    
protected:

    /// smart pointer that manages the plugin's dynamic object code
    plugin_ptr<InterfaceClassType>   m_plugin_ptr;
    
    /// raw pointer to the plugin instance
    InterfaceClassType  *               m_instance_ptr;
};


/**
* Macros to declare entry points for statically linked plugins in accordance
* with the general naming convention.
*
* Typical use:
* @code
* PION_DECLARE_PLUGIN(EchoService)
* ....
* PION_DECLARE_PLUGIN(FileService)
*
* @endcode
*
*/
#ifdef PION_STATIC_LINKING

#define PION_DECLARE_PLUGIN(plugin_name)    \
    class plugin_name;                      \
    extern "C" plugin_name *pion_create_##plugin_name(void); \
    extern "C" void pion_destroy_##plugin_name(plugin_name *plugin_ptr); \
    static pion::static_entry_point_helper helper_##plugin_name(#plugin_name, (void*) pion_create_##plugin_name, (void*) pion_destroy_##plugin_name);

/// used by PION_DECLARE_PLUGIN to add an entry point for static-linked plugins
class static_entry_point_helper {
public:
    static_entry_point_helper(const std::string& name, void *create, void *destroy)
    {
        pion::plugin::add_static_entry_point(name, create, destroy);
    }
};

#else

#define PION_DECLARE_PLUGIN(plugin_name)

#endif

}   // end namespace pion

#endif
