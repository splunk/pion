// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_TCPCONNECTION_HEADER__
#define __PION_TCPCONNECTION_HEADER__

#ifdef PION_HAVE_SSL
	#ifdef PION_XCODE
		// ignore openssl warnings if building with XCode
		#pragma GCC system_header
	#endif
	#include <boost/asio/ssl.hpp>
	#if defined _MSC_VER
		#pragma comment(lib, "ssleay32")
		#pragma comment(lib, "libeay32")
	#endif 
#endif

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/function.hpp>
#include <pion/PionConfig.hpp>
#include <string>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)

///
/// TCPConnection: represents a single tcp connection
/// 
class TCPConnection :
	public boost::enable_shared_from_this<TCPConnection>,
	private boost::noncopyable
{
public:

	/// data type for the connection's lifecycle state
	enum LifecycleType {
		LIFECYCLE_CLOSE, LIFECYCLE_KEEPALIVE, LIFECYCLE_PIPELINED
	};
	
	/// size of the read buffer
	enum { READ_BUFFER_SIZE = 8192 };
	
	/// data type for a function that handles TCP connection objects
	typedef boost::function1<void, boost::shared_ptr<TCPConnection> >	ConnectionHandler;
	
	/// data type for an I/O read buffer
	typedef boost::array<char, READ_BUFFER_SIZE>	ReadBuffer;
	
	/// data type for a socket connection
	typedef boost::asio::ip::tcp::socket			Socket;

#ifdef PION_HAVE_SSL
	/// data type for an SSL socket connection
	typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket>	SSLSocket;

	/// data type for SSL configuration context
	typedef boost::asio::ssl::context								SSLContext;
#else
	typedef Socket	SSLSocket;
	typedef int		SSLContext;
#endif

	
	/**
	 * creates new shared TCPConnection objects
	 *
	 * @param io_service asio service associated with the connection
	 * @param ssl_context asio ssl context associated with the connection
	 * @param ssl_flag if true then the connection will be encrypted using SSL 
	 * @param finished_handler function called when a server has finished
	 *                         handling	the connection
	 */
	static inline boost::shared_ptr<TCPConnection> create(boost::asio::io_service& io_service,
														  SSLContext& ssl_context,
														  const bool ssl_flag,
														  ConnectionHandler finished_handler)
	{
		return boost::shared_ptr<TCPConnection>(new TCPConnection(io_service, ssl_context,
																  ssl_flag, finished_handler));
	}
	
	/**
	 * creates a new TCPConnection object
	 *
	 * @param io_service asio service associated with the connection
	 * @param ssl_flag if true then the connection will be encrypted using SSL 
	 */
	explicit TCPConnection(boost::asio::io_service& io_service, const bool ssl_flag = false)
		: m_tcp_socket(io_service),
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
		saveReadPosition(NULL, NULL);
	}
	
	/**
	 * creates a new TCPConnection object for SSL
	 *
	 * @param io_service asio service associated with the connection
	 * @param ssl_context asio ssl context associated with the connection
	 */
	TCPConnection(boost::asio::io_service& io_service, SSLContext& ssl_context)
		: m_tcp_socket(io_service),
#ifdef PION_HAVE_SSL
		m_ssl_context(io_service, boost::asio::ssl::context::sslv23),
		m_ssl_socket(io_service, ssl_context), m_ssl_flag(true),
#else
		m_ssl_context(0),
		m_ssl_socket(io_service), m_ssl_flag(false), 
#endif
		m_lifecycle(LIFECYCLE_CLOSE)
	{
		saveReadPosition(NULL, NULL);
	}
	
	/// returns true if the connection is currently open
	inline bool is_open(void) const {
#ifdef PION_HAVE_SSL
		if (getSSLFlag())
			return const_cast<SSLSocket&>(m_ssl_socket).lowest_layer().is_open();
		else 
#endif
			return m_tcp_socket.is_open();
	}
	
	/// closes the tcp socket and cancels any pending asynchronous operations
	inline void close(void) {
#ifdef PION_HAVE_SSL
		if (getSSLFlag()) {
			if (m_ssl_socket.lowest_layer().is_open())
				m_ssl_socket.lowest_layer().close();
		} else
#endif
		{
			if (m_tcp_socket.is_open())
				m_tcp_socket.close();
		}
	}

	/// cancels any asynchronous operations pending on the socket
	inline void cancel(void) {
#ifdef PION_HAVE_SSL
		if (getSSLFlag())
			m_ssl_socket.lowest_layer().cancel();
		else
#endif
			m_tcp_socket.cancel();
	}
	
	/// virtual destructor
	virtual ~TCPConnection() { close(); }
	
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
#ifdef PION_HAVE_SSL
		if (getSSLFlag())
			tcp_acceptor.async_accept(m_ssl_socket.lowest_layer(), handler);
		else
