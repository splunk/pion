// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_TCP_CONNECTION_HEADER__
#define __PION_TCP_CONNECTION_HEADER__

#ifdef PION_HAVE_SSL
    #ifdef PION_XCODE
        // ignore openssl warnings if building with XCode
        #pragma GCC system_header
    #endif
    #include <boost/asio/ssl.hpp>
#endif

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/function.hpp>
#include <boost/function/function1.hpp>
#include <pion/config.hpp>
#include <string>


namespace pion {    // begin namespace pion
namespace tcp {     // begin namespace tcp


///
/// connection: represents a single tcp connection
/// 
class connection :
    public boost::enable_shared_from_this<connection>,
    private boost::noncopyable
{
public:

    /// data type for the connection's lifecycle state
    enum lifecycle_type {
        LIFECYCLE_CLOSE, LIFECYCLE_KEEPALIVE, LIFECYCLE_PIPELINED
    };
    
    /// size of the read buffer
    enum { READ_BUFFER_SIZE = 8192 };
    
    /// data type for a function that handles TCP connection objects
    typedef boost::function1<void, boost::shared_ptr<connection> >   connection_handler;
    
    /// data type for an I/O read buffer
    typedef boost::array<char, READ_BUFFER_SIZE>    read_buffer_type;
    
    /// data type for a socket connection
    typedef boost::asio::ip::tcp::socket            socket_type;

#ifdef PION_HAVE_SSL
    /// data type for an SSL socket connection
    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket>  ssl_socket_type;

    /// data type for SSL configuration context
    typedef boost::asio::ssl::context                               ssl_context_type;
#else
    class ssl_socket_type {
    public:
        ssl_socket_type(boost::asio::io_service& io_service) : m_socket(io_service) {}
        inline socket_type& next_layer(void) { return m_socket; }
        inline const socket_type& next_layer(void) const { return m_socket; }
        inline socket_type::lowest_layer_type& lowest_layer(void) { return m_socket.lowest_layer(); }
        inline const socket_type::lowest_layer_type& lowest_layer(void) const { return m_socket.lowest_layer(); }
        inline void shutdown(void) {}
    private:
        socket_type  m_socket;
    };
    typedef int     ssl_context_type;
#endif

    
    /**
     * creates new shared connection objects
     *
     * @param io_service asio service associated with the connection
     * @param ssl_context asio ssl context associated with the connection
     * @param ssl_flag if true then the connection will be encrypted using SSL 
     * @param finished_handler function called when a server has finished
     *                         handling the connection
     */
    static inline boost::shared_ptr<connection> create(boost::asio::io_service& io_service,
                                                          ssl_context_type& ssl_context,
                                                          const bool ssl_flag,
                                                          connection_handler finished_handler)
    {
        return boost::shared_ptr<connection>(new connection(io_service, ssl_context,
                                                                  ssl_flag, finished_handler));
    }
    
    /**
     * creates a new connection object
     *
     * @param io_service asio service associated with the connection
     * @param ssl_flag if true then the connection will be encrypted using SSL 
     */
    explicit connection(boost::asio::io_service& io_service, const bool ssl_flag = false)
        :
#ifdef PION_HAVE_SSL
        m_ssl_context(io_service, boost::asio::ssl::context::sslv23),
        m_ssl_socket(io_service, m_ssl_context),
        m_ssl_flag(ssl_flag),
#else
        m_ssl_context(0),
        m_ssl_socket(io_service),
        m_ssl_flag(false),
#endif
        m_lifecycle(LIFECYCLE_CLOSE)
    {
        save_read_pos(NULL, NULL);
    }
    
    /**
     * creates a new connection object for SSL
     *
     * @param io_service asio service associated with the connection
     * @param ssl_context asio ssl context associated with the connection
     */
    connection(boost::asio::io_service& io_service, ssl_context_type& ssl_context)
        :
#ifdef PION_HAVE_SSL
        m_ssl_context(io_service, boost::asio::ssl::context::sslv23),
        m_ssl_socket(io_service, ssl_context), m_ssl_flag(true),
#else
        m_ssl_context(0),
        m_ssl_socket(io_service), m_ssl_flag(false), 
#endif
        m_lifecycle(LIFECYCLE_CLOSE)
    {
        save_read_pos(NULL, NULL);
    }
    
    /// returns true if the connection is currently open
    inline bool is_open(void) const {
        return const_cast<ssl_socket_type&>(m_ssl_socket).lowest_layer().is_open();
    }
    
    /// closes the tcp socket and cancels any pending asynchronous operations
    inline void close(void) {
        if (is_open()) {
            try {
                // shutting down SSL will wait forever for a response from the remote end,
                // which causes it to hang indefinitely if the other end died unexpectedly
                // if (get_ssl_flag()) m_ssl_socket.shutdown();
                m_ssl_socket.next_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both);
            } catch (...) {}
            boost::system::error_code ec;
            m_ssl_socket.next_layer().close(ec);
        }
    }

