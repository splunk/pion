// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <pion/net/HTTPServer.hpp>
#include <pion/net/HTTPRequest.hpp>
#include <pion/net/HTTPResponse.hpp>
#include <pion/net/HTTPRequestParser.hpp>
#include <fstream>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)

// HTTPServer member functions

void HTTPServer::handleConnection(TCPConnectionPtr& tcp_conn)
{
	HTTPRequestParserPtr request_parser(HTTPRequestParser::create(boost::bind(&HTTPServer::handleRequest,
																			  this, _1, _2), tcp_conn));
	request_parser->readRequest();
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
	std::string resource(http_request->getResource());
	if (! resource.empty() && resource[resource.size() - 1] == '/')
		resource.resize( resource.size() - 1 );

	// true if a web service successfully handled the request
	bool request_was_handled = false;
	
	// lock mutex for thread safety (this should probably use ref counters)
	boost::mutex::scoped_lock services_lock(m_mutex);

	if (m_services.empty()) {
		
		// no services configured
		PION_LOG_WARN(m_logger, "No web services configured");
		
	} else {

		// iterate through each web service that may be able to handle the request
		WebServiceMap::iterator i = m_services.upper_bound(resource);
		while (i != m_services.begin()) {
			--i;
			
			// keep checking while the first part of the strings match
			if (resource.compare(0, i->first.size(), i->first) != 0) {
				// the first part no longer matches
				if (i != m_services.begin()) {
					// continue to next service in list if its size is < this one
					WebServiceMap::iterator j=i;
					--j;
					if (j->first.size() < i->first.size())
						continue;
				}
				// otherwise we've reached the end; stop looking for a match
				break;
			}
				
			// only try the service if the request matches the service name or
			// if service name is followed first with a '/' character
			if (resource.size() == i->first.size() || resource[i->first.size()]=='/') {
				
				// try to handle the request with the web service
				try {
					request_was_handled = i->second.first->handleRequest(http_request, tcp_conn);
				} catch (HTTPResponse::LostConnectionException& e) {
					// the connection was lost while or before sending the response
					PION_LOG_WARN(m_logger, "Web service (" << resource << "): " << e.what());
					tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_CLOSE);	// make sure it will get closed
					tcp_conn->finish();
					request_was_handled = true;
					break;
				} catch (std::bad_alloc&) {
					// propagate memory errors (FATAL)
					throw;
				} catch (std::exception& e) {
					// recover gracefully from other exceptions thrown by services
					PION_LOG_ERROR(m_logger, "Web service (" << resource << ") exception: " << e.what());
					m_server_error_handler(http_request, tcp_conn, e.what());
					request_was_handled = true;
					break;
				}

				if (request_was_handled) {
					// the web service successfully handled the request
					PION_LOG_DEBUG(m_logger, "HTTP request handled by web service ("
								   << i->first << "): "
								   << http_request->getResource());
					break;
				}
			}
		}
	}
	
	if (! request_was_handled) {
		// no web services found that could handle the request
		PION_LOG_INFO(m_logger, "No web services found to handle HTTP request: " << resource);
		m_not_found_handler(http_request, tcp_conn);
	}
}

void HTTPServer::beforeStarting(void)
{
	// call the start() method for each web service associated with this server
	boost::mutex::scoped_lock services_lock(m_mutex);
	for (WebServiceMap::iterator i = m_services.begin(); i != m_services.end(); ++i)
		i->second.first->start();
}

void HTTPServer::afterStopping(void)
{
	// call the stop() method for each web service associated with this server
	boost::mutex::scoped_lock services_lock(m_mutex);
	for (WebServiceMap::iterator i = m_services.begin(); i != m_services.end(); ++i)
		i->second.first->stop();
}

void HTTPServer::addService(const std::string& resource, WebService *service_ptr)
{
	PionPluginPtr<WebService> plugin_ptr;
	service_ptr->setResource(resource);	// strips any trailing '/' from the name
	boost::mutex::scoped_lock services_lock(m_mutex);
	m_services.insert(std::make_pair(service_ptr->getResource(),
									 std::make_pair(service_ptr, plugin_ptr)));
}

