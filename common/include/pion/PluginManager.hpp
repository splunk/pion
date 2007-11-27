// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PLUGINMANAGER_HEADER__
#define __PION_PLUGINMANAGER_HEADER__

#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <pion/PionConfig.hpp>
#include <pion/PionException.hpp>
#include <pion/PionPlugin.hpp>
#include <string>
#include <map>


namespace pion {	// begin namespace pion

///
/// PluginManager: used to manage a collection of plug-in objects
///
template <typename PLUGIN_TYPE>
class PION_COMMON_API PluginManager
{
public:

	/// exception thrown if a plug-in cannot be found
	class PluginNotFoundException : public PionException {
	public:
		PluginNotFoundException(const std::string& plugin_id)
			: PionException("No plug-ins found for identifier: ", plugin_id) {}
	};

	/// data type for a function that may be called by the run() method
	typedef boost::function1<void, PLUGIN_TYPE*>	PluginFunction;

	
	/// default constructor
	PluginManager(void) {}

	/// default destructor
	virtual ~PluginManager() {}

	/// clears all the plug-in objects being managed
	inline void clear(void) {
		boost::mutex::scoped_lock plugins_lock(m_plugin_mutex);
		m_plugin_map.clear();
	}
	
	/// returns true if there are no plug-in objects being managed
	inline bool empty(void) const { 
		boost::mutex::scoped_lock plugins_lock(m_plugin_mutex);
		return m_plugin_map.empty();
	}
	
	/**
	 * adds a new plug-in object
	 *
	 * @param plugin_id unique identifier associated with the plug-in
	 * @param plugin_object_ptr pointer to the plug-in object to add
	 */
	inline void add(const std::string& plugin_id, PLUGIN_TYPE *plugin_object_ptr);
	
	/**
	 * loads a new plug-in object
	 *
	 * @param plugin_id unique identifier associated with the plug-in
	 * @param plugin_name the name or type of the plug-in to load (searches
	 *                    plug-in directories and appends extensions)
	 * @return PLUGIN_TYPE* pointer to the new plug-in object
	 */
	inline PLUGIN_TYPE *load(const std::string& plugin_id, const std::string& plugin_name);
	
	/**
	 * finds the plug-in object associated with a particular resource
	 *
	 * @param resource resource identifier (uri-stem) to search for
	 * @return PLUGIN_TYPE* pointer to the matching plug-in object or NULL if not found
	 */
	inline PLUGIN_TYPE *find(const std::string& resource);
	
	/**
	 * runs a method for each plug-in being managed
	 *
	 * @param run_func the function to execute for each plug-in object
	 */
	inline void run(PluginFunction run_func);
	
	/**
	 * sets a configuration option for a managed plug-in
	 *
	 * @param plugin_id unique identifier associated with the plug-in
	 * @param option_name the name of the configuration option
	 * @param option_value the value to set the option to
	 */
	inline void setOption(const std::string& plugin_id,
						  const std::string& option_name,
						  const std::string& option_value);
	
	
protected:
	
	/// data type that maps identifiers to plug-in objects
	class PluginMap
		: public std::map<std::string, std::pair<PLUGIN_TYPE *, PionPluginPtr<PLUGIN_TYPE> > >
	{
	public:
		inline void clear(void);
		virtual ~PluginMap() { PluginMap::clear(); }
		PluginMap(void) {}
	};
	
	/// collection of plug-in objects being managed
	PluginMap						m_plugin_map;