	/// cancels any asynchronous operations pending on the socket
	/// WARNING: Use close instead; basic_socket::cancel doesn't work on Windows pre-Vista!
    inline void cancel(void) {
        m_ssl_socket.next_layer().cancel();
    }
    
    /// virtual destructor
    virtual ~connection() { close(); }
    
    /**
     * asynchronously accepts a new tcp connection
     *
     * @param tcp_acceptor object used to accept new connections
     * @param handler called after a new connection has been accepted
     *
     * @see boost::asio::basic_socket_acceptor::async_accept()
     */
    template <typename AcceptHandler>
    inline void async_accept(boost::asio::ip::tcp::acceptor& tcp_acceptor,
                             AcceptHandler handler)
    {
        tcp_acceptor.async_accept(m_ssl_socket.lowest_layer(), handler);
    }

    /**
     * accepts a new tcp connection (blocks until established)
     *
     * @param tcp_acceptor object used to accept new connections
     * @return boost::system::error_code contains error code if the connection fails
     *
     * @see boost::asio::basic_socket_acceptor::accept()
     */
    inline boost::system::error_code accept(boost::asio::ip::tcp::acceptor& tcp_acceptor)
    {
        boost::system::error_code ec;
        tcp_acceptor.accept(m_ssl_socket.lowest_layer(), ec);
        return ec;
    }
    
    /**
     * asynchronously connects to a remote endpoint
     *
     * @param tcp_endpoint remote endpoint to connect to
     * @param handler called after a new connection has been established
     *
     * @see boost::asio::basic_socket_acceptor::async_connect()
     */
    template <typename ConnectHandler>
    inline void async_connect(boost::asio::ip::tcp::endpoint& tcp_endpoint,
                              ConnectHandler handler)
    {
        m_ssl_socket.lowest_layer().async_connect(tcp_endpoint, handler);
    }

    /**
     * asynchronously connects to a (IPv4) remote endpoint
     *
     * @param remote_addr remote IP address (v4) to connect to
     * @param remote_port remote port number to connect to
     * @param handler called after a new connection has been established
     *
     * @see boost::asio::basic_socket_acceptor::async_connect()
     */
    template <typename ConnectHandler>
    inline void async_connect(const boost::asio::ip::address& remote_addr,
                              const unsigned int remote_port,
                              ConnectHandler handler)
    {
        boost::asio::ip::tcp::endpoint tcp_endpoint(remote_addr, remote_port);
        async_connect(tcp_endpoint, handler);
    }
    
    /**
     * connects to a remote endpoint (blocks until established)
     *
     * @param tcp_endpoint remote endpoint to connect to
     * @return boost::system::error_code contains error code if the connection fails
     *
     * @see boost::asio::basic_socket_acceptor::connect()
     */
    inline boost::system::error_code connect(boost::asio::ip::tcp::endpoint& tcp_endpoint)
    {
        boost::system::error_code ec;
        m_ssl_socket.lowest_layer().connect(tcp_endpoint, ec);
        return ec;
    }

