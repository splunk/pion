// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_WEBSERVER_HEADER__
#define __PION_WEBSERVER_HEADER__

#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <pion/PionConfig.hpp>
#include <pion/PionException.hpp>
#include <pion/PionPlugin.hpp>
#include <pion/PluginManager.hpp>
#include <pion/net/HTTPServer.hpp>
#include <pion/net/WebService.hpp>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)

///
/// WebServer: a server that handles HTTP connections using WebService plug-ins
///
class PION_NET_API WebServer :
	public HTTPServer
{

public:

	/// exception thrown if a web service cannot be found
	class ServiceNotFoundException : public PionException {
	public:
		ServiceNotFoundException(const std::string& resource)
			: PionException("No web services are identified by the resource: ", resource) {}
	};

	/// exception thrown if the web service configuration file cannot be found
	class ConfigNotFoundException : public PionException {
	public:
		ConfigNotFoundException(const std::string& file)
			: PionException("Web service configuration file not found: ", file) {}
	};
	
	/// exception thrown if the plug-in file cannot be opened
	class ConfigParsingException : public PionException {
	public:
		ConfigParsingException(const std::string& file)
			: PionException("Unable to parse configuration file: ", file) {}
	};

	/// exception used to propagate exceptions thrown by web services
	class WebServiceException : public PionException {
	public:
		WebServiceException(const std::string& resource, const std::string& file)
			: PionException(std::string("WebService (") + resource,
							std::string("): ") + file)
		{}
	};
		
	
	/// default destructor
	virtual ~WebServer() { clear(); }
	
	/**
	 * creates a new WebServer object
	 * 
	 * @param tcp_port port number used to listen for new connections (IPv4)
	 */
	explicit WebServer(const unsigned int tcp_port = 0)
		: HTTPServer(tcp_port)
	{ 
		setLogger(PION_GET_LOGGER("pion.net.WebServer"));
	}
	
	/**
	 * creates a new WebServer object
	 * 
	 * @param endpoint TCP endpoint used to listen for new connections (see ASIO docs)
	 */
	explicit WebServer(const boost::asio::ip::tcp::endpoint& endpoint)
		: HTTPServer(endpoint)
	{ 
		setLogger(PION_GET_LOGGER("pion.net.WebServer"));
	}

	/**
	 * creates a new WebServer object
	 * 
	 * @param scheduler the PionScheduler that will be used to manage worker threads
	 * @param tcp_port port number used to listen for new connections (IPv4)
	 */
	explicit WebServer(PionScheduler& scheduler, const unsigned int tcp_port = 0)
		: HTTPServer(scheduler, tcp_port)
	{ 
		setLogger(PION_GET_LOGGER("pion.net.WebServer"));
	}
	
	/**
	 * creates a new WebServer object
	 * 
	 * @param scheduler the PionScheduler that will be used to manage worker threads
	 * @param endpoint TCP endpoint used to listen for new connections (see ASIO docs)
	 */
	WebServer(PionScheduler& scheduler, const boost::asio::ip::tcp::endpoint& endpoint)
		: HTTPServer(scheduler, endpoint)
	{ 
		setLogger(PION_GET_LOGGER("pion.net.WebServer"));
	}

	/**
	 * adds a new web service to the web server
	 *
	 * @param resource the resource name or uri-stem to bind to the web service
	 * @param service_ptr a pointer to the web service
	 */
	void addService(const std::string& resource, WebService *service_ptr);
	
	/**
	 * loads a web service from a shared object file
	 *
	 * @param resource the resource name or uri-stem to bind to the web service
	 * @param service_name the name of the web service to load (searches plug-in
	 *        directories and appends extensions)
	 */
	void loadService(const std::string& resource, const std::string& service_name);
	
	/**
	 * sets a configuration option for the web service associated with resource
	 *
	 * @param resource the resource name or uri-stem that identifies the web service
	 * @param name the name of the configuration option
	 * @param value the value to set the option to
	 */
	void setServiceOption(const std::string& resource,
						  const std::string& name, const std::string& value);
	
	/**
	 * Parses a simple web service configuration file. Each line in the file
	 * starts with one of the following commands:
	 *
	 * path VALUE  :  adds a directory to the web service search path
	 * service RESOURCE FILE  :  loads web service bound to RESOURCE from FILE
	 * option RESOURCE NAME=VALUE  :  sets web service option NAME to VALUE
	 *
	 * Blank lines or lines that begin with # are ignored as comments.
	 *
	 * @param config_name the name of the config file to parse
	 */
	void loadServiceConfig(const std::string& config_name);

	/// clears all the web services that are currently configured
	virtual void clear(void) {
		if (isListening()) stop();
		m_services.clear();
		HTTPServer::clear();
	}

	
protected:
	
	/// called before the TCP server starts listening for new connections
	virtual void beforeStarting(void) {
		// call the start() method for each web service associated with this server
		try { m_services.run(boost::bind(&WebService::start, _1)); }
		catch (std::exception& e) {
			// catch exceptions thrown by services since their exceptions may be free'd
			// from memory before they are caught
			throw WebServiceException("[Startup]", e.what());
		}
	}
	
	/// called after the TCP server has stopped listening for new connections
	virtual void afterStopping(void) {
		// call the stop() method for each web service associated with this server
		try { m_services.run(boost::bind(&WebService::stop, _1)); }
		catch (std::exception& e) {
			// catch exceptions thrown by services since their exceptions may be free'd
			// from memory before they are caught
			throw WebServiceException("[Shutdown]", e.what());
		}
	}

	
private:
	
	/// data type for a collection of web services
	typedef PluginManager<WebService>	WebServiceManager;
	
	
	/// Web services associated with this server
	WebServiceManager		m_services;
};


/// data type for a web server pointer
typedef boost::shared_ptr<WebServer>		WebServerPtr;


}	// end namespace net
}	// end namespace pion

#endif
