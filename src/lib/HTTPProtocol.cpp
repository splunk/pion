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

#include "HTTPProtocol.hpp"
#include "HTTPRequestParser.hpp"
#include "HTTPResponse.hpp"
#include <boost/bind.hpp>
#include <boost/asio.hpp>


namespace pion {	// begin namespace pion

// HTTPProtocol member functions

void HTTPProtocol::handleConnection(TCPConnectionPtr& tcp_conn)
{
	HTTPRequestParserPtr request_parser(new HTTPRequestParser(boost::bind(&HTTPProtocol::handleRequest,
																		  shared_from_this(), _1, _2),
															  tcp_conn));
	request_parser->readRequest();
}

void HTTPProtocol::handleRequest(HTTPRequestPtr& http_request,
								 TCPConnectionPtr& tcp_conn)
{
	// create a keepalive handler that we can reuse when trying to
	// handle requests with modules
	TCPConnection::ConnectionHandler keepalive_handler =
		boost::bind(&HTTPProtocol::handleConnection, shared_from_this(), _1);

	if (! http_request->isValid()) {
		// the request is invalid or an error occured
		LOG4CXX_INFO(m_logger, "Received an invalid HTTP request");
		if (! m_bad_request_module->handleRequest(http_request, tcp_conn, keepalive_handler)) {
			// this shouldn't ever happen, but just in case
			tcp_conn->finish();
		}
		return;
	}
		
	LOG4CXX_DEBUG(m_logger, "Received a valid HTTP request");
	
	// strip off trailing slash if the request has one
	std::string resource(http_request->getResource());
	if (! resource.empty() && resource[resource.size() - 1] == '/')
		resource.resize( resource.size() - 1 );

	// true if a module successfully handled the request
	bool request_was_handled = false;
	
	if (m_modules.empty()) {
		
		// no modules configured
		LOG4CXX_WARN(m_logger, "No modules configured");
		
	} else {

		// iterate through each module that may be able to handle the request
		ModuleMap::iterator i = m_modules.upper_bound(resource);
		while (i != m_modules.begin()) {
			--i;
			// keep checking while the first part of the strings match
			if (i->second->checkResource(resource)) {
				
				// try to handle the request with the module
				request_was_handled = i->second->handleRequest(http_request,
															   tcp_conn,
															   keepalive_handler);

				if (request_was_handled) {
					// the module successfully handled the request
					LOG4CXX_DEBUG(m_logger, "HTTP request handled by module: " << i->second->getResource());
					break;
				}
			} else {
				// we've gone to far; the first part no longer matches
				break;
			}
		}
	}
	
	if (! request_was_handled) {
		// no modules found that could handle the request
		LOG4CXX_INFO(m_logger, "No modules found to handle HTTP request: " << resource);
		if (! m_not_found_module->handleRequest(http_request, tcp_conn, keepalive_handler)) {
			// this shouldn't ever happen, but just in case
			tcp_conn->finish();
		}
	}
}

void HTTPProtocol::addModule(HTTPModulePtr m)
{
	// lock mutex for thread safety
	boost::mutex::scoped_lock modules_lock(m_mutex);
	m_modules.insert(std::make_pair(m->getResource(), m));
}

void HTTPProtocol::clearModules(void)
{
	// lock mutex for thread safety
	boost::mutex::scoped_lock modules_lock(m_mutex);
	m_modules.clear();
}


// static members of HTTPProtocol::BadRequestModule

const std::string HTTPProtocol::BadRequestModule::BAD_REQUEST_HTML =
	"<html><body>The request is <em>invalid</em></body></html>\r\n\r\n";


// HTTPProtocol::BadRequestModule member functions

bool HTTPProtocol::BadRequestModule::handleRequest(HTTPRequestPtr& request,
												   TCPConnectionPtr& tcp_conn,
												   TCPConnection::ConnectionHandler& keepalive_handler)
{
	HTTPResponsePtr response(HTTPResponse::create(keepalive_handler, tcp_conn));
	response->setResponseCode(HTTPTypes::RESPONSE_CODE_BAD_REQUEST);
	response->setResponseMessage(HTTPTypes::RESPONSE_MESSAGE_BAD_REQUEST);
	response->writeNoCopy(BAD_REQUEST_HTML);
	response->send(request->checkKeepAlive());
	return true;
}


// static members of HTTPProtocol::NotFoundModule

const std::string HTTPProtocol::NotFoundModule::NOT_FOUND_HTML =
	"<html><body>Request Not Found</body></html>\r\n\r\n";


// HTTPProtocol::BadRequestModule member functions

bool HTTPProtocol::NotFoundModule::handleRequest(HTTPRequestPtr& request,
												 TCPConnectionPtr& tcp_conn,
												 TCPConnection::ConnectionHandler& keepalive_handler)
{
	HTTPResponsePtr response(HTTPResponse::create(keepalive_handler, tcp_conn));
	response->setResponseCode(HTTPTypes::RESPONSE_CODE_NOT_FOUND);
	response->setResponseMessage(HTTPTypes::RESPONSE_MESSAGE_NOT_FOUND);
	response->writeNoCopy(NOT_FOUND_HTML);
	response->send(request->checkKeepAlive());
	return true;
}


}	// end namespace pion