    /**
     * connects to a (IPv4) remote endpoint (blocks until established)
     *
     * @param remote_addr remote IP address (v4) to connect to
     * @param remote_port remote port number to connect to
     * @return boost::system::error_code contains error code if the connection fails
     *
     * @see boost::asio::basic_socket_acceptor::connect()
     */
    inline boost::system::error_code connect(const boost::asio::ip::address& remote_addr,
                                             const unsigned int remote_port)
    {
        boost::asio::ip::tcp::endpoint tcp_endpoint(remote_addr, remote_port);
        return connect(tcp_endpoint);
    }
    
    /**
     * connects to a remote endpoint with hostname lookup
     *
     * @param remote_server hostname of the remote server to connect to
     * @param remote_port remote port number to connect to
     * @return boost::system::error_code contains error code if the connection fails
     *
     * @see boost::asio::basic_socket_acceptor::connect()
     */
    inline boost::system::error_code connect(const std::string& remote_server,
                                             const unsigned int remote_port)
    {
        // query a list of matching endpoints
        boost::system::error_code ec;
        boost::asio::ip::tcp::resolver resolver(m_ssl_socket.lowest_layer().get_io_service());
        boost::asio::ip::tcp::resolver::query query(remote_server,
            boost::lexical_cast<std::string>(remote_port),
            boost::asio::ip::tcp::resolver::query::numeric_service);
        boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query, ec);
        if (ec)
            return ec;

        // try each one until we are successful
        ec = boost::asio::error::host_not_found;
        boost::asio::ip::tcp::resolver::iterator end;
        while (ec && endpoint_iterator != end) {
            boost::asio::ip::tcp::endpoint ep(endpoint_iterator->endpoint());
            ++endpoint_iterator;
            ec = connect(ep);
            if (ec)
                close();
        }

        return ec;
    }
    
    /**
     * asynchronously performs client-side SSL handshake for a new connection
     *
     * @param handler called after the ssl handshake has completed
     *
     * @see boost::asio::ssl::stream::async_handshake()
     */
    template <typename SSLHandshakeHandler>
    inline void async_handshake_client(SSLHandshakeHandler handler) {
#ifdef PION_HAVE_SSL
        m_ssl_socket.async_handshake(boost::asio::ssl::stream_base::client, handler);
        m_ssl_flag = true;
#endif
    }

    /**
     * asynchronously performs server-side SSL handshake for a new connection
     *
     * @param handler called after the ssl handshake has completed
     *
     * @see boost::asio::ssl::stream::async_handshake()
     */
    template <typename SSLHandshakeHandler>
    inline void async_handshake_server(SSLHandshakeHandler handler) {
#ifdef PION_HAVE_SSL
        m_ssl_socket.async_handshake(boost::asio::ssl::stream_base::server, handler);
        m_ssl_flag = true;
#endif
    }
    
    /**
     * performs client-side SSL handshake for a new connection (blocks until finished)
     *
     * @return boost::system::error_code contains error code if the connection fails
     *
     * @see boost::asio::ssl::stream::handshake()
     */
    inline boost::system::error_code handshake_client(void) {
        boost::system::error_code ec;
#ifdef PION_HAVE_SSL
        m_ssl_socket.handshake(boost::asio::ssl::stream_base::client, ec);
        m_ssl_flag = true;
#endif
        return ec;
    }

    /**
     * performs server-side SSL handshake for a new connection (blocks until finished)
     *
     * @return boost::system::error_code contains error code if the connection fails
     *
     * @see boost::asio::ssl::stream::handshake()
     */
    inline boost::system::error_code handshake_server(void) {
        boost::system::error_code ec;
#ifdef PION_HAVE_SSL
        m_ssl_socket.handshake(boost::asio::ssl::stream_base::server, ec);
        m_ssl_flag = true;
#endif
        return ec;
    }
    
