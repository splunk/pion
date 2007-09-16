// -----------------------------------------------------------------
// libpion: a C++ framework for building lightweight HTTP interfaces
// -----------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/bind.hpp>
#include <libpion/HTTPResponse.hpp>


namespace pion {	// begin namespace pion

	
// HTTPResponse member functions

void HTTPResponse::send(TCPConnectionPtr& tcp_conn)
{
	flushContentStream();
	
	// update headers
	m_response_headers.insert(std::make_pair(HTTPTypes::HEADER_CONTENT_TYPE, m_content_type));
	m_response_headers.insert(std::make_pair(HTTPTypes::HEADER_CONTENT_LENGTH,
									boost::lexical_cast<std::string>(m_content_length)));
	m_response_headers.insert(std::make_pair(HTTPTypes::HEADER_CONNECTION,
									(tcp_conn->getKeepAlive() ? "Keep-Alive" : "close") ));
	
	// combine I/O write buffers (headers and content) so that everything
	// can be sent together; otherwise, we would have to send headers
	// and content separately, which would not be as efficient
	
	WriteBuffers write_buffers;
	
	// first response line (HTTP/1.1 200 OK)
	write_buffers.push_back(boost::asio::buffer(HTTPTypes::STRING_HTTP_VERSION));
	write_buffers.push_back(boost::asio::buffer(m_response_code));
	write_buffers.push_back(boost::asio::buffer(m_response_message));
	write_buffers.push_back(boost::asio::buffer(HTTPTypes::STRING_CRLF));
	
	// HTTP headers
	for (HTTPTypes::StringDictionary::const_iterator i = m_response_headers.begin();
		 i != m_response_headers.end(); ++i)
	{
		write_buffers.push_back(boost::asio::buffer(i->first));
		write_buffers.push_back(boost::asio::buffer(HTTPTypes::HEADER_NAME_VALUE_DELIMITER));
		write_buffers.push_back(boost::asio::buffer(i->second));
		write_buffers.push_back(boost::asio::buffer(HTTPTypes::STRING_CRLF));
	}
	
	// extra CRLF to end HTTP headers
	write_buffers.push_back(boost::asio::buffer(HTTPTypes::STRING_CRLF));
	
	// append response content buffers
	write_buffers.insert(write_buffers.end(), m_content_buffers.begin(),
						 m_content_buffers.end());
	
	// send response
	tcp_conn->async_write(write_buffers,
						  boost::bind(&HTTPResponse::handleWrite, shared_from_this(),
									  tcp_conn, boost::asio::placeholders::error,
									  boost::asio::placeholders::bytes_transferred));
}

void HTTPResponse::handleWrite(TCPConnectionPtr tcp_conn,
							   const boost::system::error_code& write_error,
							   std::size_t bytes_written)
{
	if (write_error) {
		// encountered error sending response
		tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_CLOSE);	// make sure it will get closed
		PION_LOG_INFO(m_logger, "Unable to send HTTP response due to I/O error");
	} else {
		// response sent OK
		PION_LOG_DEBUG(m_logger, "Sent HTTP response of " << bytes_written << " bytes ("
					   << (tcp_conn->getKeepAlive() ? "keeping alive)" : "closing)"));
	}
	
	// TCPConnection::finish() calls TCPServer::finishConnection, which will either:
	// a) call HTTPServer::handleConnection again if keep-alive is true; or,
	// b) close the socket and remove it from the server's connection pool
	tcp_conn->finish();
}

std::string HTTPResponse::makeSetCookieHeader(const std::string& name,
											  const std::string& value,
											  const std::string& path,
											  const bool has_max_age,
											  const unsigned long max_age)
{
	std::string set_cookie_header(name);
	set_cookie_header += "=\"";
	set_cookie_header += value;
	set_cookie_header += "\"; Version=\"1\"";
	if (! path.empty()) {
		set_cookie_header += "; Path=\"";
		set_cookie_header += path;
		set_cookie_header += '\"';
	}
		if (has_max_age) {
			set_cookie_header += "; Max-Age=\"";
			set_cookie_header += boost::lexical_cast<std::string>(max_age);
			set_cookie_header += '\"';
		}
	return set_cookie_header;
}

}	// end namespace pion

