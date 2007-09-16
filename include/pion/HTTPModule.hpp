// -----------------------------------------------------------------
// libpion: a C++ framework for building lightweight HTTP interfaces
// -----------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTPMODULE_HEADER__
#define __PION_HTTPMODULE_HEADER__

#include <boost/noncopyable.hpp>
#include <libpion/PionConfig.hpp>
#include <libpion/PionException.hpp>
#include <libpion/HTTPRequest.hpp>
#include <libpion/TCPConnection.hpp>
#include <string>


namespace pion {	// begin namespace pion

///
/// HTTPModule: interface class for HTTP modules
/// 
class HTTPModule :
	private boost::noncopyable
{
public:

	/// exception thrown if the module does not recognize a configuration option
	class UnknownOptionException : public PionException {
	public:
		UnknownOptionException(const std::string& name)
			: PionException("Option not recognized by HTTP module: ", name) {}
	};

	/// default constructor
	HTTPModule(void) {}

	/// virtual destructor
	virtual ~HTTPModule() {}

	/**
     * attempts to handle a new HTTP request
	 *
     * @param request the new HTTP request to handle
     * @param tcp_conn the TCP connection that has the new request
	 *
	 * @return true if the request was handled; false if not
	 */
	virtual bool handleRequest(HTTPRequestPtr& request, TCPConnectionPtr& tcp_conn) = 0;
	
	/**
	 * sets a configuration option
	 *
	 * @param name the name of the option to change
	 * @param value the value of the option
	 */
	virtual void setOption(const std::string& name, const std::string& value) {
		throw UnknownOptionException(name);
	}
	
	/// called when the module's server is starting
	virtual void start(void) {}
	
	/// called when the module's server is stopping
	virtual void stop(void) {}
	
	/// sets the URI stem or resource that is bound to the module (strips any trailing slash)	
	inline void setResource(const std::string& str) { m_resource = stripTrailingSlash(str); }

	/// returns the URI stem or resource that is bound to the module	
	inline const std::string& getResource(void) const { return m_resource; }
	
	/// returns the path to the resource requested, relative to the module's location
	inline std::string getRelativeResource(const std::string& resource_requested) const {
		if (resource_requested.size() <= getResource().size()) {
			// either the request matches the module's resource path (a directory)
			// or the request does not match (should never happen)
			return std::string();
		}
		// strip the module's resource path plus the slash after it
		return HTTPTypes::url_decode(resource_requested.substr(getResource().size() + 1));
	}
	
	
private:
		
	/// strips trailing slash from string, if one exists
	static inline std::string stripTrailingSlash(const std::string& str) {
		std::string result(str);
		if (!result.empty() && result[result.size()-1]=='/')
			result.resize(result.size() - 1);
		return result;
	}
		
	/// the URI stem or resource that is bound to the module	
	std::string	m_resource;
};


//
// The following symbols must be defined for any HTTP modules that you would
// like to be able to load dynamically using the HTTPServer::loadModule()
// function.  These are not required for any modules that you only want to link
// directly into your programs.
//
// Make sure that you replace "HTTPModule" with the name of your derived class.
// This name must also match the name of the object file (excluding the
// extension).  These symbols must be linked into your module's object file,
// not included in any headers that it may use (declarations are OK in headers
// but not the definitions).
//
// The "pion_create" function is used to create new instances of your module.
// The "pion_destroy" function is used to destroy instances of your module.
//
// extern "C" HTTPModule *pion_create_HTTPModule(void) {
//		return new HTTPModule;
// }
//
// extern "C" void pion_destroy_HTTPModule(HTTPModule *module_ptr) {
//		delete module_ptr;
// }
//

}	// end namespace pion

#endif
