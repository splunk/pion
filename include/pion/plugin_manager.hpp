// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PLUGIN_MANAGER_HEADER__
#define __PION_PLUGIN_MANAGER_HEADER__

#include <map>
#include <string>
#include <boost/cstdint.hpp>
#include <boost/assert.hpp>
#include <boost/function.hpp>
#include <boost/function/function1.hpp>
#include <boost/thread/mutex.hpp>
#include <pion/config.hpp>
#include <pion/error.hpp>
#include <pion/plugin.hpp>


namespace pion {    // begin namespace pion

///
/// PluginManager: used to manage a collection of plug-in objects
///
template <typename PLUGIN_TYPE>
class PluginManager
{
public:

    /// data type for a function that may be called by the run() method
    typedef boost::function1<void, PLUGIN_TYPE*>    PluginRunFunction;

    /// data type for a function that may be called by the getStat() method
    typedef boost::function1<boost::uint64_t, const PLUGIN_TYPE*>   PluginStatFunction;

    
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
     * removes a plug-in object
     *
     * @param plugin_id unique identifier associated with the plug-in
     */
    inline void remove(const std::string& plugin_id);
    
    /**
     * replaces an existing plug-in object with a new one
     *
     * @param plugin_id unique identifier associated with the plug-in
     * @param plugin_ptr pointer to the new plug-in object which will replace the old one
     */
    inline void replace(const std::string& plugin_id, PLUGIN_TYPE *plugin_ptr);
    
    /**
     * clones an existing plug-in object (creates a new one of the same type)
     *
     * @param plugin_id unique identifier associated with the plug-in
     * @return PLUGIN_TYPE* pointer to the new plug-in object
     */
    inline PLUGIN_TYPE *clone(const std::string& plugin_id);

    /**
     * loads a new plug-in object
     *
     * @param plugin_id unique identifier associated with the plug-in
     * @param plugin_type the name or type of the plug-in to load (searches
     *                    plug-in directories and appends extensions)
     * @return PLUGIN_TYPE* pointer to the new plug-in object
     */
    inline PLUGIN_TYPE *load(const std::string& plugin_id, const std::string& plugin_type);
    
    /**
     * gets the plug-in object associated with a particular plugin_id (exact match)
     *
     * @param plugin_id unique identifier associated with the plug-in
     * @return PLUGIN_TYPE* pointer to the matching plug-in object or NULL if not found
     */
    inline PLUGIN_TYPE *get(const std::string& plugin_id);
    
    /**
     * gets the plug-in object associated with a particular plugin_id (exact match)
     *
     * @param plugin_id unique identifier associated with the plug-in
     * @return PLUGIN_TYPE* pointer to the matching plug-in object or NULL if not found
     */
    inline const PLUGIN_TYPE *get(const std::string& plugin_id) const;
    
    /**
     * gets a smart pointer to the plugin shared library for a particular plugin_id (exact match)
     *
     * @param plugin_id unique identifier associated with the plug-in
     * @return PionPluginPtr<PLUGIN_TYPE> pointer to the plugin shared library if found
     */
    inline PionPluginPtr<PLUGIN_TYPE> getLibPtr(const std::string& plugin_id) const;
    
    /**
     * finds the plug-in object associated with a particular resource (fuzzy match)
     *
     * @param resource resource identifier (uri-stem) to search for
     * @return PLUGIN_TYPE* pointer to the matching plug-in object or NULL if not found
     */
    inline PLUGIN_TYPE *find(const std::string& resource);
    
    /**
     * runs a method for every plug-in being managed
     *
     * @param run_func the function to execute for each plug-in object
     */
    inline void run(PluginRunFunction run_func);
    
    /**
     * runs a method for a particular plug-in
     *
     * @param plugin_id unique identifier associated with the plug-in
     * @param run_func the function to execute
     */
    inline void run(const std::string& plugin_id, PluginRunFunction run_func);
    
    /**
     * returns a total statistic value summed for every plug-in being managed
     *
     * @param stat_func the statistic function to execute for each plug-in object
     */
    inline boost::uint64_t getStatistic(PluginStatFunction stat_func) const;
    
