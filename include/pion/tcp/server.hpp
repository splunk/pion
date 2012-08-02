// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_TCP_SERVER_HEADER__
#define __PION_TCP_SERVER_HEADER__

#include <set>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <pion/config.hpp>
#include <pion/logger.hpp>
#include <pion/scheduler.hpp>
#include <pion/tcp/connection.hpp>


namespace pion {    // begin namespace pion
namespace tcp {     // begin namespace tcp


///
/// tcp::server: a multi-threaded, asynchronous TCP server
/// 
class PION_API server :
    private boost::noncopyable
{
public:

    /// default destructor
    virtual ~server() { if (m_is_listening) stop(false); }
    
    /// starts listening for new connections
    void start(void);

    /**
     * stops listening for new connections
     *
     * @param wait_until_finished if true, blocks until all pending connections have closed
     */
    void stop(bool wait_until_finished = false);
    
    /// the calling thread will sleep until the server has stopped listening for connections
    void join(void);
    
    /**
     * configures server for SSL using a PEM-encoded RSA private key file
     *
     * @param pem_key_file name of the file containing a PEM-encoded private key
     */
    void setSSLKeyFile(const std::string& pem_key_file);

    /// returns the number of active tcp connections
    std::size_t getConnections(void) const;

    /// returns tcp port number that the server listens for connections on
    inline unsigned int getPort(void) const { return m_endpoint.port(); }
    
    /// sets tcp port number that the server listens for connections on
    inline void setPort(unsigned int p) { m_endpoint.port(p); }
    
    /// returns IP address that the server listens for connections on
    inline boost::asio::ip::address getAddress(void) const { return m_endpoint.address(); }
    
    /// sets IP address that the server listens for connections on
    inline void setAddress(const boost::asio::ip::address& addr) { m_endpoint.address(addr); }
    
    /// returns tcp endpoint that the server listens for connections on
    inline const boost::asio::ip::tcp::endpoint& getEndpoint(void) const { return m_endpoint; }
    
    /// sets tcp endpoint that the server listens for connections on
    inline void setEndpoint(const boost::asio::ip::tcp::endpoint& ep) { m_endpoint = ep; }

    /// returns true if the server uses SSL to encrypt connections
    inline bool get_ssl_flag(void) const { return m_ssl_flag; }
    
    /// sets value of SSL flag (true if the server uses SSL to encrypt connections)
    inline void setSSLFlag(bool b = true) { m_ssl_flag = b; }
    
    /// returns the SSL context for configuration
    inline connection::ssl_context_type& get_ssl_context_type(void) { return m_ssl_context; }
    
    /// returns true if the server is listening for connections
    inline bool isListening(void) const { return m_is_listening; }
    
    /// sets the logger to be used
    inline void setLogger(logger log_ptr) { m_logger = log_ptr; }
    
    /// returns the logger currently in use
    inline logger getLogger(void) { return m_logger; }
    

protected:
        
    /**
     * protected constructor so that only derived objects may be created
     * 
     * @param tcp_port port number used to listen for new connections (IPv4)
     */
    explicit server(const unsigned int tcp_port);
    
    /**
     * protected constructor so that only derived objects may be created
     * 
     * @param endpoint TCP endpoint used to listen for new connections (see ASIO docs)
     */
    explicit server(const boost::asio::ip::tcp::endpoint& endpoint);

    /**
     * protected constructor so that only derived objects may be created
     * 
     * @param sched the scheduler that will be used to manage worker threads
     * @param tcp_port port number used to listen for new connections (IPv4)
     */
    explicit server(scheduler& sched, const unsigned int tcp_port = 0);
    
    /**
     * protected constructor so that only derived objects may be created
     * 
     * @param sched the scheduler that will be used to manage worker threads
     * @param endpoint TCP endpoint used to listen for new connections (see ASIO docs)
     */
    server(scheduler& sched, const boost::asio::ip::tcp::endpoint& endpoint);
    
    /**
     * handles a new TCP connection; derived classes SHOULD override this
     * since the default behavior does nothing
     * 
     * @param tcp_conn the new TCP connection to handle
     */
    virtual void handleConnection(tcp::connection_ptr& tcp_conn) {
        tcp_conn->set_lifecycle(connection::LIFECYCLE_CLOSE); // make sure it will get closed
        tcp_conn->finish();
    }
    
    /// called before the TCP server starts listening for new connections
    virtual void beforeStarting(void) {}

    /// called after the TCP server has stopped listing for new connections
    virtual void afterStopping(void) {}
    
    /// returns an async I/O service used to schedule work
    inline boost::asio::io_service& get_io_service(void) { return m_active_scheduler.get_io_service(); }
    
    
    /// primary logging interface used by this class
    logger                  m_logger;
    
    
private:
        
    /// handles a request to stop the server
    void handleStopRequest(void);
    
    /// listens for a new connection
    void listen(void);

    /**
     * handles new connections (checks if there was an accept error)
     *
     * @param tcp_conn the new TCP connection (if no error occurred)
     * @param accept_error true if an error occurred while accepting connections
     */
    void handleAccept(tcp::connection_ptr& tcp_conn,
                      const boost::system::error_code& accept_error);

    /**
     * handles new connections following an SSL handshake (checks for errors)
     *
     * @param tcp_conn the new TCP connection (if no error occurred)
     * @param handshake_error true if an error occurred during the SSL handshake
     */
    void handleSSLHandshake(tcp::connection_ptr& tcp_conn,
                            const boost::system::error_code& handshake_error);
    
    /// This will be called by connection::finish() after a server has
    /// finished handling a connection.  If the keep_alive flag is true,
    /// it will call handleConnection(); otherwise, it will close the
    /// connection and remove it from the server's management pool
    void finishConnection(tcp::connection_ptr& tcp_conn);
    
    /// prunes orphaned connections that did not close cleanly
    /// and returns the remaining number of connections in the pool
    std::size_t pruneConnections(void);
    
    /// data type for a pool of TCP connections
    typedef std::set<tcp::connection_ptr>      ConnectionPool;
    
    
    /// the default scheduler object used to manage worker threads
    single_service_scheduler              m_default_scheduler;

    /// reference to the active scheduler object used to manage worker threads
    scheduler &                         m_active_scheduler;
    
    /// manages async TCP connections
    boost::asio::ip::tcp::acceptor          m_tcp_acceptor;

    /// context used for SSL configuration
    connection::ssl_context_type               m_ssl_context;
        
    /// condition triggered when the server has stopped listening for connections
    boost::condition                        m_server_has_stopped;

    /// condition triggered when the connection pool is empty
    boost::condition                        m_no_more_connections;

    /// pool of active connections associated with this server 
    ConnectionPool                          m_conn_pool;

    /// tcp endpoint used to listen for new connections
    boost::asio::ip::tcp::endpoint          m_endpoint;

    /// true if the server uses SSL to encrypt connections
    bool                                    m_ssl_flag;

    /// set to true when the server is listening for new connections
    bool                                    m_is_listening;

    /// mutex to make class thread-safe
    mutable boost::mutex                    m_mutex;
};


/// data type for a server pointer
typedef boost::shared_ptr<server>    server_ptr;


}   // end namespace tcp
}   // end namespace pion

#endif
