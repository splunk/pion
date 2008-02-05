// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTPSERVER_HEADER__
#define __PION_HTTPSERVER_HEADER__

#include <map>
#include <string>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <pion/PionConfig.hpp>
#include <pion/net/TCPServer.hpp>
#include <pion/net/TCPConnection.hpp>
#include <pion/net/HTTPRequest.hpp>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)

///
/// HTTPServer: a server that handles HTTP connections
///
class PION_NET_API HTTPServer :
	public TCPServer
{

public:

	/// type of function that is used to handle requests
	typedef boost::function2<void, HTTPRequestPtr&, TCPConnectionPtr&>	RequestHandler;

	/// handler for requests that result in "500 Server Error"
	typedef boost::function3<void, HTTPRequestPtr&, TCPConnectionPtr&,
		const std::string&>	ServerErrorHandler;
	
	
	/// default destructor
	virtual ~HTTPServer() { if (isListening()) stop(); }
	
	/**
	 * creates a new HTTPServer object
	 * 
	 * @param tcp_port port number used to listen for new connections (IPv4)
	 */
	explicit HTTPServer(const unsigned int tcp_port = 0)
		: TCPServer(tcp_port),
		m_bad_request_handler(HTTPServer::handleBadRequest),
		m_not_found_handler(HTTPServer::handleNotFoundRequest),
		m_server_error_handler(HTTPServer::handleServerError)
	{ 
		setLogger(PION_GET_LOGGER("pion.net.HTTPServer"));
	}
	
	/**
	 * creates a new HTTPServer object
	 * 
	 * @param endpoint TCP endpoint used to listen for new connections (see ASIO docs)
	 */
	explicit HTTPServer(const boost::asio::ip::tcp::endpoint& endpoint)
		: TCPServer(endpoint),
		m_bad_request_handler(HTTPServer::handleBadRequest),
		m_not_found_handler(HTTPServer::handleNotFoundRequest),
		m_server_error_handler(HTTPServer::handleServerError)
	{ 
		setLogger(PION_GET_LOGGER("pion.net.HTTPServer"));
	}
	
	/**
	 * creates a new HTTPServer object
	 * 
	 * @param scheduler the PionScheduler that will be used to manage worker threads
	 * @param tcp_port port number used to listen for new connections (IPv4)
	 */
	explicit HTTPServer(PionScheduler& scheduler, const unsigned int tcp_port = 0)
		: TCPServer(scheduler, tcp_port),
		m_bad_request_handler(HTTPServer::handleBadRequest),
		m_not_found_handler(HTTPServer::handleNotFoundRequest),
		m_server_error_handler(HTTPServer::handleServerError)
	{ 
		setLogger(PION_GET_LOGGER("pion.net.HTTPServer"));
	}
	
	/**
	 * creates a new HTTPServer object
	 * 
	 * @param scheduler the PionScheduler that will be used to manage worker threads
	 * @param endpoint TCP endpoint used to listen for new connections (see ASIO docs)
	 */
	HTTPServer(PionScheduler& scheduler, const boost::asio::ip::tcp::endpoint& endpoint)
		: TCPServer(scheduler, endpoint),
		m_bad_request_handler(HTTPServer::handleBadRequest),
		m_not_found_handler(HTTPServer::handleNotFoundRequest),
		m_server_error_handler(HTTPServer::handleServerError)
	{ 
		setLogger(PION_GET_LOGGER("pion.net.HTTPServer"));
	}
	
	/**
	 * adds a new web service to the HTTP server
	 *
	 * @param resource the resource name or uri-stem to bind to the handler
	 * @param request_handler function used to handle requests to the resource
	 */
	void addResource(const std::string& resource, RequestHandler request_handler);

	/// sets the function that handles bad HTTP requests
	inline void setBadRequestHandler(RequestHandler h) { m_bad_request_handler = h; }
	
	/// sets the function that handles requests which match no other web services
	inline void setNotFoundHandler(RequestHandler h) { m_not_found_handler = h; }
	
	/// sets the function that handles requests which match no other web services
	inline void setServerErrorHandler(ServerErrorHandler h) { m_server_error_handler = h; }

	/// clears the collection of resources recognized by the HTTP server
	virtual void clear(void) {
		if (isListening()) stop();
		boost::mutex::scoped_lock resource_lock(m_resource_mutex);
		m_resources.clear();
	}
	
	/**
	 * strips trailing slash from a string, if one exists
	 *
	 * @param str the original string
	 * @return the resulting string, after any trailing slash is removed
	 */
	static inline std::string stripTrailingSlash(const std::string& str) {
		std::string result(str);
		if (!result.empty() && result[result.size()-1]=='/')
			result.resize(result.size() - 1);
		return result;
	}
	
	/**
	 * used to send responses when a bad HTTP request is made
	 *
     * @param http_request the new HTTP request to handle
     * @param tcp_conn the TCP connection that has the new request
	 */
	static void handleBadRequest(HTTPRequestPtr& http_request,
								 TCPConnectionPtr& tcp_conn);
	
	/**
	 * used to send responses when no web services can handle the request
	 *
     * @param http_request the new HTTP request to handle
     * @param tcp_conn the TCP connection that has the new request
	 */
	static void handleNotFoundRequest(HTTPRequestPtr& http_request,
									  TCPConnectionPtr& tcp_conn);
	
	/**
	 * used to send responses when a server error occurs
	 *
     * @param http_request the new HTTP request to handle
     * @param tcp_conn the TCP connection that has the new request
	 * @param error_msg message that explains what went wrong
	 */
	static void handleServerError(HTTPRequestPtr& http_request,
								  TCPConnectionPtr& tcp_conn,
								  const std::string& error_msg);

	
protected:
	
	/**
	 * handles a new TCP connection
	 * 
	 * @param tcp_conn the new TCP connection to handle
	 */
	virtual void handleConnection(TCPConnectionPtr& tcp_conn);
	
	/**
	 * handles a new HTTP request
	 *
	 * @param http_request the HTTP request to handle
	 * @param tcp_conn TCP connection containing a new request
	 */
	void handleRequest(HTTPRequestPtr& http_request, TCPConnectionPtr& tcp_conn);
	
	/**
	 * searches for the appropriate request handler to use for a given resource
	 *
	 * @param resource the name of the resource to search for
	 * @param request_handler function that can handle requests for this resource
	 */
	bool findRequestHandler(const std::string& resource,
							RequestHandler& request_handler) const;
	
		
private:
	
	/// data type for a map of resources to request handlers
	typedef std::map<std::string, RequestHandler>	ResourceMap;
	
	
	/// collection of resources that are recognized by this HTTP server
	ResourceMap					m_resources;

	/// points to a function that handles bad HTTP requests
	RequestHandler				m_bad_request_handler;
	
	/// points to a function that handles requests which match no web services
	RequestHandler				m_not_found_handler;

	/// points to the function that handles server errors
	ServerErrorHandler			m_server_error_handler;

	/// mutex used to protect access to the resources
	mutable boost::mutex		m_resource_mutex;
};


/// data type for a HTTP protocol handler pointer
typedef boost::shared_ptr<HTTPServer>		HTTPServerPtr;


}	// end namespace net
}	// end namespace pion

#endif
