// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <pion/admin_rights.hpp>
#include <pion/tcp/server.hpp>

using boost::asio::ip::tcp;


namespace pion {    // begin namespace pion
namespace net {     // begin namespace net (Pion Network Library)

    
// TCPServer member functions

TCPServer::TCPServer(PionScheduler& scheduler, const unsigned int tcp_port)
    : m_logger(PION_GET_LOGGER("pion.net.TCPServer")),
    m_active_scheduler(scheduler),
    m_tcp_acceptor(m_active_scheduler.getIOService()),
#ifdef PION_HAVE_SSL
    m_ssl_context(m_active_scheduler.getIOService(), boost::asio::ssl::context::sslv23),
#else
    m_ssl_context(0),
#endif
    m_endpoint(tcp::v4(), tcp_port), m_ssl_flag(false), m_is_listening(false)
{}
    
TCPServer::TCPServer(PionScheduler& scheduler, const tcp::endpoint& endpoint)
    : m_logger(PION_GET_LOGGER("pion.net.TCPServer")),
    m_active_scheduler(scheduler),
    m_tcp_acceptor(m_active_scheduler.getIOService()),
#ifdef PION_HAVE_SSL
    m_ssl_context(m_active_scheduler.getIOService(), boost::asio::ssl::context::sslv23),
#else
    m_ssl_context(0),
#endif
    m_endpoint(endpoint), m_ssl_flag(false), m_is_listening(false)
{}

TCPServer::TCPServer(const unsigned int tcp_port)
    : m_logger(PION_GET_LOGGER("pion.net.TCPServer")),
    m_default_scheduler(), m_active_scheduler(m_default_scheduler),
    m_tcp_acceptor(m_active_scheduler.getIOService()),
#ifdef PION_HAVE_SSL
    m_ssl_context(m_active_scheduler.getIOService(), boost::asio::ssl::context::sslv23),
#else
    m_ssl_context(0),
#endif
    m_endpoint(tcp::v4(), tcp_port), m_ssl_flag(false), m_is_listening(false)
{}

TCPServer::TCPServer(const tcp::endpoint& endpoint)
    : m_logger(PION_GET_LOGGER("pion.net.TCPServer")),
    m_default_scheduler(), m_active_scheduler(m_default_scheduler),
    m_tcp_acceptor(m_active_scheduler.getIOService()),
#ifdef PION_HAVE_SSL
    m_ssl_context(m_active_scheduler.getIOService(), boost::asio::ssl::context::sslv23),
#else
    m_ssl_context(0),
#endif
    m_endpoint(endpoint), m_ssl_flag(false), m_is_listening(false)
{}
    
void TCPServer::start(void)
{
    // lock mutex for thread safety
    boost::mutex::scoped_lock server_lock(m_mutex);

    if (! m_is_listening) {
        PION_LOG_INFO(m_logger, "Starting server on port " << getPort());
        
        beforeStarting();

        // configure the acceptor service
        try {
            // get admin permissions in case we're binding to a privileged port
            pion::PionAdminRights use_admin_rights(getPort() < 1024);
            m_tcp_acceptor.open(m_endpoint.protocol());
            // allow the acceptor to reuse the address (i.e. SO_REUSEADDR)
            // ...except when running not on Windows - see http://msdn.microsoft.com/en-us/library/ms740621%28VS.85%29.aspx
#ifndef _MSC_VER
            m_tcp_acceptor.set_option(tcp::acceptor::reuse_address(true));
#endif
            m_tcp_acceptor.bind(m_endpoint);
            if (m_endpoint.port() == 0) {
                // update the endpoint to reflect the port chosen by bind
                m_endpoint = m_tcp_acceptor.local_endpoint();
            }
            m_tcp_acceptor.listen();
        } catch (std::exception& e) {
            PION_LOG_ERROR(m_logger, "Unable to bind to port " << getPort() << ": " << e.what());
            throw;
        }

        m_is_listening = true;

        // unlock the mutex since listen() requires its own lock
        server_lock.unlock();
        listen();
        
        // notify the thread scheduler that we need it now
        m_active_scheduler.addActiveUser();
    }
}

void TCPServer::stop(bool wait_until_finished)
{
    // lock mutex for thread safety
    boost::mutex::scoped_lock server_lock(m_mutex);

    if (m_is_listening) {
        PION_LOG_INFO(m_logger, "Shutting down server on port " << getPort());
    
        m_is_listening = false;

        // this terminates any connections waiting to be accepted
        m_tcp_acceptor.close();
        
        if (! wait_until_finished) {
            // this terminates any other open connections
            std::for_each(m_conn_pool.begin(), m_conn_pool.end(),
                          boost::bind(&TCPConnection::close, _1));
        }
    
        // wait for all pending connections to complete
        while (! m_conn_pool.empty()) {
            // try to prun connections that didn't finish cleanly
            if (pruneConnections() == 0)
                break;  // if no more left, then we can stop waiting
            // sleep for up to a quarter second to give open connections a chance to finish
            PION_LOG_INFO(m_logger, "Waiting for open connections to finish");
            PionScheduler::sleep(m_no_more_connections, server_lock, 0, 250000000);
        }
        
        // notify the thread scheduler that we no longer need it
        m_active_scheduler.removeActiveUser();
        
        // all done!
        afterStopping();
        m_server_has_stopped.notify_all();
    }
}

void TCPServer::join(void)
{
    boost::mutex::scoped_lock server_lock(m_mutex);
    while (m_is_listening) {
        // sleep until server_has_stopped condition is signaled
        m_server_has_stopped.wait(server_lock);
    }
}

void TCPServer::setSSLKeyFile(const std::string& pem_key_file)
{
    // configure server for SSL
    setSSLFlag(true);
#ifdef PION_HAVE_SSL
    m_ssl_context.set_options(boost::asio::ssl::context::default_workarounds
                              | boost::asio::ssl::context::no_sslv2
                              | boost::asio::ssl::context::single_dh_use);
    m_ssl_context.use_certificate_file(pem_key_file, boost::asio::ssl::context::pem);
    m_ssl_context.use_private_key_file(pem_key_file, boost::asio::ssl::context::pem);
#endif
}

void TCPServer::listen(void)
{
    // lock mutex for thread safety
    boost::mutex::scoped_lock server_lock(m_mutex);
    
    if (m_is_listening) {
        // create a new TCP connection object
        TCPConnectionPtr new_connection(TCPConnection::create(getIOService(),
                                                              m_ssl_context, m_ssl_flag,
                                                              boost::bind(&TCPServer::finishConnection,
                                                                          this, _1)));
        
        // prune connections that finished uncleanly
        pruneConnections();

        // keep track of the object in the server's connection pool
        m_conn_pool.insert(new_connection);
        
        // use the object to accept a new connection
        new_connection->async_accept(m_tcp_acceptor,
                                     boost::bind(&TCPServer::handleAccept,
                                                 this, new_connection,
                                                 boost::asio::placeholders::error));
    }
}

void TCPServer::handleAccept(TCPConnectionPtr& tcp_conn,
                             const boost::system::error_code& accept_error)
{
    if (accept_error) {
        // an error occured while trying to a accept a new connection
        // this happens when the server is being shut down
        if (m_is_listening) {
            listen();   // schedule acceptance of another connection
            PION_LOG_WARN(m_logger, "Accept error on port " << getPort() << ": " << accept_error.message());
        }
        finishConnection(tcp_conn);
    } else {
        // got a new TCP connection
        PION_LOG_DEBUG(m_logger, "New" << (tcp_conn->getSSLFlag() ? " SSL " : " ")
                       << "connection on port " << getPort());

        // schedule the acceptance of another new connection
        // (this returns immediately since it schedules it as an event)
        if (m_is_listening) listen();
        
        // handle the new connection
#ifdef PION_HAVE_SSL
        if (tcp_conn->getSSLFlag()) {
            tcp_conn->async_handshake_server(boost::bind(&TCPServer::handleSSLHandshake,
                                                         this, tcp_conn,
                                                         boost::asio::placeholders::error));
        } else
#endif
            // not SSL -> call the handler immediately
            handleConnection(tcp_conn);
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
    boost::mutex::scoped_lock server_lock(m_mutex);
    if (m_is_listening && tcp_conn->getKeepAlive()) {
        
        // keep the connection alive
        handleConnection(tcp_conn);

    } else {
        PION_LOG_DEBUG(m_logger, "Closing connection on port " << getPort());
        
        // remove the connection from the server's management pool
        ConnectionPool::iterator conn_itr = m_conn_pool.find(tcp_conn);
        if (conn_itr != m_conn_pool.end())
            m_conn_pool.erase(conn_itr);

        // trigger the no more connections condition if we're waiting to stop
        if (!m_is_listening && m_conn_pool.empty())
            m_no_more_connections.notify_all();
    }
}

std::size_t TCPServer::pruneConnections(void)
{
    // assumes that a server lock has already been acquired
    ConnectionPool::iterator conn_itr = m_conn_pool.begin();
    while (conn_itr != m_conn_pool.end()) {
        if (conn_itr->unique()) {
            PION_LOG_WARN(m_logger, "Closing orphaned connection on port " << getPort());
            ConnectionPool::iterator erase_itr = conn_itr;
            ++conn_itr;
            (*erase_itr)->close();
            m_conn_pool.erase(erase_itr);
        } else {
            ++conn_itr;
        }
    }

    // return the number of connections remaining
    return m_conn_pool.size();
}

std::size_t TCPServer::getConnections(void) const
{
    boost::mutex::scoped_lock server_lock(m_mutex);
    return (m_is_listening ? (m_conn_pool.size() - 1) : m_conn_pool.size());
}

}   // end namespace net
}   // end namespace pion
