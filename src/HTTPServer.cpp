// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/net/HTTPServer.hpp>
#include <pion/net/HTTPRequest.hpp>
#include <pion/net/HTTPRequestReader.hpp>
#include <pion/net/HTTPResponseWriter.hpp>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)

	
// HTTPServer member functions

void HTTPServer::handleConnection(TCPConnectionPtr& tcp_conn)
{
	HTTPRequestReader::create(tcp_conn, boost::bind(&HTTPServer::handleRequest,
													 this, _1, _2)) -> receive();
}

void HTTPServer::handleRequest(HTTPRequestPtr& http_request,
							   TCPConnectionPtr& tcp_conn)
{
	if (! http_request->isValid()) {
		// the request is invalid or an error occured
		PION_LOG_INFO(m_logger, "Received an invalid HTTP request");
		m_bad_request_handler(http_request, tcp_conn);
		return;
	}
		
	PION_LOG_DEBUG(m_logger, "Received a valid HTTP request");

    // strip off trailing slash if the request has one
    std::string resource_requested(stripTrailingSlash(http_request->getResource()));

	// if authentication activated, check current request
	if (m_auth) {
		// try to verify authentication
		if (! m_auth->handleRequest(http_request, tcp_conn)) {
			// the HTTP 401 message has already been sent by the authentication object
			PION_LOG_DEBUG(m_logger, "Authentication required for HTTP resource: "
				<< resource_requested);
			return;
		}
	}
	
	// search for a handler matching the resource requested
	RequestHandler request_handler;
	if (findRequestHandler(resource_requested, request_handler)) {
		
		// try to handle the request
		try {
			request_handler(http_request, tcp_conn);
			PION_LOG_DEBUG(m_logger, "Found request handler for HTTP resource: "
						   << resource_requested);
		} catch (HTTPResponseWriter::LostConnectionException& e) {
			// the connection was lost while or before sending the response
			PION_LOG_WARN(m_logger, "HTTP request handler: " << e.what());
			tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_CLOSE);	// make sure it will get closed
			tcp_conn->finish();
		} catch (std::bad_alloc&) {
			// propagate memory errors (FATAL)
			throw;
		} catch (std::exception& e) {
			// recover gracefully from other exceptions thrown request handlers
			PION_LOG_ERROR(m_logger, "HTTP request handler: " << e.what());
			m_server_error_handler(http_request, tcp_conn, e.what());
		}
		
	} else {
		
		// no web services found that could handle the request
		PION_LOG_INFO(m_logger, "No HTTP request handlers found for resource: "
					  << resource_requested);
		m_not_found_handler(http_request, tcp_conn);
	}
}
	
bool HTTPServer::findRequestHandler(const std::string& resource,
									RequestHandler& request_handler) const
{
	// first make sure that HTTP resources are registered
	boost::mutex::scoped_lock resource_lock(m_resource_mutex);
	if (m_resources.empty())
		return false;
	
	// iterate through each resource entry that may match the resource
	ResourceMap::const_iterator i = m_resources.upper_bound(resource);
	while (i != m_resources.begin()) {
		--i;
		// check for a match if the first part of the strings match
		if (i->first.empty() || resource.compare(0, i->first.size(), i->first) == 0) {
			// only if the resource matches the plug-in's identifier
			// or if resource is followed first with a '/' character
			if (resource.size() == i->first.size() || resource[i->first.size()]=='/') {
				request_handler = i->second;
				return true;
			}
		}
	}
	
	return false;
}

void HTTPServer::addResource(const std::string& resource,
							 RequestHandler request_handler)
{
	boost::mutex::scoped_lock resource_lock(m_resource_mutex);
	const std::string clean_resource(stripTrailingSlash(resource));
	m_resources.insert(std::make_pair(clean_resource, request_handler));
	PION_LOG_INFO(m_logger, "Added request handler for HTTP resource: " << clean_resource);
}

void HTTPServer::handleBadRequest(HTTPRequestPtr& http_request,
								  TCPConnectionPtr& tcp_conn)
{
	static const std::string BAD_REQUEST_HTML =
		"<html><head>\n"
		"<title>400 Bad Request</title>\n"
		"</head><body>\n"
		"<h1>Bad Request</h1>\n"
		"<p>Your browser sent a request that this server could not understand.</p>\n"
		"</body></html>\n";
	HTTPResponseWriterPtr writer(HTTPResponseWriter::create(tcp_conn, *http_request,
															boost::bind(&TCPConnection::finish, tcp_conn)));
	writer->getResponse().setStatusCode(HTTPTypes::RESPONSE_CODE_BAD_REQUEST);
	writer->getResponse().setStatusMessage(HTTPTypes::RESPONSE_MESSAGE_BAD_REQUEST);
	writer->writeNoCopy(BAD_REQUEST_HTML);
	writer->send();
}

void HTTPServer::handleNotFoundRequest(HTTPRequestPtr& http_request,
									   TCPConnectionPtr& tcp_conn)
{
	static const std::string NOT_FOUND_HTML_START =
		"<html><head>\n"
		"<title>404 Not Found</title>\n"
		"</head><body>\n"
		"<h1>Not Found</h1>\n"
		"<p>The requested URL ";
	static const std::string NOT_FOUND_HTML_FINISH =
		" was not found on this server.</p>\n"
		"</body></html>\n";
	HTTPResponseWriterPtr writer(HTTPResponseWriter::create(tcp_conn, *http_request,
															boost::bind(&TCPConnection::finish, tcp_conn)));
	writer->getResponse().setStatusCode(HTTPTypes::RESPONSE_CODE_NOT_FOUND);
	writer->getResponse().setStatusMessage(HTTPTypes::RESPONSE_MESSAGE_NOT_FOUND);
	writer->writeNoCopy(NOT_FOUND_HTML_START);
	writer << http_request->getResource();
	writer->writeNoCopy(NOT_FOUND_HTML_FINISH);
	writer->send();
}

void HTTPServer::handleServerError(HTTPRequestPtr& http_request,
								   TCPConnectionPtr& tcp_conn,
								   const std::string& error_msg)
{
	static const std::string SERVER_ERROR_HTML_START =
		"<html><head>\n"
		"<title>500 Server Error</title>\n"
		"</head><body>\n"
		"<h1>Internal Server Error</h1>\n"
		"<p>The server encountered an internal error: <strong>";
	static const std::string SERVER_ERROR_HTML_FINISH =
		"</strong></p>\n"
		"</body></html>\n";
	HTTPResponseWriterPtr writer(HTTPResponseWriter::create(tcp_conn, *http_request,
															boost::bind(&TCPConnection::finish, tcp_conn)));
	writer->getResponse().setStatusCode(HTTPTypes::RESPONSE_CODE_SERVER_ERROR);
	writer->getResponse().setStatusMessage(HTTPTypes::RESPONSE_MESSAGE_SERVER_ERROR);
	writer->writeNoCopy(SERVER_ERROR_HTML_START);
	writer << error_msg;
	writer->writeNoCopy(SERVER_ERROR_HTML_FINISH);
	writer->send();
}

}	// end namespace net
}	// end namespace pion

