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
#include <pion/net/HTTPRequestReader.hpp>
#include <pion/net/HTTPResponseWriter.hpp>
#include <fstream>


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
	
	// search for a service object matching the resource requested
	WebService *service_ptr = m_services.find(resource_requested);
	
	if (service_ptr == NULL) {
		
		if (m_services.empty()) {
			// no services configured
			PION_LOG_WARN(m_logger, "No web services configured");
		} else {
			// no web services found that could handle the request
			PION_LOG_INFO(m_logger, "No web services found to handle HTTP request: "
						  << resource_requested);
		}
		m_not_found_handler(http_request, tcp_conn);

	} else {
		
		// try to handle the request with the web service
		try {
			service_ptr->handleRequest(http_request, tcp_conn);
			PION_LOG_DEBUG(m_logger, "HTTP request handled by web service ("
						   << service_ptr->getResource() << "): "
						   << resource_requested);
		} catch (HTTPResponseWriter::LostConnectionException& e) {
			// the connection was lost while or before sending the response
			PION_LOG_WARN(m_logger, "Web service (" << service_ptr->getResource() << "): " << e.what());
			tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_CLOSE);	// make sure it will get closed
			tcp_conn->finish();
		} catch (std::bad_alloc&) {
			// propagate memory errors (FATAL)
			throw;
		} catch (std::exception& e) {
			// recover gracefully from other exceptions thrown by services
			PION_LOG_ERROR(m_logger, "WebService (" << service_ptr->getResource() << "): " << e.what());
			m_server_error_handler(http_request, tcp_conn, e.what());
		}
	}		
}

void HTTPServer::addService(const std::string& resource, WebService *service_ptr)
{
	PionPluginPtr<WebService> plugin_ptr;
	const std::string clean_resource(stripTrailingSlash(resource));
	service_ptr->setResource(clean_resource);
	// catch exceptions thrown by services since their exceptions may be free'd
	// from memory before they are caught
	try { m_services.add(clean_resource, service_ptr); }
	catch (std::exception& e) {
		throw WebServiceException(resource, e.what());
	}
	PION_LOG_INFO(m_logger, "Loaded static web service for resource (" << clean_resource << ")");
}

void HTTPServer::loadService(const std::string& resource, const std::string& service_name)
{
	const std::string clean_resource(stripTrailingSlash(resource));
	WebService *service_ptr;
	// catch exceptions thrown by services since their exceptions may be free'd
	// from memory before they are caught
	try { service_ptr = m_services.load(clean_resource, service_name); }
	catch (std::exception& e) {
		throw WebServiceException(resource, e.what());
	}
	service_ptr->setResource(clean_resource);
	PION_LOG_INFO(m_logger, "Loaded web service plug-in for resource (" << clean_resource << "): " << service_name);
}

void HTTPServer::setServiceOption(const std::string& resource,
								  const std::string& name, const std::string& value)
{
	// catch exceptions thrown by services since their exceptions may be free'd
	// from memory before they are caught
	try { m_services.setOption(resource, name, value); }
	catch (std::exception& e) {
		throw WebServiceException(resource, e.what());
	}
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
	HTTPResponseWriterPtr writer(HTTPResponseWriter::create(tcp_conn, *http_request));
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
	HTTPResponseWriterPtr writer(HTTPResponseWriter::create(tcp_conn, *http_request));
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
	HTTPResponseWriterPtr writer(HTTPResponseWriter::create(tcp_conn, *http_request));
	writer->getResponse().setStatusCode(HTTPTypes::RESPONSE_CODE_SERVER_ERROR);
	writer->getResponse().setStatusMessage(HTTPTypes::RESPONSE_MESSAGE_SERVER_ERROR);
	writer->writeNoCopy(SERVER_ERROR_HTML_START);
	writer << error_msg;
	writer->writeNoCopy(SERVER_ERROR_HTML_FINISH);
	writer->send();
}

}	// end namespace net
}	// end namespace pion

