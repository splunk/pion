// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PLUGIN_MANAGER_HEADER__
#define __PION_PLUGIN_MANAGER_HEADER__

#include <map>
#include <string>
#include <functional>
#include <mutex>
#include <boost/assert.hpp>
#include <pion/config.hpp>
#include <pion/error.hpp>
#include <pion/plugin.hpp>


namespace pion {    // begin namespace pion

///
/// plugin_manager: used to manage a collection of plug-in objects
///
template <typename PluginType>
class plugin_manager
{
public:

    /// data type for a function that may be called by the run() method
    typedef std::function<void(PluginType*)>    PluginRunFunction;

    /// data type for a function that may be called by the getStat() method
    typedef std::function<std::uint64_t(const PluginType*)>   PluginStatFunction;

    
    /// default constructor
    plugin_manager(void) {}

    /// default destructor
    virtual ~plugin_manager() {}

    /// clears all the plug-in objects being managed
    inline void clear(void) {
        std::unique_lock<std::mutex> plugins_lock(m_plugin_mutex);
        m_plugin_map.clear();
    }
    
    /// returns true if there are no plug-in objects being managed
    inline bool empty(void) const { 
        std::unique_lock<std::mutex> plugins_lock(m_plugin_mutex);
        return m_plugin_map.empty();
    }
    
    /**
     * adds a new plug-in object
     *
     * @param plugin_id unique identifier associated with the plug-in
     * @param plugin_object_ptr pointer to the plug-in object to add
     */
    inline void add(const std::string& plugin_id, PluginType *plugin_object_ptr);
    
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
    inline void replace(const std::string& plugin_id, PluginType *plugin_ptr);
    
    /**
     * clones an existing plug-in object (creates a new one of the same type)
     *
     * @param plugin_id unique identifier associated with the plug-in
     * @return PluginType* pointer to the new plug-in object
     */
    inline PluginType *clone(const std::string& plugin_id);

    /**
     * loads a new plug-in object
     *
     * @param plugin_id unique identifier associated with the plug-in
     * @param plugin_type the name or type of the plug-in to load (searches
     *                    plug-in directories and appends extensions)
     * @return PluginType* pointer to the new plug-in object
     */
    inline PluginType *load(const std::string& plugin_id, const std::string& plugin_type);
    
    /**
     * gets the plug-in object associated with a particular plugin_id (exact match)
     *
     * @param plugin_id unique identifier associated with the plug-in
     * @return PluginType* pointer to the matching plug-in object or NULL if not found
     */
    inline PluginType *get(const std::string& plugin_id);
    
    /**
     * gets the plug-in object associated with a particular plugin_id (exact match)
     *
     * @param plugin_id unique identifier associated with the plug-in
     * @return PluginType* pointer to the matching plug-in object or NULL if not found
     */
    inline const PluginType *get(const std::string& plugin_id) const;
    
    /**
     * gets a smart pointer to the plugin shared library for a particular plugin_id (exact match)
     *
     * @param plugin_id unique identifier associated with the plug-in
     * @return plugin_ptr<PluginType> pointer to the plugin shared library if found
     */
    inline plugin_ptr<PluginType> get_lib_ptr(const std::string& plugin_id) const;
    
    /**
     * finds the plug-in object associated with a particular resource (fuzzy match)
     *
     * @param resource resource identifier (uri-stem) to search for
     * @return PluginType* pointer to the matching plug-in object or NULL if not found
     */
    inline PluginType *find(const std::string& resource);
    
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
    inline std::uint64_t get_statistic(PluginStatFunction stat_func) const;
    
    /**
     * returns a statistic value for a particular plug-in
     *
     * @param plugin_id unique identifier associated with the plug-in
     * @param stat_func the statistic function to execute
     */
    inline std::uint64_t get_statistic(const std::string& plugin_id,
                                        PluginStatFunction stat_func) const;
        
    
protected:
    
    /// data type that maps identifiers to plug-in objects
    class map_type
        : public std::map<std::string, std::pair<PluginType *, plugin_ptr<PluginType> > >
    {
    public:
        inline void clear(void);
        virtual ~map_type() { map_type::clear(); }
        map_type(void) {}
    };
    
    /// collection of plug-in objects being managed
    map_type                m_plugin_map;