    /**
     * returns a statistic value for a particular plug-in
     *
     * @param plugin_id unique identifier associated with the plug-in
     * @param stat_func the statistic function to execute
     */
    inline boost::uint64_t getStatistic(const std::string& plugin_id,
                                        PluginStatFunction stat_func) const;
        
    
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
    PluginMap                       m_plugin_map;

    /// mutex to make class thread-safe
    mutable boost::mutex            m_plugin_mutex;
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
inline void PluginManager<PLUGIN_TYPE>::remove(const std::string& plugin_id)
{
    boost::mutex::scoped_lock plugins_lock(m_plugin_mutex);
    typename pion::PluginManager<PLUGIN_TYPE>::PluginMap::iterator i = m_plugin_map.find(plugin_id);
    if (i == m_plugin_map.end())
        BOOST_THROW_EXCEPTION( error::plugin_not_found() << error::errinfo_plugin_name(plugin_id) );
    if (i->second.second.is_open()) {
        i->second.second.destroy(i->second.first);
    } else {
        delete i->second.first;
    }
    m_plugin_map.erase(i);
}

template <typename PLUGIN_TYPE>
inline void PluginManager<PLUGIN_TYPE>::replace(const std::string& plugin_id, PLUGIN_TYPE *plugin_ptr)
{
    BOOST_ASSERT(plugin_ptr);
    boost::mutex::scoped_lock plugins_lock(m_plugin_mutex);
    typename pion::PluginManager<PLUGIN_TYPE>::PluginMap::iterator i = m_plugin_map.find(plugin_id);
    if (i == m_plugin_map.end())
        BOOST_THROW_EXCEPTION( error::plugin_not_found() << error::errinfo_plugin_name(plugin_id) );
    if (i->second.second.is_open()) {
        i->second.second.destroy(i->second.first);
    } else {
        delete i->second.first;
    }
    i->second.first = plugin_ptr;
}

template <typename PLUGIN_TYPE>
inline PLUGIN_TYPE *PluginManager<PLUGIN_TYPE>::clone(const std::string& plugin_id)
{
    boost::mutex::scoped_lock plugins_lock(m_plugin_mutex);
    typename pion::PluginManager<PLUGIN_TYPE>::PluginMap::iterator i = m_plugin_map.find(plugin_id);
    if (i == m_plugin_map.end())
        BOOST_THROW_EXCEPTION( error::plugin_not_found() << error::errinfo_plugin_name(plugin_id) );
    return i->second.second.create();
}

template <typename PLUGIN_TYPE>
inline PLUGIN_TYPE *PluginManager<PLUGIN_TYPE>::load(const std::string& plugin_id,
                                                     const std::string& plugin_type)
{
    // search for the plug-in file using the configured paths
    if (m_plugin_map.find(plugin_id) != m_plugin_map.end())
        BOOST_THROW_EXCEPTION( error::duplicate_plugin() << error::errinfo_plugin_name(plugin_id) );
    
    // open up the plug-in's shared object library
    PionPluginPtr<PLUGIN_TYPE> plugin_ptr;
    plugin_ptr.open(plugin_type);   // may throw
    
    // create a new object using the plug-in library
    PLUGIN_TYPE *plugin_object_ptr(plugin_ptr.create());
    
    // add the new plug-in object to our map
    boost::mutex::scoped_lock plugins_lock(m_plugin_mutex);
    m_plugin_map.insert(std::make_pair(plugin_id,
                                       std::make_pair(plugin_object_ptr, plugin_ptr)));

    return plugin_object_ptr;
}

template <typename PLUGIN_TYPE>
inline PLUGIN_TYPE *PluginManager<PLUGIN_TYPE>::get(const std::string& plugin_id)
{
    PLUGIN_TYPE *plugin_object_ptr = NULL;
    boost::mutex::scoped_lock plugins_lock(m_plugin_mutex);
    typename pion::PluginManager<PLUGIN_TYPE>::PluginMap::iterator i = m_plugin_map.find(plugin_id);
    if (i != m_plugin_map.end())
        plugin_object_ptr = i->second.first;
    return plugin_object_ptr;
}
    
template <typename PLUGIN_TYPE>
inline const PLUGIN_TYPE *PluginManager<PLUGIN_TYPE>::get(const std::string& plugin_id) const
{
    const PLUGIN_TYPE *plugin_object_ptr = NULL;
    boost::mutex::scoped_lock plugins_lock(m_plugin_mutex);
    typename pion::PluginManager<PLUGIN_TYPE>::PluginMap::const_iterator i = m_plugin_map.find(plugin_id);
    if (i != m_plugin_map.end())
        plugin_object_ptr = i->second.first;
    return plugin_object_ptr;
}
    
template <typename PLUGIN_TYPE>
inline PionPluginPtr<PLUGIN_TYPE> PluginManager<PLUGIN_TYPE>::getLibPtr(const std::string& plugin_id) const
{
    PionPluginPtr<PLUGIN_TYPE> plugin_ptr;
    boost::mutex::scoped_lock plugins_lock(m_plugin_mutex);
    typename pion::PluginManager<PLUGIN_TYPE>::PluginMap::const_iterator i = m_plugin_map.find(plugin_id);
    if (i != m_plugin_map.end())
        plugin_ptr = i->second.second;
    return plugin_ptr;
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
inline void PluginManager<PLUGIN_TYPE>::run(PluginRunFunction run_func)
{
    boost::mutex::scoped_lock plugins_lock(m_plugin_mutex);
    for (typename pion::PluginManager<PLUGIN_TYPE>::PluginMap::iterator i = m_plugin_map.begin();
         i != m_plugin_map.end(); ++i)
    {
        run_func(i->second.first);
    }
}

template <typename PLUGIN_TYPE>
inline void PluginManager<PLUGIN_TYPE>::run(const std::string& plugin_id,
                                            PluginRunFunction run_func)
{
    // no need to lock (handled by PluginManager::get())
    PLUGIN_TYPE *plugin_object_ptr = get(plugin_id);
    if (plugin_object_ptr == NULL)
        BOOST_THROW_EXCEPTION( error::plugin_not_found() << error::errinfo_plugin_name(plugin_id) );
    run_func(plugin_object_ptr);
}

template <typename PLUGIN_TYPE>
inline boost::uint64_t PluginManager<PLUGIN_TYPE>::getStatistic(PluginStatFunction stat_func) const
{
    boost::uint64_t stat_value = 0;
    boost::mutex::scoped_lock plugins_lock(m_plugin_mutex);
    for (typename pion::PluginManager<PLUGIN_TYPE>::PluginMap::const_iterator i = m_plugin_map.begin();
         i != m_plugin_map.end(); ++i)
    {
        stat_value += stat_func(i->second.first);
    }
    return stat_value;
}

template <typename PLUGIN_TYPE>
inline boost::uint64_t PluginManager<PLUGIN_TYPE>::getStatistic(const std::string& plugin_id,
                                                                PluginStatFunction stat_func) const
{
    // no need to lock (handled by PluginManager::get())
    const PLUGIN_TYPE *plugin_object_ptr = const_cast<PluginManager<PLUGIN_TYPE>*>(this)->get(plugin_id);
    if (plugin_object_ptr == NULL)
        BOOST_THROW_EXCEPTION( error::plugin_not_found() << error::errinfo_plugin_name(plugin_id) );
    return stat_func(plugin_object_ptr);
}


// PluginManager::PluginMap member functions

template <typename PLUGIN_TYPE>
inline void PluginManager<PLUGIN_TYPE>::PluginMap::clear(void)
{
    if (! std::map<std::string, std::pair<PLUGIN_TYPE *, PionPluginPtr<PLUGIN_TYPE> > >::empty()) {
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
        this->erase(std::map<std::string, std::pair<PLUGIN_TYPE *, PionPluginPtr<PLUGIN_TYPE> > >::begin(),
              std::map<std::string, std::pair<PLUGIN_TYPE *, PionPluginPtr<PLUGIN_TYPE> > >::end());
    }
}


}   // end namespace pion

#endif
