// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/algorithm/string.hpp>
#include <pion/http/auth.hpp>
#include <pion/http/server.hpp>


namespace pion {    // begin namespace pion
namespace net {     // begin namespace net (Pion Network Library)


// HTTPAuth member functions

void HTTPAuth::addRestrict(const std::string& resource)
{
    boost::mutex::scoped_lock resource_lock(m_resource_mutex);
    const std::string clean_resource(HTTPServer::stripTrailingSlash(resource));
    m_restrict_list.insert(clean_resource);
    PION_LOG_INFO(m_logger, "Set authentication restrictions for HTTP resource: " << clean_resource);
}

void HTTPAuth::addPermit(const std::string& resource)
{
    boost::mutex::scoped_lock resource_lock(m_resource_mutex);
    const std::string clean_resource(HTTPServer::stripTrailingSlash(resource));
    m_white_list.insert(clean_resource);
    PION_LOG_INFO(m_logger, "Set authentication permission for HTTP resource: " << clean_resource);
}

bool HTTPAuth::needAuthentication(const HTTPRequestPtr& http_request) const
{
    // if no users are defined, authentication is never required
    if (m_user_manager->empty())
        return false;
    
    // strip off trailing slash if the request has one
    std::string resource(HTTPServer::stripTrailingSlash(http_request->getResource()));
    
    boost::mutex::scoped_lock resource_lock(m_resource_mutex);
    
    // just return false if restricted list is empty
    if (m_restrict_list.empty())
        return false;

    // try to find resource in restricted list
    if (findResource(m_restrict_list, resource)) {
        // return true if white list is empty
        if (m_white_list.empty())
            return true;
        // return false if found in white list, or true if not found
        return ( ! findResource(m_white_list, resource) );
    }
    
    // resource not found in restricted list
    return false;
}

bool HTTPAuth::findResource(const AuthResourceSet& resource_set,
                            const std::string& resource) const
{
    AuthResourceSet::const_iterator i = resource_set.upper_bound(resource);
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

  
}   // end namespace net
}   // end namespace pion
