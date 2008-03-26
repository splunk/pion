// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/algorithm/string.hpp>
#include <pion/net/HTTPAuth.hpp>
#include <pion/net/HTTPServer.hpp>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)


// HTTPAuth member functions

void HTTPAuth::addResource(const std::string& resource)
{
	boost::mutex::scoped_lock resource_lock(m_resource_mutex);
	const std::string clean_resource(HTTPServer::stripTrailingSlash(resource));
	m_auth_resources.insert(clean_resource);
	PION_LOG_INFO(m_logger, "Set authentication for HTTP resource: " << clean_resource);
}

bool HTTPAuth::needAuthentication(const HTTPRequestPtr& http_request) const
{
	// strip off trailing slash if the request has one
	std::string resource(HTTPServer::stripTrailingSlash(http_request->getResource()));
	
	// first make sure that HTTP resources are registered
	boost::mutex::scoped_lock resource_lock(m_resource_mutex);
	if (m_auth_resources.empty())
		return false;
	
	// iterate through each auth resource entry that may match the input resource
	AuthResourceSet::const_iterator i = m_auth_resources.upper_bound(resource);
	while (i != m_auth_resources.begin()) {
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

  
}	// end namespace net
}	// end namespace pion
