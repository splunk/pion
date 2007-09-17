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

	// true if a module successfully handled the request
	bool request_was_handled = false;
	
	// lock mutex for thread safety (this should probably use ref counters)
	boost::mutex::scoped_lock modules_lock(m_mutex);

	if (m_modules.empty()) {
		
		// no modules configured
		PION_LOG_WARN(m_logger, "No modules configured");
		
	} else {

		// iterate through each module that may be able to handle the request
		ModuleMap::iterator i = m_modules.upper_bound(resource);
		while (i != m_modules.begin()) {
			--i;
			
			// keep checking while the first part of the strings match
			if (resource.compare(0, i->first.size(), i->first) != 0) {
				// the first part no longer matches
				if (i != m_modules.begin()) {
					// continue to next module in list if its size is < this one
					ModuleMap::iterator j=i;
					--j;
					if (j->first.size() < i->first.size())
						continue;
				}
				// otherwise we've reached the end; stop looking for a match
				break;
			}
				
			// only try the module if the request matches the module name or
			// if module name is followed first with a '/' character
			if (resource.size() == i->first.size() || resource[i->first.size()]=='/') {
				
				// try to handle the request with the module
				try {
					request_was_handled = i->second.first->handleRequest(http_request, tcp_conn);
				} catch (std::bad_alloc&) {
					// propagate memory errors (FATAL)
					throw;
				} catch (std::exception& e) {
					// recover gracefully from other exceptions thrown by modules
					PION_LOG_ERROR(m_logger, "HTTP module (" << resource << ") exception: " << e.what());
					m_server_error_handler(http_request, tcp_conn, e.what());
					request_was_handled = true;
					break;
				}

				if (request_was_handled) {
					// the module successfully handled the request
					PION_LOG_DEBUG(m_logger, "HTTP request handled by module ("
								   << i->first << "): "
								   << http_request->getResource());
					break;
				}
			}
		}
	}
	
	if (! request_was_handled) {
		// no modules found that could handle the request
		PION_LOG_INFO(m_logger, "No modules found to handle HTTP request: " << resource);
		m_not_found_handler(http_request, tcp_conn);
	}
}

void HTTPServer::beforeStarting(void)
{
	// call the start() method for each module associated with this server
	boost::mutex::scoped_lock modules_lock(m_mutex);
	for (ModuleMap::iterator i = m_modules.begin(); i != m_modules.end(); ++i)
		i->second.first->start();
}

void HTTPServer::afterStopping(void)
{
	// call the stop() method for each module associated with this server
	boost::mutex::scoped_lock modules_lock(m_mutex);
	for (ModuleMap::iterator i = m_modules.begin(); i != m_modules.end(); ++i)
		i->second.first->stop();
}

void HTTPServer::addModule(const std::string& resource, HTTPModule *module_ptr)
{
	PionPluginPtr<HTTPModule> plugin_ptr;
	module_ptr->setResource(resource);	// strips any trailing '/' from the name
	boost::mutex::scoped_lock modules_lock(m_mutex);
	m_modules.insert(std::make_pair(module_ptr->getResource(),
									std::make_pair(module_ptr, plugin_ptr)));
}

void HTTPServer::loadModule(const std::string& resource, const std::string& module_name)
{
	// search for the plug-in file using the configured paths
	bool is_static;
	void *create_func;
	void *destroy_func;
	std::string module_file;
	
	// check if module is statically linked, and if not, try to resolve for dynamic
	is_static = PionPlugin::findStaticEntryPoint(module_name, &create_func, &destroy_func);
	if (!is_static) {
		if (!PionPlugin::findPluginFile(module_file, module_name))
			throw PionPlugin::PluginNotFoundException(module_name);
	}

	// open up the plug-in's shared object library
	PionPluginPtr<HTTPModule> plugin_ptr;
	if (is_static) {
		plugin_ptr.openStaticLinked(module_name, create_func, destroy_func);	// may throw
	} else {
		plugin_ptr.open(module_file);	// may throw
	}

	// create a new module using the plug-in library
	HTTPModule *module_ptr(plugin_ptr.create());
	module_ptr->setResource(resource);	// strips any trailing '/' from the name

	// add the module to the server's collection
	boost::mutex::scoped_lock modules_lock(m_mutex);
	m_modules.insert(std::make_pair(module_ptr->getResource(),
									std::make_pair(module_ptr, plugin_ptr)));
	modules_lock.unlock();

	if (is_static){
		PION_LOG_INFO(m_logger, "Loaded HTTP static module for resource (" << resource << "): " << module_name);
	} else {
		PION_LOG_INFO(m_logger, "Loaded HTTP module for resource (" << resource << "): " << module_file);
	}
}

void HTTPServer::setModuleOption(const std::string& resource,
								 const std::string& name, const std::string& value)
{
	boost::mutex::scoped_lock modules_lock(m_mutex);

	// find the module associated with resource & set the option
	// if resource == "/" then look for module with an empty string
	ModuleMap::iterator i = (resource == "/" ? m_modules.find("") : m_modules.find(resource));
	if (i == m_modules.end())
		throw ModuleNotFoundException(resource);
	i->second.first->setOption(name, value);

	modules_lock.unlock();
	PION_LOG_INFO(m_logger, "Set module option for resource ("
				  << resource << "): " << name << '=' << value);
}

void HTTPServer::loadModuleConfig(const std::string& config_name)
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
				} else if (command_string=="module" || command_string=="option") {
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
				} else if (command_string == "module") {
					// finished module command
					loadModule(resource_string, value_string);
				} else if (command_string == "option") {
					// finished option command
					std::string::size_type pos = value_string.find('=');
					if (pos == std::string::npos)
						throw ConfigParsingException(config_name);
					option_name_string = value_string.substr(0, pos);
					option_value_string = value_string.substr(pos + 1);
					setModuleOption(resource_string, option_name_string,
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

void HTTPServer::clearModules(void)
{
	boost::mutex::scoped_lock modules_lock(m_mutex);
	m_modules.clear();
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
	HTTPResponsePtr response(HTTPResponse::create());
	response->setResponseCode(HTTPTypes::RESPONSE_CODE_BAD_REQUEST);
	response->setResponseMessage(HTTPTypes::RESPONSE_MESSAGE_BAD_REQUEST);
	response->writeNoCopy(BAD_REQUEST_HTML);
	response->send(tcp_conn);
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
	HTTPResponsePtr response(HTTPResponse::create());
	response->setResponseCode(HTTPTypes::RESPONSE_CODE_NOT_FOUND);
	response->setResponseMessage(HTTPTypes::RESPONSE_MESSAGE_NOT_FOUND);
	response->writeNoCopy(NOT_FOUND_HTML_START);
	response << http_request->getResource();
	response->writeNoCopy(NOT_FOUND_HTML_FINISH);
	response->send(tcp_conn);
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
	HTTPResponsePtr response(HTTPResponse::create());
	response->setResponseCode(HTTPTypes::RESPONSE_CODE_SERVER_ERROR);
	response->setResponseMessage(HTTPTypes::RESPONSE_MESSAGE_SERVER_ERROR);
	response->writeNoCopy(SERVER_ERROR_HTML_START);
	response << error_msg;
	response->writeNoCopy(SERVER_ERROR_HTML_FINISH);
	response->send(tcp_conn);
}


// HTTPServer::ModuleMap member functions

void HTTPServer::ModuleMap::clear(void) {
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

