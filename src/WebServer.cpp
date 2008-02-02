// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/net/WebServer.hpp>
#include <pion/net/HTTPRequest.hpp>
#include <pion/net/HTTPRequestReader.hpp>
#include <pion/net/HTTPResponseWriter.hpp>
#include <fstream>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)


// WebServer member functions

void WebServer::addService(const std::string& resource, WebService *service_ptr)
{
	PionPluginPtr<WebService> plugin_ptr;
	const std::string clean_resource(stripTrailingSlash(resource));
	service_ptr->setResource(clean_resource);
	// catch exceptions thrown by services since their exceptions may be free'd
	// from memory before they are caught
	try {
		m_services.add(clean_resource, service_ptr);
		HTTPServer::addResource(clean_resource, boost::ref(*service_ptr));
	} catch (std::exception& e) {
		throw WebServiceException(resource, e.what());
	}
	PION_LOG_INFO(m_logger, "Loaded static web service for resource (" << clean_resource << ")");
}

void WebServer::loadService(const std::string& resource, const std::string& service_name)
{
	const std::string clean_resource(stripTrailingSlash(resource));
	WebService *service_ptr;
	// catch exceptions thrown by services since their exceptions may be free'd
	// from memory before they are caught
	try {
		service_ptr = m_services.load(clean_resource, service_name);
		HTTPServer::addResource(clean_resource, boost::ref(*service_ptr));
		service_ptr->setResource(clean_resource);
	} catch (std::exception& e) {
		throw WebServiceException(resource, e.what());
	}
	PION_LOG_INFO(m_logger, "Loaded web service plug-in for resource (" << clean_resource << "): " << service_name);
}

void WebServer::setServiceOption(const std::string& resource,
								 const std::string& name, const std::string& value)
{
	// catch exceptions thrown by services since their exceptions may be free'd
	// from memory before they are caught
	const std::string clean_resource(stripTrailingSlash(resource));
	try {
		m_services.run(clean_resource, boost::bind(&WebService::setOption, _1, name, value));
	} catch (PluginManager<WebService>::PluginNotFoundException&) {
		throw ServiceNotFoundException(resource);
	} catch (std::exception& e) {
		throw WebServiceException(resource, e.what());
	}
	PION_LOG_INFO(m_logger, "Set web service option for resource ("
				  << resource << "): " << name << '=' << value);
}

void WebServer::loadServiceConfig(const std::string& config_name)
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

}	// end namespace net
}	// end namespace pion

