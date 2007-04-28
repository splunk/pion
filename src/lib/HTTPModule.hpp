// -----------------------------------------------------------------
// libpion: a C++ framework for building lightweight HTTP interfaces
// -----------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.
// 
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//

#ifndef __PION_HTTPMODULE_HEADER__
#define __PION_HTTPMODULE_HEADER__

#include "HTTPRequest.hpp"
#include "TCPConnection.hpp"
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <string>


namespace pion {	// begin namespace pion

///
/// HTTPModule: interface class for HTTP modules
/// 
class HTTPModule : private boost::noncopyable {
public:

	/// virtual destructor
	virtual ~HTTPModule() {}
	
	/**
	 * constructs a new module
	 *
	 * @param resource the resource or URI stem associated with this module
	 */
	explicit HTTPModule(const std::string& resource) : m_resource(resource) {}
	
	/**
     * attempts to handle a new HTTP request
	 *
     * @param request the new HTTP request to handle
     * @param tcp_conn the TCP connection that has the new request
	 * @param keepalive_handler called after the response has finished sending
	 *
	 * @return true if the request was handled; false if not
	 */
	virtual bool handleRequest(HTTPRequestPtr& request, TCPConnectionPtr& tcp_conn,
							   TCPConnection::ConnectionHandler& keepalive_handler)
	{ 
		return false;
	}

	/// returns true if the module may be able to handle the resource
	inline bool checkResource(const std::string& r) const {
		return(r.compare(0, m_resource.size(), m_resource) == 0);
	}

	/// returns the resource associated with this module
	inline const std::string& getResource(void) const { return m_resource; }
 	
	
private:

	/// HTTP resource name or URI stem
	const std::string		m_resource;
};


/// data type for a tcp protocol handler pointer
typedef boost::shared_ptr<HTTPModule>		HTTPModulePtr;


}	// end namespace pion

#endif