    /**
     * asynchronously reads some data into the connection's read buffer 
     *
     * @param handler called after the read operation has completed
     *
     * @see boost::asio::basic_stream_socket::async_read_some()
     */
    template <typename ReadHandler>
    inline void async_read_some(ReadHandler handler) {
#ifdef PION_HAVE_SSL
        if (get_ssl_flag())
            m_ssl_socket.async_read_some(boost::asio::buffer(m_read_buffer),
                                         handler);
        else
#endif      
            m_ssl_socket.next_layer().async_read_some(boost::asio::buffer(m_read_buffer),
                                         handler);
    }
    
    /**
     * asynchronously reads some data into the connection's read buffer 
     *
     * @param read_buffer the buffer to read data into
     * @param handler called after the read operation has completed
     *
     * @see boost::asio::basic_stream_socket::async_read_some()
     */
    template <typename ReadBufferType, typename ReadHandler>
    inline void async_read_some(ReadBufferType read_buffer,
                                ReadHandler handler) {
#ifdef PION_HAVE_SSL
        if (get_ssl_flag())
            m_ssl_socket.async_read_some(read_buffer, handler);
        else
#endif      
            m_ssl_socket.next_layer().async_read_some(read_buffer, handler);
    }
    
    /**
     * reads some data into the connection's read buffer (blocks until finished)
     *
     * @param ec contains error code if the read fails
     * @return std::size_t number of bytes read
     *
     * @see boost::asio::basic_stream_socket::read_some()
     */
    inline std::size_t read_some(boost::system::error_code& ec) {
#ifdef PION_HAVE_SSL
        if (get_ssl_flag())
            return m_ssl_socket.read_some(boost::asio::buffer(m_read_buffer), ec);
        else
#endif      
            return m_ssl_socket.next_layer().read_some(boost::asio::buffer(m_read_buffer), ec);
    }
    
    /**
     * reads some data into the connection's read buffer (blocks until finished)
     *
     * @param read_buffer the buffer to read data into
     * @param ec contains error code if the read fails
     * @return std::size_t number of bytes read
     *
     * @see boost::asio::basic_stream_socket::read_some()
     */
    template <typename ReadBufferType>
    inline std::size_t read_some(ReadBufferType read_buffer,
                                 boost::system::error_code& ec)
    {
#ifdef PION_HAVE_SSL
        if (get_ssl_flag())
            return m_ssl_socket.read_some(read_buffer, ec);
        else
#endif      
            return m_ssl_socket.next_layer().read_some(read_buffer, ec);
    }
    
    /**
     * asynchronously reads data into the connection's read buffer until
     * completion_condition is met
     *
     * @param completion_condition determines if the read operation is complete
     * @param handler called after the read operation has completed
     *
     * @see boost::asio::async_read()
     */
    template <typename CompletionCondition, typename ReadHandler>
    inline void async_read(CompletionCondition completion_condition,
                           ReadHandler handler)
    {
#ifdef PION_HAVE_SSL
        if (get_ssl_flag())
            boost::asio::async_read(m_ssl_socket, boost::asio::buffer(m_read_buffer),
                                    completion_condition, handler);
        else
#endif      
            boost::asio::async_read(m_ssl_socket.next_layer(), boost::asio::buffer(m_read_buffer),
                                    completion_condition, handler);
    }
            
    /**
     * asynchronously reads data from the connection until completion_condition
     * is met
     *
     * @param buffers one or more buffers into which the data will be read
     * @param completion_condition determines if the read operation is complete
     * @param handler called after the read operation has completed
     *
     * @see boost::asio::async_read()
     */
    template <typename MutableBufferSequence, typename CompletionCondition, typename ReadHandler>
    inline void async_read(const MutableBufferSequence& buffers,
                           CompletionCondition completion_condition,
                           ReadHandler handler)
    {
#ifdef PION_HAVE_SSL
        if (get_ssl_flag())
            boost::asio::async_read(m_ssl_socket, buffers,
                                    completion_condition, handler);
        else
#endif      
            boost::asio::async_read(m_ssl_socket.next_layer(), buffers,
                                    completion_condition, handler);
    }
    