void HTTPServer::loadService(const std::string& resource, const std::string& service_name)
{
	// search for the plug-in file using the configured paths
	bool is_static;
	void *create_func;
	void *destroy_func;
	
	// check if service is statically linked, and if not, try to resolve for dynamic
	is_static = PionPlugin::findStaticEntryPoint(service_name, &create_func, &destroy_func);

	// open up the plug-in's shared object library
	PionPluginPtr<WebService> plugin_ptr;
	if (is_static) {
		plugin_ptr.openStaticLinked(service_name, create_func, destroy_func);	// may throw
	} else {
		plugin_ptr.open(service_name);	// may throw
	}

	// create a new web service using the plug-in library
	WebService *service_ptr(plugin_ptr.create());
	service_ptr->setResource(resource);	// strips any trailing '/' from the name

	// add the web service to the server's collection
	boost::mutex::scoped_lock services_lock(m_mutex);
	m_services.insert(std::make_pair(service_ptr->getResource(),
									 std::make_pair(service_ptr, plugin_ptr)));
	services_lock.unlock();

	if (is_static) {
		PION_LOG_INFO(m_logger, "Loaded static web service for resource (" << resource << "): " << service_name);
	} else {
		PION_LOG_INFO(m_logger, "Loaded web service plug-in for resource (" << resource << "): " << service_name);
	}
}

void HTTPServer::setServiceOption(const std::string& resource,
								  const std::string& name, const std::string& value)
{
	boost::mutex::scoped_lock services_lock(m_mutex);

	// find the web service associated with resource & set the option
	// if resource == "/" then look for web service with an empty string
	WebServiceMap::iterator i = (resource == "/" ? m_services.find("") : m_services.find(resource));
	if (i == m_services.end())
		throw ServiceNotFoundException(resource);
	i->second.first->setOption(name, value);

	services_lock.unlock();
	PION_LOG_INFO(m_logger, "Set web service option for resource ("
				  << resource << "): " << name << '=' << value);
}