	/// mutex to make class thread-safe
	mutable boost::mutex			m_plugin_mutex;
};

	
// PluginManager member functions

template <typename PLUGIN_TYPE>
inline void PluginManager<PLUGIN_TYPE>::add(const std::string& plugin_id,
											PLUGIN_TYPE *plugin_object_ptr)
{
	PionPluginPtr<PLUGIN_TYPE> plugin_ptr;
	boost::mutex::scoped_lock plugins_lock(m_plugin_mutex);
	m_plugin_map.insert(std::make_pair(plugin_id,
									   std::make_pair(plugin_object_ptr, plugin_ptr)));
}

template <typename PLUGIN_TYPE>
inline PLUGIN_TYPE *PluginManager<PLUGIN_TYPE>::load(const std::string& plugin_id,
													 const std::string& plugin_name)
{
	// search for the plug-in file using the configured paths
	bool is_static;
	void *create_func;
	void *destroy_func;
	
	// check if plug-in is statically linked, and if not, try to resolve for dynamic
	is_static = PionPlugin::findStaticEntryPoint(plugin_name, &create_func, &destroy_func);
	
	// open up the plug-in's shared object library
	PionPluginPtr<PLUGIN_TYPE> plugin_ptr;
	if (is_static) {
		plugin_ptr.openStaticLinked(plugin_name, create_func, destroy_func);	// may throw
	} else {
		plugin_ptr.open(plugin_name);	// may throw
	}
	
	// create a new object using the plug-in library
	PLUGIN_TYPE *plugin_object_ptr(plugin_ptr.create());
	
	// add the new plug-in object to our map
	boost::mutex::scoped_lock plugins_lock(m_plugin_mutex);
	m_plugin_map.insert(std::make_pair(plugin_id,
									   std::make_pair(plugin_object_ptr, plugin_ptr)));

	return plugin_object_ptr;
}

template <typename PLUGIN_TYPE>
inline PLUGIN_TYPE *PluginManager<PLUGIN_TYPE>::find(const std::string& resource)
{
	// will point to the matching plug-in object, if found
	PLUGIN_TYPE *plugin_object_ptr = NULL;
	
	// lock mutex for thread safety (this should probably use ref counters)
	boost::mutex::scoped_lock plugins_lock(m_plugin_mutex);
	
	// check if no plug-ins are being managed
	if (m_plugin_map.empty()) return plugin_object_ptr;
	
	// iterate through each plug-in whose identifier may match the resource
	typename pion::PluginManager<PLUGIN_TYPE>::PluginMap::iterator i = m_plugin_map.upper_bound(resource);
	while (i != m_plugin_map.begin()) {
		--i;
		
		// keep checking while the first part of the strings match
		if (resource.compare(0, i->first.size(), i->first) != 0) {
			// the first part no longer matches
			if (i != m_plugin_map.begin()) {
				// continue to next plug-in in list if its size is < this one
				typename pion::PluginManager<PLUGIN_TYPE>::PluginMap::iterator j=i;
				--j;
				if (j->first.size() < i->first.size())
					continue;
			}
			// otherwise we've reached the end; stop looking for a match
			break;
		}
		
		// only if the resource matches the plug-in's identifier
		// or if resource is followed first with a '/' character
		if (resource.size() == i->first.size() || resource[i->first.size()]=='/') {
			plugin_object_ptr = i->second.first;
			break;
		}
	}
	
	return plugin_object_ptr;
}
	
template <typename PLUGIN_TYPE>
inline void PluginManager<PLUGIN_TYPE>::setOption(const std::string& plugin_id,
												  const std::string& option_name,
												  const std::string& option_value)
{
	boost::mutex::scoped_lock plugins_lock(m_plugin_mutex);
	// find the plug-in & set the option
	// if plugin_id == "/" then look for identifier with an empty string
	typename pion::PluginManager<PLUGIN_TYPE>::PluginMap::iterator i = (plugin_id == "/" ? m_plugin_map.find("") : m_plugin_map.find(plugin_id));
	if (i == m_plugin_map.end())
		throw PluginNotFoundException(plugin_id);
	i->second.first->setOption(option_name, option_value);
}

template <typename PLUGIN_TYPE>
inline void PluginManager<PLUGIN_TYPE>::run(PluginFunction run_func)
{
	boost::mutex::scoped_lock plugins_lock(m_plugin_mutex);
	for (typename pion::PluginManager<PLUGIN_TYPE>::PluginMap::iterator i = m_plugin_map.begin();
		 i != m_plugin_map.end(); ++i)
	{
		run_func(i->second.first);
	}
}


// PluginManager::PluginMap member functions

template <typename PLUGIN_TYPE>
inline void PluginManager<PLUGIN_TYPE>::PluginMap::clear(void)
{
	if (! empty()) {
		for (typename pion::PluginManager<PLUGIN_TYPE>::PluginMap::iterator i =
			 std::map<std::string, std::pair<PLUGIN_TYPE *, PionPluginPtr<PLUGIN_TYPE> > >::begin();
			 i != std::map<std::string, std::pair<PLUGIN_TYPE *, PionPluginPtr<PLUGIN_TYPE> > >::end(); ++i)
		{
			if (i->second.second.is_open()) {
				i->second.second.destroy(i->second.first);
			} else {
				delete i->second.first;
			}
		}
		erase(std::map<std::string, std::pair<PLUGIN_TYPE *, PionPluginPtr<PLUGIN_TYPE> > >::begin(),
			  std::map<std::string, std::pair<PLUGIN_TYPE *, PionPluginPtr<PLUGIN_TYPE> > >::end());
	}
}


}	// end namespace pion

#endif