    /**
     * reads data into the connection's read buffer until completion_condition
     * is met (blocks until finished)
     *
     * @param completion_condition determines if the read operation is complete
     * @param ec contains error code if the read fails
     * @return std::size_t number of bytes read
     *
     * @see boost::asio::read()
     */
    template <typename CompletionCondition>
    inline std::size_t read(CompletionCondition completion_condition,
                            boost::system::error_code& ec)
    {
#ifdef PION_HAVE_SSL
        if (get_ssl_flag())
            return boost::asio::async_read(m_ssl_socket, boost::asio::buffer(m_read_buffer),
                                           completion_condition, ec);
        else
#endif      
            return boost::asio::async_read(m_ssl_socket.next_layer(), boost::asio::buffer(m_read_buffer),
                                           completion_condition, ec);
    }
    
    /**
     * reads data from the connection until completion_condition is met
     * (blocks until finished)
     *
     * @param buffers one or more buffers into which the data will be read
     * @param completion_condition determines if the read operation is complete
     * @param ec contains error code if the read fails
     * @return std::size_t number of bytes read
     *
     * @see boost::asio::read()
     */
    template <typename MutableBufferSequence, typename CompletionCondition>
    inline std::size_t read(const MutableBufferSequence& buffers,
                            CompletionCondition completion_condition,
                            boost::system::error_code& ec)
    {
#ifdef PION_HAVE_SSL
        if (get_ssl_flag())
            return boost::asio::read(m_ssl_socket, buffers,
                                     completion_condition, ec);
        else
#endif      
            return boost::asio::read(m_ssl_socket.next_layer(), buffers,
                                     completion_condition, ec);
    }
    
    /**
     * asynchronously writes data to the connection
     *
     * @param buffers one or more buffers containing the data to be written
     * @param handler called after the data has been written
     *
     * @see boost::asio::async_write()
     */
    template <typename ConstBufferSequence, typename write_handler_t>
    inline void async_write(const ConstBufferSequence& buffers, write_handler_t handler) {
#ifdef PION_HAVE_SSL
        if (get_ssl_flag())
            boost::asio::async_write(m_ssl_socket, buffers, handler);
        else
#endif      
            boost::asio::async_write(m_ssl_socket.next_layer(), buffers, handler);
    }   
        
    /**
     * writes data to the connection (blocks until finished)
     *
     * @param buffers one or more buffers containing the data to be written
     * @param ec contains error code if the write fails
     * @return std::size_t number of bytes written
     *
     * @see boost::asio::write()
     */
    template <typename ConstBufferSequence>
    inline std::size_t write(const ConstBufferSequence& buffers,
                             boost::system::error_code& ec)
    {
#ifdef PION_HAVE_SSL
        if (get_ssl_flag())
            return boost::asio::write(m_ssl_socket, buffers,
                                      boost::asio::transfer_all(), ec);
        else
#endif      
            return boost::asio::write(m_ssl_socket.next_layer(), buffers,
                                      boost::asio::transfer_all(), ec);
    }   
    
    
    /// This function should be called when a server has finished handling
    /// the connection
    inline void finish(void) { if (m_finished_handler) m_finished_handler(shared_from_this()); }

    /// returns true if the connection is encrypted using SSL
    inline bool get_ssl_flag(void) const { return m_ssl_flag; }

    /// sets the lifecycle type for the connection
    inline void set_lifecycle(lifecycle_type t) { m_lifecycle = t; }
    
    /// returns the lifecycle type for the connection
    inline lifecycle_type get_lifecycle(void) const { return m_lifecycle; }
    
    /// returns true if the connection should be kept alive
    inline bool get_keep_alive(void) const { return m_lifecycle != LIFECYCLE_CLOSE; }
    
    /// returns true if the HTTP requests are pipelined
    inline bool get_pipelined(void) const { return m_lifecycle == LIFECYCLE_PIPELINED; }

    /// returns the buffer used for reading data from the TCP connection
    inline read_buffer_type& get_read_buffer(void) { return m_read_buffer; }
    
