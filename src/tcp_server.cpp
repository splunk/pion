// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <pion/admin_rights.hpp>
#include <pion/tcp/server.hpp>


namespace pion {    // begin namespace pion
namespace tcp {     // begin namespace tcp

    
// tcp::server member functions

server::server(scheduler& sched, const unsigned int tcp_port)
    : m_logger(PION_GET_LOGGER("pion.tcp.server")),
    m_active_scheduler(sched),
    m_tcp_acceptor(m_active_scheduler.get_io_service()),
#ifdef PION_HAVE_SSL
    m_ssl_context(m_active_scheduler.get_io_service(), boost::asio::ssl::context::sslv23),
#else
    m_ssl_context(0),
#endif
    m_endpoint(boost::asio::ip::tcp::v4(), tcp_port), m_ssl_flag(false), m_is_listening(false)
{}
    
server::server(scheduler& sched, const boost::asio::ip::tcp::endpoint& endpoint)
    : m_logger(PION_GET_LOGGER("pion.tcp.server")),
    m_active_scheduler(sched),
    m_tcp_acceptor(m_active_scheduler.get_io_service()),
#ifdef PION_HAVE_SSL
    m_ssl_context(m_active_scheduler.get_io_service(), boost::asio::ssl::context::sslv23),
#else
    m_ssl_context(0),
#endif
    m_endpoint(endpoint), m_ssl_flag(false), m_is_listening(false)
{}

server::server(const unsigned int tcp_port)
    : m_logger(PION_GET_LOGGER("pion.tcp.server")),
    m_default_scheduler(), m_active_scheduler(m_default_scheduler),
    m_tcp_acceptor(m_active_scheduler.get_io_service()),
#ifdef PION_HAVE_SSL
    m_ssl_context(m_active_scheduler.get_io_service(), boost::asio::ssl::context::sslv23),
#else
    m_ssl_context(0),
#endif
    m_endpoint(boost::asio::ip::tcp::v4(), tcp_port), m_ssl_flag(false), m_is_listening(false)
{}

server::server(const boost::asio::ip::tcp::endpoint& endpoint)
    : m_logger(PION_GET_LOGGER("pion.tcp.server")),
    m_default_scheduler(), m_active_scheduler(m_default_scheduler),
    m_tcp_acceptor(m_active_scheduler.get_io_service()),
#ifdef PION_HAVE_SSL
    m_ssl_context(m_active_scheduler.get_io_service(), boost::asio::ssl::context::sslv23),
#else
    m_ssl_context(0),
#endif
    m_endpoint(endpoint), m_ssl_flag(false), m_is_listening(false)
{}
    
void server::start(void)
{
    // lock mutex for thread safety
    boost::mutex::scoped_lock server_lock(m_mutex);

    if (! m_is_listening) {
        PION_LOG_INFO(m_logger, "Starting server on port " << get_port());
        
        before_starting();

        // configure the acceptor service
        try {
            // get admin permissions in case we're binding to a privileged port
            pion::admin_rights use_admin_rights(get_port() > 0 && get_port() < 1024);
            m_tcp_acceptor.open(m_endpoint.protocol());
            // allow the acceptor to reuse the address (i.e. SO_REUSEADDR)
            // ...except when running not on Windows - see http://msdn.microsoft.com/en-us/library/ms740621%28VS.85%29.aspx
#ifndef PION_WIN32
            m_tcp_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
#endif
            m_tcp_acceptor.bind(m_endpoint);
            if (m_endpoint.port() == 0) {
                // update the endpoint to reflect the port chosen by bind
                m_endpoint = m_tcp_acceptor.local_endpoint();
            }
            m_tcp_acceptor.listen();
        } catch (std::exception& e) {
            PION_LOG_ERROR(m_logger, "Unable to bind to port " << get_port() << ": " << e.what());
            throw;
        }

        m_is_listening = true;

        // unlock the mutex since listen() requires its own lock
        server_lock.unlock();
        listen();
        
        // notify the thread scheduler that we need it now
        m_active_scheduler.add_active_user();
    }
}

void server::stop(bool wait_until_finished)
{
    // lock mutex for thread safety
    boost::mutex::scoped_lock server_lock(m_mutex);

    if (m_is_listening) {
        PION_LOG_INFO(m_logger, "Shutting down server on port " << get_port());
    
        m_is_listening = false;

        // this terminates any connections waiting to be accepted
        m_tcp_acceptor.close();
        
        if (! wait_until_finished) {
            // this terminates any other open connections
            std::for_each(m_conn_pool.begin(), m_conn_pool.end(),
                          boost::bind(&connection::close, _1));
        }
    
        // wait for all pending connections to complete
        while (! m_conn_pool.empty()) {
            // try to prun connections that didn't finish cleanly
            if (prune_connections() == 0)
                break;  // if no more left, then we can stop waiting
            // sleep for up to a quarter second to give open connections a chance to finish
            PION_LOG_INFO(m_logger, "Waiting for open connections to finish");
            scheduler::sleep(m_no_more_connections, server_lock, 0, 250000000);
        }
        
        // notify the thread scheduler that we no longer need it
        m_active_scheduler.remove_active_user();
        
        // all done!
        after_stopping();
        m_server_has_stopped.notify_all();
    }
}

void server::join(void)
{
    boost::mutex::scoped_lock server_lock(m_mutex);
    while (m_is_listening) {
        // sleep until server_has_stopped condition is signaled
        m_server_has_stopped.wait(server_lock);
    }
}

void server::set_ssl_key_file(const std::string& pem_key_file)
{
    // configure server for SSL
    set_ssl_flag(true);
#ifdef PION_HAVE_SSL
    m_ssl_context.set_options(boost::asio::ssl::context::default_workarounds
                              | boost::asio::ssl::context::no_sslv2
                              | boost::asio::ssl::context::single_dh_use);
    m_ssl_context.use_certificate_file(pem_key_file, boost::asio::ssl::context::pem);
    m_ssl_context.use_private_key_file(pem_key_file, boost::asio::ssl::context::pem);
#endif
}

void server::listen(void)
{
    // lock mutex for thread safety
    boost::mutex::scoped_lock server_lock(m_mutex);
    
    if (m_is_listening) {
        // create a new TCP connection object
        tcp::connection_ptr new_connection(connection::create(get_io_service(),
                                                              m_ssl_context, m_ssl_flag,
                                                              boost::bind(&server::finish_connection,
                                                                          this, _1)));
        
        // prune connections that finished uncleanly
        prune_connections();

        // keep track of the object in the server's connection pool
        m_conn_pool.insert(new_connection);
        
        // use the object to accept a new connection
        new_connection->async_accept(m_tcp_acceptor,
                                     boost::bind(&server::handle_accept,
                                                 this, new_connection,
                                                 boost::asio::placeholders::error));
    }
}

void server::handle_accept(const tcp::connection_ptr& tcp_conn,
                             const boost::system::error_code& accept_error)
{
    if (accept_error) {
        // an error occured while trying to a accept a new connection
        // this happens when the server is being shut down
        if (m_is_listening) {
            listen();   // schedule acceptance of another connection
            PION_LOG_WARN(m_logger, "Accept error on port " << get_port() << ": " << accept_error.message());
        }
        finish_connection(tcp_conn);
    } else {
        // got a new TCP connection
        PION_LOG_DEBUG(m_logger, "New" << (tcp_conn->get_ssl_flag() ? " SSL " : " ")
                       << "connection on port " << get_port());

        // schedule the acceptance of another new connection
        // (this returns immediately since it schedules it as an event)
        if (m_is_listening) listen();
        
        // handle the new connection
#ifdef PION_HAVE_SSL
        if (tcp_conn->get_ssl_flag()) {
            tcp_conn->async_handshake_server(boost::bind(&server::handle_ssl_handshake,
                                                         this, tcp_conn,
                                                         boost::asio::placeholders::error));
        } else
#endif
            // not SSL -> call the handler immediately
            handle_connection(tcp_conn);
    }
}

void server::handle_ssl_handshake(const tcp::connection_ptr& tcp_conn,
                                   const boost::system::error_code& handshake_error)
{
    if (handshake_error) {
        // an error occured while trying to establish the SSL connection
        PION_LOG_WARN(m_logger, "SSL handshake failed on port " << get_port()
                      << " (" << handshake_error.message() << ')');
        finish_connection(tcp_conn);
    } else {
        // handle the new connection
        PION_LOG_DEBUG(m_logger, "SSL handshake succeeded on port " << get_port());
        handle_connection(tcp_conn);
    }
}

void server::finish_connection(const tcp::connection_ptr& tcp_conn)
{
    boost::mutex::scoped_lock server_lock(m_mutex);
    if (m_is_listening && tcp_conn->get_keep_alive()) {
        
        // keep the connection alive
        handle_connection(tcp_conn);

    } else {
        PION_LOG_DEBUG(m_logger, "Closing connection on port " << get_port());
        
        // remove the connection from the server's management pool
        ConnectionPool::iterator conn_itr = m_conn_pool.find(tcp_conn);
        if (conn_itr != m_conn_pool.end())
            m_conn_pool.erase(conn_itr);

        // trigger the no more connections condition if we're waiting to stop
        if (!m_is_listening && m_conn_pool.empty())
            m_no_more_connections.notify_all();
    }
}

std::size_t server::prune_connections(void)
{
    // assumes that a server lock has already been acquired
    ConnectionPool::iterator conn_itr = m_conn_pool.begin();
    while (conn_itr != m_conn_pool.end()) {
        if (conn_itr->unique()) {
            PION_LOG_WARN(m_logger, "Closing orphaned connection on port " << get_port());
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

std::size_t server::get_connections(void) const
{
    boost::mutex::scoped_lock server_lock(m_mutex);
    return (m_is_listening ? (m_conn_pool.size() - 1) : m_conn_pool.size());
}

}   // end namespace tcp
}   // end namespace pion