void HTTPServer::loadServiceConfig(const std::string& config_name)
{
	std::string config_file;
	if (! PionPlugin::findConfigFile(config_file, config_name))
		throw ConfigNotFoundException(config_name);
	
	// open the file for reading
	std::ifstream config_stream;
	config_stream.open(config_file.c_str(), std::ios::in);
	if (! config_stream.is_open())
		throw ConfigParsingException(config_name);
	
	// parse the contents of the file
	enum ParseState {
		PARSE_NEWLINE, PARSE_COMMAND, PARSE_RESOURCE, PARSE_VALUE, PARSE_COMMENT
	} parse_state = PARSE_NEWLINE;
	std::string command_string;
	std::string resource_string;
	std::string value_string;
	std::string option_name_string;
	std::string option_value_string;
	int c = config_stream.get();	// read the first character
	
	while (config_stream) {
		switch(parse_state) {
		case PARSE_NEWLINE:
			// parsing command portion (or beginning of line)
			if (c == '#') {
				// line is a comment
				parse_state = PARSE_COMMENT;
			} else if (isalpha(c)) {
				// first char in command
				parse_state = PARSE_COMMAND;
				// ignore case for commands
				command_string += tolower(c);
			} else if (c != '\r' && c != '\n') {	// check for blank lines
				throw ConfigParsingException(config_name);
			}
			break;
			
		case PARSE_COMMAND:
			// parsing command portion (or beginning of line)
			if (c == ' ' || c == '\t') {
				// command finished -> check if valid
				if (command_string=="path") {
					value_string.clear();
					parse_state = PARSE_VALUE;
				} else if (command_string=="service" || command_string=="option") {
					resource_string.clear();
					parse_state = PARSE_RESOURCE;
				} else {
					throw ConfigParsingException(config_name);
				}
			} else if (! isalpha(c)) {
				// commands may only contain alpha chars
				throw ConfigParsingException(config_name);
			} else {
				// ignore case for commands
				command_string += tolower(c);
			}
			break;

		case PARSE_RESOURCE:
			// parsing resource portion (/hello)
			if (c == ' ' || c == '\t') {
				// check for leading whitespace
				if (! resource_string.empty()) {
					// resource finished
					value_string.clear();
					parse_state = PARSE_VALUE;
				}
			} else if (c == '\r' || c == '\n') {
				// line truncated before value
				throw ConfigParsingException(config_name);
			} else {
				// add char to resource
				resource_string += c;
			}
			break;
		
		case PARSE_VALUE:
			// parsing value portion
			if (c == '\r' || c == '\n') {
				// value is finished
				if (value_string.empty()) {
					// value must not be empty
					throw ConfigParsingException(config_name);
				} else if (command_string == "path") {
					// finished path command
					PionPlugin::addPluginDirectory(value_string);
				} else if (command_string == "service") {
					// finished service command
					loadService(resource_string, value_string);
				} else if (command_string == "option") {
					// finished option command
					std::string::size_type pos = value_string.find('=');
					if (pos == std::string::npos)
						throw ConfigParsingException(config_name);
					option_name_string = value_string.substr(0, pos);
					option_value_string = value_string.substr(pos + 1);
					setServiceOption(resource_string, option_name_string,
									 option_value_string);
				}
				command_string.clear();
				parse_state = PARSE_NEWLINE;
			} else if (c == ' ' || c == '\t') {
				// only skip leading whitespace (value may contain spaces, etc)
				if (! value_string.empty())
					value_string += c;
			} else {
				// add char to value
				value_string += c;
			}
			break;
		
		case PARSE_COMMENT:
			// skipping comment line
			if (c == '\r' || c == '\n')
				parse_state = PARSE_NEWLINE;
			break;
		}

		// read the next character
		c = config_stream.get();
	}
}

void HTTPServer::clearServices(void)
{
	boost::mutex::scoped_lock services_lock(m_mutex);
	m_services.clear();
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
	HTTPResponsePtr response(HTTPResponse::create(http_request, tcp_conn));
	response->setResponseCode(HTTPTypes::RESPONSE_CODE_BAD_REQUEST);
	response->setResponseMessage(HTTPTypes::RESPONSE_MESSAGE_BAD_REQUEST);
	response->writeNoCopy(BAD_REQUEST_HTML);
	response->send();
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
	HTTPResponsePtr response(HTTPResponse::create(http_request, tcp_conn));
	response->setResponseCode(HTTPTypes::RESPONSE_CODE_NOT_FOUND);
	response->setResponseMessage(HTTPTypes::RESPONSE_MESSAGE_NOT_FOUND);
	response->writeNoCopy(NOT_FOUND_HTML_START);
	response << http_request->getResource();
	response->writeNoCopy(NOT_FOUND_HTML_FINISH);
	response->send();
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
	HTTPResponsePtr response(HTTPResponse::create(http_request, tcp_conn));
	response->setResponseCode(HTTPTypes::RESPONSE_CODE_SERVER_ERROR);
	response->setResponseMessage(HTTPTypes::RESPONSE_MESSAGE_SERVER_ERROR);
	response->writeNoCopy(SERVER_ERROR_HTML_START);
	response << error_msg;
	response->writeNoCopy(SERVER_ERROR_HTML_FINISH);
	response->send();
}


// HTTPServer::WebServiceMap member functions

void HTTPServer::WebServiceMap::clear(void) {
	for (iterator i = begin(); i != end(); ++i) {
		if (i->second.second.is_open()) {
			i->second.second.destroy(i->second.first);
			i->second.second.close();
		} else {
			delete i->second.first;
		}
	}
	std::map<std::string, PluginPair>::clear();
}

}	// end namespace net
}	// end namespace pion