    /**
     * saves a read position bookmark
     *
     * @param read_ptr points to the next character to be consumed in the read_buffer
     * @param read_end_ptr points to the end of the read_buffer (last byte + 1)
     */
    inline void save_read_pos(const char *read_ptr, const char *read_end_ptr) {
        m_read_position.first = read_ptr;
        m_read_position.second = read_end_ptr;
    }
    
    /**
     * loads a read position bookmark
     *
     * @param read_ptr points to the next character to be consumed in the read_buffer
     * @param read_end_ptr points to the end of the read_buffer (last byte + 1)
     */
    inline void load_read_pos(const char *&read_ptr, const char *&read_end_ptr) const {
        read_ptr = m_read_position.first;
        read_end_ptr = m_read_position.second;
    }

    /// returns an ASIO endpoint for the client connection
    inline boost::asio::ip::tcp::endpoint get_remote_endpoint(void) const {
        boost::asio::ip::tcp::endpoint remote_endpoint;
        try {
            // const_cast is required since lowest_layer() is only defined non-const in asio
            remote_endpoint = const_cast<ssl_socket_type&>(m_ssl_socket).lowest_layer().remote_endpoint();
        } catch (boost::system::system_error& /* e */) {
            // do nothing
        }
        return remote_endpoint;
    }

    /// returns the client's IP address
    inline boost::asio::ip::address get_remote_ip(void) const {
        return get_remote_endpoint().address();
    }

    /// returns the client's port number
    inline unsigned short get_remote_port(void) const {
        return get_remote_endpoint().port();
    }
    
    /// returns reference to the io_service used for async operations
    inline boost::asio::io_service& get_io_service(void) {
        return m_ssl_socket.lowest_layer().get_io_service();
    }

    /// returns non-const reference to underlying TCP socket object
    inline socket_type& get_socket(void) { return m_ssl_socket.next_layer(); }
    
    /// returns non-const reference to underlying SSL socket object
    inline ssl_socket_type& get_ssl_socket(void) { return m_ssl_socket; }

    /// returns const reference to underlying TCP socket object
    inline const socket_type& get_socket(void) const { return const_cast<ssl_socket_type&>(m_ssl_socket).next_layer(); }
    
    /// returns const reference to underlying SSL socket object
    inline const ssl_socket_type& get_ssl_socket(void) const { return m_ssl_socket; }

    
protected:
        
    /**
     * protected constructor restricts creation of objects (use create())
     *
     * @param io_service asio service associated with the connection
     * @param ssl_context asio ssl context associated with the connection
     * @param ssl_flag if true then the connection will be encrypted using SSL 
     * @param finished_handler function called when a server has finished
     *                         handling the connection
     */
    connection(boost::asio::io_service& io_service,
                  ssl_context_type& ssl_context,
                  const bool ssl_flag,
                  connection_handler finished_handler)
        :
#ifdef PION_HAVE_SSL
        m_ssl_context(io_service, boost::asio::ssl::context::sslv23),
        m_ssl_socket(io_service, ssl_context), m_ssl_flag(ssl_flag),
#else
        m_ssl_context(0),
        m_ssl_socket(io_service), m_ssl_flag(false), 
#endif
        m_lifecycle(LIFECYCLE_CLOSE),
        m_finished_handler(finished_handler)
    {
        save_read_pos(NULL, NULL);
    }
    

private:

    /// data type for a read position bookmark
    typedef std::pair<const char*, const char*>     read_pos_type;

    
    /// context object for the SSL connection socket
    ssl_context_type        m_ssl_context;

    /// SSL connection socket
    ssl_socket_type         m_ssl_socket;

    /// true if the connection is encrypted using SSL
    bool                    m_ssl_flag;

    /// buffer used for reading data from the TCP connection
    read_buffer_type        m_read_buffer;
    
    /// saved read position bookmark
    read_pos_type           m_read_position;
    
    /// lifecycle state for the connection
    lifecycle_type          m_lifecycle;

    /// function called when a server has finished handling the connection
    connection_handler      m_finished_handler;
};


/// data type for a connection pointer
typedef boost::shared_ptr<connection>    connection_ptr;


}   // end namespace tcp
}   // end namespace pion

#endif
