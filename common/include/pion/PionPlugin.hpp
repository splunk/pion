// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PIONPLUGIN_HEADER__
#define __PION_PIONPLUGIN_HEADER__

#include <vector>
#include <string>
#include <map>
#include <list>
#include <boost/thread/mutex.hpp>
#include <boost/filesystem/path.hpp>
#include <pion/PionConfig.hpp>
#include <pion/PionException.hpp>


namespace pion {	// begin namespace pion

///
/// PionPlugin: base class for plug-in management
///
class PION_COMMON_API PionPlugin {
public:

	/// exception thrown if the plug-in file cannot be opened
	class PluginUndefinedException : public std::exception {
	public:
		virtual const char* what() const throw() {
			return "Plug-in was not loaded properly";
		}
	};
	
	/// exception thrown if the plug-in directory does not exist
	class DirectoryNotFoundException : public PionException {
	public:
		DirectoryNotFoundException(const std::string& dir)
			: PionException("Plug-in directory not found: ", dir) {}
	};

	/// exception thrown if the plug-in file cannot be opened
	class PluginNotFoundException : public PionException {
	public:
		PluginNotFoundException(const std::string& file)
			: PionException("Plug-in library not found: ", file) {}
	};
	
	/// exception thrown if a plug-in library is missing the create() function
	class PluginMissingCreateException : public PionException {
	public:
		PluginMissingCreateException(const std::string& file)
			: PionException("Plug-in library does not include create() symbol: ", file) {}
	};
	
	/// exception thrown if a plug-in library is missing the destroy() function
	class PluginMissingDestroyException : public PionException {
	public:
		PluginMissingDestroyException(const std::string& file)
			: PionException("Plug-in library does not include destroy() symbol: ", file) {}
	};

	/**
	 * searches directories for a valid plug-in file
	 *
	 * @param path_to_file the path to the plug-in file, if found
	 * @param the name name of the plug-in to search for
	 * @return true if the plug-in file was found
	 */
	static inline bool findPluginFile(std::string& path_to_file,
									  const std::string& name)
	{
		return findFile(path_to_file, name, PION_PLUGIN_EXTENSION);
	}

	/**
	 * searches directories for a valid plug-in configuration file
	 *
	 * @param path_to_file if found, is set to the complete path to the file
	 * @param name the name of the configuration file to search for
	 * @return true if the configuration file was found
	 */
	static inline bool findConfigFile(std::string& path_to_file,
									  const std::string& name)
	{
		return findFile(path_to_file, name, PION_CONFIG_EXTENSION);
	}
	
	/**
	 * finds an entry point for a plugin that is statically linked
	 *
	 * @param plugin_name the name of the plugin to look for
	 * @param create_func - pointer to the function to be used in to create plugin object
	 * @param destroy_func - pointer to the function to be used to release plugin object
	 */
	static bool findStaticEntryPoint(const std::string& plugin_name,
									 void **create_func,
									 void **destroy_func);
	
