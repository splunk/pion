// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/bind.hpp>
#include <pion/PionScheduler.hpp>
#include <pion/net/TCPServer.hpp>

using boost::asio::ip::tcp;


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)

// TCPServer member functions

TCPServer::TCPServer(const unsigned int tcp_port)
	: m_logger(PION_GET_LOGGER("pion.net.TCPServer")),
	m_tcp_acceptor(PionScheduler::getInstance().getIOService()),
#ifdef PION_HAVE_SSL
	m_ssl_context(PionScheduler::getInstance().getIOService(), boost::asio::ssl::context::sslv23),
#else
	m_ssl_context(0),
#endif
	m_tcp_port(tcp_port), m_ssl_flag(false), m_is_listening(false)
{}

void TCPServer::start(void)
{
	// lock mutex for thread safety
	boost::mutex::scoped_lock server_lock(m_mutex);

	if (! m_is_listening) {
		PION_LOG_INFO(m_logger, "Starting server on port " << getPort());
		
		beforeStarting();

		// configure the acceptor service
		tcp::endpoint endpoint(tcp::v4(), m_tcp_port);
		m_tcp_acceptor.open(endpoint.protocol());
		// allow the acceptor to reuse the address (i.e. SO_REUSEADDR)
		m_tcp_acceptor.set_option(tcp::acceptor::reuse_address(true));
		m_tcp_acceptor.bind(endpoint);
		m_tcp_acceptor.listen();

		m_is_listening = true;

		// unlock the mutex since listen() requires its own lock
		server_lock.unlock();
		listen();
	}
}

void TCPServer::stop(void)
{
	// lock mutex for thread safety
	boost::mutex::scoped_lock server_lock(m_mutex);

	if (m_is_listening) {
		PION_LOG_INFO(m_logger, "Shutting down server on port " << getPort());
	
		m_is_listening = false;

		// this terminates any pending connections
		m_tcp_acceptor.close();
	
		// close all of the TCP connections managed by this server instance
		std::for_each(m_conn_pool.begin(), m_conn_pool.end(),
					  boost::bind(&TCPConnection::close, _1));

		// clear the TCP connection management pool
		m_conn_pool.clear();
		
		afterStopping();
	}
}

void TCPServer::listen(void)
{
	// lock mutex for thread safety
	boost::mutex::scoped_lock server_lock(m_mutex);
	
	if (m_is_listening) {
		// create a new TCP connection object
		TCPConnectionPtr new_connection(TCPConnection::create(PionScheduler::getInstance().getIOService(),
															  m_ssl_context, m_ssl_flag,
															  boost::bind(&TCPServer::finishConnection,
																		  this, _1)));
		
		// keep track of the object in the server's connection pool
		m_conn_pool.insert(new_connection);
		
		// use the object to accept a new connection
		new_connection->async_accept(m_tcp_acceptor,
									 boost::bind(&TCPServer::handleAccept,
												 this, new_connection,
												 boost::asio::placeholders::error));

		// startup the thread scheduler (in case it is not running)
		PionScheduler::getInstance().startup();
	}
}

void TCPServer::handleAccept(TCPConnectionPtr& tcp_conn,
							 const boost::system::error_code& accept_error)
{
	if (accept_error) {
		// an error occured while trying to a accept a new connection
		// this happens when the server is being shut down
		if (m_is_listening) {
			listen();	// schedule acceptance of another connection
			finishConnection(tcp_conn);
			PION_LOG_WARN(m_logger, "Accept error on port " << getPort() << ": " << accept_error.message());
		}
	} else {
		// got a new TCP connection
		PION_LOG_INFO(m_logger, "New" << (tcp_conn->getSSLFlag() ? " SSL " : " ")
					  << "connection on port " << getPort());

		// schedule the acceptance of another new connection
		// (this returns immediately since it schedules it as an event)
		if (m_is_listening) listen();
		
		// handle the new connection
		if (tcp_conn->getSSLFlag()) {
			tcp_conn->ssl_handshake_server( boost::bind(&TCPServer::handleSSLHandshake,
														this, tcp_conn,
														boost::asio::placeholders::error));
		} else {
			// not SSL -> call the handler immediately
			handleConnection(tcp_conn);
		}
	}
}

void TCPServer::handleSSLHandshake(TCPConnectionPtr& tcp_conn,
								   const boost::system::error_code& handshake_error)
{
	if (handshake_error) {
		// an error occured while trying to establish the SSL connection
		PION_LOG_WARN(m_logger, "SSL handshake failed on port " << getPort()
					  << " (" << handshake_error.message() << ')');
		finishConnection(tcp_conn);
	} else {
		// handle the new connection
		PION_LOG_DEBUG(m_logger, "SSL handshake succeeded on port " << getPort());
		handleConnection(tcp_conn);
	}
}

void TCPServer::finishConnection(TCPConnectionPtr& tcp_conn)
{
	if (tcp_conn->getKeepAlive()) {
		
		// keep the connection alive
		handleConnection(tcp_conn);

	} else {
		PION_LOG_INFO(m_logger, "Closing connection on port " << getPort());
		
		// remove the connection from the server's management pool
		boost::mutex::scoped_lock server_lock(m_mutex);
		ConnectionPool::iterator conn_itr = m_conn_pool.find(tcp_conn);
		if (conn_itr != m_conn_pool.end())
			m_conn_pool.erase(conn_itr);
		server_lock.unlock();
	}
}

unsigned long TCPServer::getConnections(void) const
{
	boost::mutex::scoped_lock server_lock(m_mutex);
	return (m_is_listening ? (m_conn_pool.size() - 1) : m_conn_pool.size());
}


}	// end namespace net
}	// end namespace pion