    /// mutex to make class thread-safe
    mutable std::mutex    m_plugin_mutex;
};

    
// plugin_manager member functions

template <typename PluginType>
inline void plugin_manager<PluginType>::add(const std::string& plugin_id,
                                            PluginType *plugin_object_ptr)
{
    plugin_ptr<PluginType> plugin_ptr;
    std::unique_lock<std::mutex> plugins_lock(m_plugin_mutex);
    m_plugin_map.insert(std::make_pair(plugin_id,
                                       std::make_pair(plugin_object_ptr, plugin_ptr)));
}

template <typename PluginType>
inline void plugin_manager<PluginType>::remove(const std::string& plugin_id)
{
    std::unique_lock<std::mutex> plugins_lock(m_plugin_mutex);
    typename pion::plugin_manager<PluginType>::map_type::iterator i = m_plugin_map.find(plugin_id);
    if (i == m_plugin_map.end())
        BOOST_THROW_EXCEPTION( error::plugin_not_found() << error::errinfo_plugin_name(plugin_id) );
    if (i->second.second.is_open()) {
        i->second.second.destroy(i->second.first);
    } else {
        delete i->second.first;
    }
    m_plugin_map.erase(i);
}

template <typename PluginType>
inline void plugin_manager<PluginType>::replace(const std::string& plugin_id, PluginType *plugin_ptr)
{
    BOOST_ASSERT(plugin_ptr);
    std::unique_lock<std::mutex> plugins_lock(m_plugin_mutex);
    typename pion::plugin_manager<PluginType>::map_type::iterator i = m_plugin_map.find(plugin_id);
    if (i == m_plugin_map.end())
        BOOST_THROW_EXCEPTION( error::plugin_not_found() << error::errinfo_plugin_name(plugin_id) );
    if (i->second.second.is_open()) {
        i->second.second.destroy(i->second.first);
    } else {
        delete i->second.first;
    }
    i->second.first = plugin_ptr;
}

template <typename PluginType>
inline PluginType *plugin_manager<PluginType>::clone(const std::string& plugin_id)
{
    std::unique_lock<std::mutex> plugins_lock(m_plugin_mutex);
    typename pion::plugin_manager<PluginType>::map_type::iterator i = m_plugin_map.find(plugin_id);
    if (i == m_plugin_map.end())
        BOOST_THROW_EXCEPTION( error::plugin_not_found() << error::errinfo_plugin_name(plugin_id) );
    return i->second.second.create();
}

template <typename PluginType>
inline PluginType *plugin_manager<PluginType>::load(const std::string& plugin_id,
                                                     const std::string& plugin_type)
{
    // search for the plug-in file using the configured paths
    if (m_plugin_map.find(plugin_id) != m_plugin_map.end())
        BOOST_THROW_EXCEPTION( error::duplicate_plugin() << error::errinfo_plugin_name(plugin_id) );
    
    // open up the plug-in's shared object library
    plugin_ptr<PluginType> plugin_ptr;
    plugin_ptr.open(plugin_type);   // may throw
    
    // create a new object using the plug-in library
    PluginType *plugin_object_ptr(plugin_ptr.create());
    
    // add the new plug-in object to our map
    std::unique_lock<std::mutex> plugins_lock(m_plugin_mutex);
    m_plugin_map.insert(std::make_pair(plugin_id,
                                       std::make_pair(plugin_object_ptr, plugin_ptr)));

    return plugin_object_ptr;
}

template <typename PluginType>
inline PluginType *plugin_manager<PluginType>::get(const std::string& plugin_id)
{
    PluginType *plugin_object_ptr = NULL;
    std::unique_lock<std::mutex> plugins_lock(m_plugin_mutex);
    typename pion::plugin_manager<PluginType>::map_type::iterator i = m_plugin_map.find(plugin_id);
    if (i != m_plugin_map.end())
        plugin_object_ptr = i->second.first;
    return plugin_object_ptr;
}
    
template <typename PluginType>
inline const PluginType *plugin_manager<PluginType>::get(const std::string& plugin_id) const
{
    const PluginType *plugin_object_ptr = NULL;
    std::unique_lock<std::mutex> plugins_lock(m_plugin_mutex);
    typename pion::plugin_manager<PluginType>::map_type::const_iterator i = m_plugin_map.find(plugin_id);
    if (i != m_plugin_map.end())
        plugin_object_ptr = i->second.first;
    return plugin_object_ptr;
}
    
template <typename PluginType>
inline plugin_ptr<PluginType> plugin_manager<PluginType>::get_lib_ptr(const std::string& plugin_id) const
{
    plugin_ptr<PluginType> plugin_ptr;
    std::unique_lock<std::mutex> plugins_lock(m_plugin_mutex);
    typename pion::plugin_manager<PluginType>::map_type::const_iterator i = m_plugin_map.find(plugin_id);
    if (i != m_plugin_map.end())
        plugin_ptr = i->second.second;
    return plugin_ptr;
}       

template <typename PluginType>
inline PluginType *plugin_manager<PluginType>::find(const std::string& resource)
{
    // will point to the matching plug-in object, if found
    PluginType *plugin_object_ptr = NULL;
    
    // lock mutex for thread safety (this should probably use ref counters)
    std::unique_lock<std::mutex> plugins_lock(m_plugin_mutex);
    
    // check if no plug-ins are being managed
    if (m_plugin_map.empty()) return plugin_object_ptr;
    
    // iterate through each plug-in whose identifier may match the resource
    typename pion::plugin_manager<PluginType>::map_type::iterator i = m_plugin_map.upper_bound(resource);
    while (i != m_plugin_map.begin()) {
        --i;
        
        // keep checking while the first part of the strings match
        if (resource.compare(0, i->first.size(), i->first) != 0) {
            // the first part no longer matches
            if (i != m_plugin_map.begin()) {
                // continue to next plug-in in list if its size is < this one
                typename pion::plugin_manager<PluginType>::map_type::iterator j=i;
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
    
template <typename PluginType>
inline void plugin_manager<PluginType>::run(PluginRunFunction run_func)
{
    std::unique_lock<std::mutex> plugins_lock(m_plugin_mutex);
    for (typename pion::plugin_manager<PluginType>::map_type::iterator i = m_plugin_map.begin();
         i != m_plugin_map.end(); ++i)
    {
        run_func(i->second.first);
    }
}

template <typename PluginType>
inline void plugin_manager<PluginType>::run(const std::string& plugin_id,
                                            PluginRunFunction run_func)
{
    // no need to lock (handled by plugin_manager::get())
    PluginType *plugin_object_ptr = get(plugin_id);
    if (plugin_object_ptr == NULL)
        BOOST_THROW_EXCEPTION( error::plugin_not_found() << error::errinfo_plugin_name(plugin_id) );
    run_func(plugin_object_ptr);
}

template <typename PluginType>
inline std::uint64_t plugin_manager<PluginType>::get_statistic(PluginStatFunction stat_func) const
{
    std::uint64_t stat_value = 0;
    std::unique_lock<std::mutex> plugins_lock(m_plugin_mutex);
    for (typename pion::plugin_manager<PluginType>::map_type::const_iterator i = m_plugin_map.begin();
         i != m_plugin_map.end(); ++i)
    {
        stat_value += stat_func(i->second.first);
    }
    return stat_value;
}

template <typename PluginType>
inline std::uint64_t plugin_manager<PluginType>::get_statistic(const std::string& plugin_id,
                                                                PluginStatFunction stat_func) const
{
    // no need to lock (handled by plugin_manager::get())
    const PluginType *plugin_object_ptr = const_cast<plugin_manager<PluginType>*>(this)->get(plugin_id);
    if (plugin_object_ptr == NULL)
        BOOST_THROW_EXCEPTION( error::plugin_not_found() << error::errinfo_plugin_name(plugin_id) );
    return stat_func(plugin_object_ptr);
}


// plugin_manager::map_type member functions

template <typename PluginType>
inline void plugin_manager<PluginType>::map_type::clear(void)
{
    if (! std::map<std::string, std::pair<PluginType *, plugin_ptr<PluginType> > >::empty()) {
        for (typename pion::plugin_manager<PluginType>::map_type::iterator i =
             std::map<std::string, std::pair<PluginType *, plugin_ptr<PluginType> > >::begin();
             i != std::map<std::string, std::pair<PluginType *, plugin_ptr<PluginType> > >::end(); ++i)
        {
            if (i->second.second.is_open()) {
                i->second.second.destroy(i->second.first);
            } else {
                delete i->second.first;
            }
        }
        this->erase(std::map<std::string, std::pair<PluginType *, plugin_ptr<PluginType> > >::begin(),
              std::map<std::string, std::pair<PluginType *, plugin_ptr<PluginType> > >::end());
    }
}


}   // end namespace pion

#endif
