// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/algorithm/string.hpp>
#include <pion/http/auth.hpp>
#include <pion/http/server.hpp>


namespace pion {    // begin namespace pion
namespace http {    // begin namespace http


// auth member functions

void auth::add_restrict(const std::string& resource)
{
    std::unique_lock<std::mutex> resource_lock(m_resource_mutex);
    const std::string clean_resource(http::server::strip_trailing_slash(resource));
    m_restrict_list.insert(clean_resource);
    PION_LOG_INFO(m_logger, "Set authentication restrictions for HTTP resource: " << clean_resource);
}

void auth::add_permit(const std::string& resource)
{
    std::unique_lock<std::mutex> resource_lock(m_resource_mutex);
    const std::string clean_resource(http::server::strip_trailing_slash(resource));
    m_white_list.insert(clean_resource);
    PION_LOG_INFO(m_logger, "Set authentication permission for HTTP resource: " << clean_resource);
}

bool auth::need_authentication(const http::request_ptr& http_request_ptr) const
{
    // if no users are defined, authentication is never required
    if (m_user_manager->empty())
        return false;
    
    // strip off trailing slash if the request has one
    std::string resource(http::server::strip_trailing_slash(http_request_ptr->get_resource()));
    
    std::unique_lock<std::mutex> resource_lock(m_resource_mutex);
    
    // just return false if restricted list is empty
    if (m_restrict_list.empty())
        return false;

    // try to find resource in restricted list
    if (find_resource(m_restrict_list, resource)) {
        // return true if white list is empty
        if (m_white_list.empty())
            return true;
        // return false if found in white list, or true if not found
        return ( ! find_resource(m_white_list, resource) );
    }
    
    // resource not found in restricted list
    return false;
}

bool auth::find_resource(const resource_set_type& resource_set,
                            const std::string& resource) const
{
    resource_set_type::const_iterator i = resource_set.upper_bound(resource);
    while (i != resource_set.begin()) {
        --i;
        // check for a match if the first part of the strings match
        if (i->empty() || resource.compare(0, i->size(), *i) == 0) {
            // only if the resource matches exactly 
            // or if resource is followed first with a '/' character
            if (resource.size() == i->size() || resource[i->size()]=='/') {
                return true;
            }
        }
    }
    return false;
}

  
}   // end namespace http
}   // end namespace pion