#endif		
			tcp_acceptor.async_accept(m_tcp_socket, handler);
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
#ifdef PION_HAVE_SSL
		if (getSSLFlag())
			tcp_acceptor.accept(m_ssl_socket.lowest_layer(), ec);
		else
#endif		
			tcp_acceptor.accept(m_tcp_socket, ec);
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
#ifdef PION_HAVE_SSL
		if (getSSLFlag())
			m_ssl_socket.lowest_layer().async_connect(tcp_endpoint, handler);
		else
#endif		
			m_tcp_socket.async_connect(tcp_endpoint, handler);
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
#ifdef PION_HAVE_SSL
		if (getSSLFlag())
			m_ssl_socket.lowest_layer().connect(tcp_endpoint, ec);
		else
#endif		
			m_tcp_socket.connect(tcp_endpoint, ec);
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
	 * asynchronously performs client-side SSL handshake for a new connection
	 *
	 * @param handler called after the ssl handshake has completed
	 *
	 * @see boost::asio::ssl::stream::async_handshake()
	 */
	template <typename SSLHandshakeHandler>
	inline void async_handshake_client(SSLHandshakeHandler handler) {
#ifdef PION_HAVE_SSL
		if (getSSLFlag())
			m_ssl_socket.async_handshake(boost::asio::ssl::stream_base::client, handler);
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
		if (getSSLFlag())
			m_ssl_socket.async_handshake(boost::asio::ssl::stream_base::server, handler);
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
		if (getSSLFlag())
			m_ssl_socket.handshake(boost::asio::ssl::stream_base::client, ec);
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
		if (getSSLFlag())
			m_ssl_socket.handshake(boost::asio::ssl::stream_base::server, ec);
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
		if (getSSLFlag())
			m_ssl_socket.async_read_some(boost::asio::buffer(m_read_buffer),
										 handler);
		else
#endif		
			m_tcp_socket.async_read_some(boost::asio::buffer(m_read_buffer),
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
		if (getSSLFlag())
			m_ssl_socket.async_read_some(read_buffer, handler);
		else
#endif		
			m_tcp_socket.async_read_some(read_buffer, handler);
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
		if (getSSLFlag())
			return m_ssl_socket.read_some(boost::asio::buffer(m_read_buffer), ec);
		else
#endif		
			return m_tcp_socket.read_some(boost::asio::buffer(m_read_buffer), ec);
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
		if (getSSLFlag())
			return m_ssl_socket.read_some(read_buffer, ec);
		else
#endif		
			return m_tcp_socket.read_some(read_buffer, ec);
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
		if (getSSLFlag())
			boost::asio::async_read(m_ssl_socket, boost::asio::buffer(m_read_buffer),
									completion_condition, handler);
		else
#endif		
			boost::asio::async_read(m_tcp_socket, boost::asio::buffer(m_read_buffer),
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
		if (getSSLFlag())
			boost::asio::async_read(m_ssl_socket, buffers,
									completion_condition, handler);
		else
#endif		
			boost::asio::async_read(m_tcp_socket, buffers,
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
		if (getSSLFlag())
			return boost::asio::async_read(m_ssl_socket, boost::asio::buffer(m_read_buffer),
										   completion_condition, ec);
		else
#endif		
			return boost::asio::async_read(m_tcp_socket, boost::asio::buffer(m_read_buffer),
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
		if (getSSLFlag())
			return boost::asio::read(m_ssl_socket, buffers,
									 completion_condition, ec);
		else
#endif		
			return boost::asio::read(m_tcp_socket, buffers,
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
	template <typename ConstBufferSequence, typename WriteHandler>
	inline void async_write(const ConstBufferSequence& buffers, WriteHandler handler) {
#ifdef PION_HAVE_SSL
		if (getSSLFlag())
			boost::asio::async_write(m_ssl_socket, buffers, handler);
		else
#endif		
			boost::asio::async_write(m_tcp_socket, buffers, handler);
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
							 boost::system::error_code ec)
	{
#ifdef PION_HAVE_SSL
		if (getSSLFlag())
			return boost::asio::write(m_ssl_socket, buffers,
									  boost::asio::transfer_all(), ec);
		else
#endif		
			return boost::asio::write(m_tcp_socket, buffers,
									  boost::asio::transfer_all(), ec);
	}	
	
	
	/// This function should be called when a server has finished handling
	/// the connection
	inline void finish(void) { if (m_finished_handler) m_finished_handler(shared_from_this()); }

	/// returns true if the connection is encrypted using SSL
	inline bool getSSLFlag(void) const { return m_ssl_flag; }

	/// sets the lifecycle type for the connection
	inline void setLifecycle(LifecycleType t) { m_lifecycle = t; }
	
	/// returns the lifecycle type for the connection
	inline LifecycleType getLifecycle(void) const { return m_lifecycle; }
	
	/// returns true if the connection should be kept alive
	inline bool getKeepAlive(void) const { return m_lifecycle != LIFECYCLE_CLOSE; }
	
	/// returns true if the HTTP requests are pipelined
	inline bool getPipelined(void) const { return m_lifecycle == LIFECYCLE_PIPELINED; }

	/// returns the buffer used for reading data from the TCP connection
	inline ReadBuffer& getReadBuffer(void) { return m_read_buffer; }
	
	/**
	 * saves a read position bookmark
	 *
	 * @param read_ptr points to the next character to be consumed in the read_buffer
	 * @param read_end_ptr points to the end of the read_buffer (last byte + 1)
	 */
	inline void saveReadPosition(const char *read_ptr, const char *read_end_ptr) {
		m_read_position.first = read_ptr;
		m_read_position.second = read_end_ptr;
	}
	
	/**
	 * loads a read position bookmark
	 *
	 * @param read_ptr points to the next character to be consumed in the read_buffer
	 * @param read_end_ptr points to the end of the read_buffer (last byte + 1)
	 */
	inline void loadReadPosition(const char *&read_ptr, const char *&read_end_ptr) const {
		read_ptr = m_read_position.first;
		read_end_ptr = m_read_position.second;
	}

	/// returns an ASIO endpoint for the client connection
	inline boost::asio::ip::tcp::endpoint getRemoteEndpoint(void) const {
		boost::asio::ip::tcp::endpoint remote_endpoint;
		try {
#ifdef PION_HAVE_SSL
			if (getSSLFlag())
				// const_cast is required since lowest_layer() is only defined non-const in asio
				remote_endpoint = const_cast<SSLSocket&>(m_ssl_socket).lowest_layer().remote_endpoint();
			else
#endif
				remote_endpoint = m_tcp_socket.remote_endpoint();
		} catch (boost::system::system_error& /* e */) {
			// do nothing
		}
		return remote_endpoint;
	}

	/// returns the client's IP address
	inline boost::asio::ip::address getRemoteIp(void) const {
		return getRemoteEndpoint().address();
	}

	/// returns the client's port number
	inline unsigned short getRemotePort(void) const {
		return getRemoteEndpoint().port();
	}
	
	
protected:
		
	/**
	 * protected constructor restricts creation of objects (use create())
	 *
	 * @param io_service asio service associated with the connection
	 * @param ssl_context asio ssl context associated with the connection
	 * @param ssl_flag if true then the connection will be encrypted using SSL 
	 * @param finished_handler function called when a server has finished
	 *                         handling	the connection
	 */
	TCPConnection(boost::asio::io_service& io_service,
				  SSLContext& ssl_context,
				  const bool ssl_flag,
				  ConnectionHandler finished_handler)
		: m_tcp_socket(io_service),
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
		saveReadPosition(NULL, NULL);
	}
	

private:

	/// data type for a read position bookmark
	typedef std::pair<const char*, const char*>		ReadPosition;

	
	/// TCP connection socket
	Socket						m_tcp_socket;
	
	/// context object for the SSL connection socket
	SSLContext					m_ssl_context;

	/// SSL connection socket
	SSLSocket					m_ssl_socket;

	/// true if the connection is encrypted using SSL
	const bool					m_ssl_flag;

	/// buffer used for reading data from the TCP connection
	ReadBuffer					m_read_buffer;
	
	/// saved read position bookmark
	ReadPosition				m_read_position;
	
	/// lifecycle state for the connection
	LifecycleType				m_lifecycle;

	/// function called when a server has finished handling the connection
	ConnectionHandler			m_finished_handler;
};


/// data type for a TCPConnection pointer
typedef boost::shared_ptr<TCPConnection>	TCPConnectionPtr;


}	// end namespace net
}	// end namespace pion

#endif