	/**
	 * adds an entry point for a plugin that is statically linked
	 *
	 * @param plugin_name the name of the plugin to add
	 * @param create_func - pointer to the function to be used in to create plugin object
	 * @param destroy_func - pointer to the function to be used to release plugin object
	 */
	static void addStaticEntryPoint(const std::string& plugin_name,
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
	static void checkCygwinPath(boost::filesystem::path& final_path,
								const std::string& path_string);

	/// appends a directory to the plug-in search path
	static void addPluginDirectory(const std::string& dir);
	
	/// clears all directories from the plug-in search path
	static void resetPluginDirectories(void);
	

	// default destructor
	virtual ~PionPlugin() { releaseData(); }
	
	/// returns true if a shared library is loaded/open
	inline bool is_open(void) const { return (m_plugin_data != NULL); }
	
	/// returns the name of the plugin that is currently open
	inline std::string getPluginName(void) const {
		return (is_open() ? m_plugin_data->m_plugin_name : std::string());
	}
	
	/**
	 * opens plug-in library within a shared object file.  If the library is
	 * already being used by another PionPlugin object, then the existing
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
	 * already being used by another PionPlugin object, then the existing
	 * code will be re-used and the reference count will be increased.  Beware
	 * that this does NOT check the plug-in's base class (InterfaceClassType),
	 * so you must be careful to ensure that the namespace is unique between
	 * plug-ins that have different base classes.  If the plug-in's name matches
	 * an existing plug-in of a different base class, the resulting behavior is
	 * undefined (it will probably crash your program).
	 * 
	 * @param plugin_file shared object file containing the plugin code
	 */
	void openFile(const std::string& plugin_file);

	/**
	* opens plug-in library that is statically linked into the program
	* 
	* @param plugin_name plugin name to be used in future references
	* @param create_func - pointer to the function to be used in to create plugin object
	* @param destroy_func - pointer to the function to be used to release plugin object
	*
	*/
	void openStaticLinked(const std::string& plugin_name,
						  void *create_func,
						  void *destroy_func);

	/// closes plug-in library
	inline void close(void) { releaseData(); }

protected:
	
	///
	/// PionPluginData: object to hold shared library symbols
	///
	struct PionPluginData
	{
		/// default constructors for convenience
		PionPluginData(void)
			: m_lib_handle(NULL), m_create_func(NULL), m_destroy_func(NULL),
			m_references(0)
		{}
		PionPluginData(const std::string& plugin_name)
			: m_lib_handle(NULL), m_create_func(NULL), m_destroy_func(NULL),
			m_plugin_name(plugin_name), m_references(0)
		{}
		PionPluginData(const PionPluginData& p)
			: m_lib_handle(p.m_lib_handle), m_create_func(p.m_create_func),
			m_destroy_func(p.m_destroy_func), m_plugin_name(p.m_plugin_name),
			m_references(p.m_references)
		{}
		
		/// symbol library loaded from a shared object file
		void *			m_lib_handle;
		
		/// function used to create instances of the plug-in object
		void *			m_create_func;
		
		/// function used to destroy instances of the plug-in object
		void *			m_destroy_func;
		
		/// the name of the plugin (must be unique per process)
		std::string		m_plugin_name;
		
		/// number of references to this class
		unsigned long	m_references;
	};

	
	/// default constructor is private (use PionPluginPtr class to create objects)
	PionPlugin(void) : m_plugin_data(NULL) {}
	
	/// copy constructor
	PionPlugin(const PionPlugin& p) : m_plugin_data(NULL) { grabData(p); }

	/// assignment operator
	PionPlugin& operator=(const PionPlugin& p) { grabData(p); return *this; }

	/// returns a pointer to the plug-in's "create object" function
	inline void *getCreateFunction(void) {
		return (is_open() ? m_plugin_data->m_create_func : NULL);
	}

	/// returns a pointer to the plug-in's "destroy object" function
	inline void *getDestroyFunction(void) {
		return (is_open() ? m_plugin_data->m_destroy_func : NULL);
	}

	/// releases the plug-in's shared library symbols
	void releaseData(void);
	
	/// grabs a reference to another plug-in's shared library symbols
	void grabData(const PionPlugin& p);

	
private:

	/// data type for keeping track of the entry points for static plugins
	class StaticEntryPoint
	{
	public:
		StaticEntryPoint(const std::string& name, void *create, void *destroy)
			: m_plugin_name(name), m_create_func(create), m_destroy_func(destroy)
			{}
		std::string  m_plugin_name;
		void *       m_create_func;
		void *       m_destroy_func;
	};

	/// data type for a list of static entry points
	typedef std::list<StaticEntryPoint>		StaticEntryPointList;
	
	/// data type that maps plug-in names to their shared library data
	typedef std::map<std::string, PionPluginData*>	PluginMap;

	
	/**
	 * searches directories for a valid plug-in file
	 *
	 * @param path_to_file if found, is set to the complete path to the file
	 * @param name the name of the file to search for
	 * @param extension will be appended to name if name is not found
	 *
	 * @return true if the file was found
	 */
	static bool findFile(std::string& path_to_file, const std::string& name,							 
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
	static bool checkForFile(std::string& final_path, const std::string& start_path,
							 const std::string& name, const std::string& extension);
	
	/**
	 * opens plug-in library within a shared object file
	 * 
	 * @param plugin_file shared object file containing the plugin code
	 * @param plugin_data data object to load the library into
	 */
	static void openPlugin(const std::string& plugin_file,
						   PionPluginData& plugin_data);

	/// returns the name of the plugin object (based on the plugin_file name)
	static std::string getPluginName(const std::string& plugin_file);
	
	/// load a dynamic library from plugin_file and return its handle
	static void *loadDynamicLibrary(const std::string& plugin_file);
	
	/// close the dynamic library corresponding with lib_handle
	static void closeDynamicLibrary(void *lib_handle);
	
	/// returns the address of a library symbal
	static void *getLibrarySymbol(void *lib_handle, const std::string& symbol);
	
	
	/// name of function defined in object code to create a new plug-in instance
	static const std::string			PION_PLUGIN_CREATE;
	
	/// name of function defined in object code to destroy a plug-in instance
	static const std::string			PION_PLUGIN_DESTROY;
	
	/// file extension used for Pion plug-in files (platform specific)
	static const std::string			PION_PLUGIN_EXTENSION;
	
	/// file extension used for Pion configuration files
	static const std::string			PION_CONFIG_EXTENSION;
	
	/// directories containing plugin files
	static std::vector<std::string>		m_plugin_dirs;
	
	/// maps plug-in names to shared library data
	static PluginMap					m_plugin_map;
	
	/// mutex to make class thread-safe
	static boost::mutex					m_plugin_mutex;

	/// list of entry points for statically linked plugins
	static StaticEntryPointList			*m_entry_points_ptr;

	/// points to the shared library and functions used by the plug-in
	PionPluginData *					m_plugin_data;
};


///
/// PionPluginPtr: smart pointer that manages plug-in code loaded from shared
///                object libraries
///
template <typename InterfaceClassType>
class PionPluginPtr :
	public PionPlugin
{
protected:
	
	/// data type for a function that is used to create object instances
	typedef InterfaceClassType* CreateObjectFunction(void);
	
	/// data type for a function that is used to destroy object instances
	typedef void DestroyObjectFunction(InterfaceClassType*);

	
public:

	/// default constructor & destructor
	PionPluginPtr(void) : PionPlugin() {}
	virtual ~PionPluginPtr() {}
	
	/// copy constructor
	PionPluginPtr(const PionPluginPtr& p) : PionPlugin(p) {}

	/// assignment operator
	PionPluginPtr& operator=(const PionPluginPtr& p) { grabData(p); return *this; }

	/// creates a new instance of the plug-in object
	inline InterfaceClassType *create(void) {
		CreateObjectFunction *create_func =
			(CreateObjectFunction*)(getCreateFunction());
		if (create_func == NULL)
			throw PluginUndefinedException();
		return create_func();
	}
	
	/// destroys an instance of the plug-in object
	inline void destroy(InterfaceClassType *object_ptr) {
		DestroyObjectFunction *destroy_func =
			(DestroyObjectFunction*)(getDestroyFunction());
		if (destroy_func == NULL)
			throw PluginUndefinedException();
		destroy_func(object_ptr);
	}
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

#define PION_DECLARE_PLUGIN(plugin_name)	\
	class plugin_name;						\
	extern "C" plugin_name *pion_create_##plugin_name(void); \
	extern "C" void pion_destroy_##plugin_name(plugin_name *plugin_ptr); \
	static pion::StaticEntryPointHelper helper_##plugin_name(#plugin_name, pion_create_##plugin_name, pion_destroy_##plugin_name);

/// used by PION_DECLARE_PLUGIN to add an entry point for static-linked plugins
class StaticEntryPointHelper {
public:
	StaticEntryPointHelper(const std::string& name, void *create, void *destroy)
	{
		pion::PionPlugin::addStaticEntryPoint(name, create, destroy);
	}
};

#else

#define PION_DECLARE_PLUGIN(plugin_name)

#endif

}	// end namespace pion

#endif
