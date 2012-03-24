// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_WEBSERVICE_HEADER__
#define __PION_WEBSERVICE_HEADER__

#include <boost/noncopyable.hpp>
#include <pion/PionConfig.hpp>
#include <pion/PionException.hpp>
#include <pion/PionAlgorithms.hpp>
#include <pion/net/HTTPRequest.hpp>
#include <pion/net/TCPConnection.hpp>
#include <string>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)

///
/// WebService: interface class for web services
/// 
class WebService :
	private boost::noncopyable
{
public:

	/// exception thrown if the service does not recognize a configuration option
	class UnknownOptionException : public PionException {
	public:
		UnknownOptionException(const std::string& name)
			: PionException("Option not recognized by web service: ", name) {}
	};

	/// default constructor
	WebService(void) {}

	/// virtual destructor
	virtual ~WebService() {}

	/**
	 * attempts to handle a new HTTP request
	 *
	 * @param request the new HTTP request to handle
	 * @param tcp_conn the TCP connection that has the new request
	 */
	virtual void operator()(HTTPRequestPtr& request, TCPConnectionPtr& tcp_conn) = 0;
	
	/**
	 * sets a configuration option
	 *
	 * @param name the name of the option to change
	 * @param value the value of the option
	 */
	virtual void setOption(const std::string& name, const std::string& value) {
		throw UnknownOptionException(name);
	}
	
	/// called when the web service's server is starting
	virtual void start(void) {}
	
	/// called when the web service's server is stopping
	virtual void stop(void) {}
	
	/// sets the URI stem or resource that is bound to the web service
	inline void setResource(const std::string& str) { m_resource = str; }

	/// returns the URI stem or resource that is bound to the web service	
	inline const std::string& getResource(void) const { return m_resource; }
	
	/// returns the path to the resource requested, relative to the web service's location
	inline std::string getRelativeResource(const std::string& resource_requested) const {
		if (resource_requested.size() <= getResource().size()) {
			// either the request matches the web service's resource path (a directory)
			// or the request does not match (should never happen)
			return std::string();
		}
		// strip the web service's resource path plus the slash after it
		return algo::url_decode(resource_requested.substr(getResource().size() + 1));
	}
	
	
private:
		
	/// the URI stem or resource that is bound to the web service	
	std::string	m_resource;
};


//
// The following symbols must be defined for any web service that you would
// like to be able to load dynamically using the HTTPServer::loadService()
// function.  These are not required for any services that you only want to link
// directly into your programs.
//
// Make sure that you replace "WebService" with the name of your derived class.
// This name must also match the name of the object file (excluding the
// extension).  These symbols must be linked into your service's object file,
// not included in any headers that it may use (declarations are OK in headers
// but not the definitions).
//
// The "pion_create" function is used to create new instances of your service.
// The "pion_destroy" function is used to destroy instances of your service.
//
// extern "C" WebService *pion_create_WebService(void) {
//		return new WebService;
// }
//
// extern "C" void pion_destroy_WebService(WebService *service_ptr) {
//		delete service_ptr;
// }
//

}	// end namespace net
}	// end namespace pion

#endif
