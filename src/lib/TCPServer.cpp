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

#include "TCPServer.hpp"
#include "HTTPProtocol.hpp"
#include <boost/bind.hpp>

using boost::asio::ip::tcp;


namespace pion {	// begin namespace pion

// TCPServer member functions

TCPServer::TCPServer(boost::asio::io_service& io_service, const unsigned int port)
	: m_logger(log4cxx::Logger::getLogger("Pion.TCPServer")),
	m_tcp_acceptor(io_service), m_protocol(new HTTPProtocol), m_tcp_port(port), 
	m_is_listening(false)
{}

void TCPServer::start(void)
{
	// lock mutex for thread safety
	boost::mutex::scoped_lock server_lock(m_mutex);

	if (! m_is_listening) {
		LOG4CXX_INFO(m_logger, "Starting server on port " << getPort());
		tcp::endpoint endpoint(tcp::v4(), m_tcp_port);
		m_tcp_acceptor.open(endpoint.protocol());
		// allow the acceptor to reuse the address (i.e. SO_REUSEADDR)
		m_tcp_acceptor.set_option(tcp::acceptor::reuse_address(true));
		m_tcp_acceptor.bind(endpoint);
		m_tcp_acceptor.listen();
		m_is_listening = true;
		server_lock.unlock();
		listen();
	}
}

void TCPServer::stop(void)
{
	// lock mutex for thread safety
	boost::mutex::scoped_lock server_lock(m_mutex);

	if (m_is_listening) {
		// schedule stop request with io_service to finish any pending events
		m_tcp_acceptor.io_service().post(boost::bind(&TCPServer::handleStopRequest,
													 shared_from_this()));
	}
}

void TCPServer::listen(void)
{
	// lock mutex for thread safety
	boost::mutex::scoped_lock server_lock(m_mutex);

	if (m_is_listening) {
		TCPConnectionPtr new_connection(new TCPConnection(m_tcp_acceptor.io_service(),
														  boost::bind(&TCPServer::finishConnection,
																	  shared_from_this(), _1)));
		m_conn_pool.insert(new_connection);
		server_lock.unlock();
		m_tcp_acceptor.async_accept(new_connection->getSocket(),
									boost::bind(&TCPServer::handleConnection,
												shared_from_this(), new_connection,
												boost::asio::placeholders::error) );
	}
}

void TCPServer::handleStopRequest(void)
{
	// lock mutex for thread safety
	boost::mutex::scoped_lock server_lock(m_mutex);

	if (m_is_listening) {
		LOG4CXX_INFO(m_logger, "Shutting down server on port " << getPort());
	
		m_is_listening = false;
		m_tcp_acceptor.close();
	
		std::for_each(m_conn_pool.begin(), m_conn_pool.end(),
					  boost::bind(&TCPConnection::close, _1));
	
		m_conn_pool.clear();
	}
}

void TCPServer::handleConnection(TCPConnectionPtr& conn, const boost::asio::error& accept_error)
{
	if (accept_error) {
		finishConnection(conn);
	} else {
		LOG4CXX_INFO(m_logger, "New connection on port " << getPort());
		if (m_is_listening) listen();
		m_protocol->handleConnection(conn);
	}
}

void TCPServer::finishConnection(TCPConnectionPtr& conn)
{
	LOG4CXX_INFO(m_logger, "Closing connection on port " << getPort());

	// lock mutex for thread safety
	boost::mutex::scoped_lock server_lock(m_mutex);
	m_conn_pool.erase(conn);
	server_lock.unlock();
}

}	// end namespace pion
