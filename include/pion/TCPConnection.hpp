// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_TCPCONNECTION_HEADER__
#define __PION_TCPCONNECTION_HEADER__

#ifdef PION_HAVE_SSL
	#include <boost/asio/ssl.hpp>
#endif

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/function.hpp>
#include <pion/net/PionConfig.hpp>
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

	/// data type for a function that handles TCP connection objects
	typedef boost::function1<void, boost::shared_ptr<TCPConnection> >	ConnectionHandler;
	
	/// data type for an I/O read buffer
	typedef boost::array<char, 8192>		ReadBuffer;
	
	/// data type for a socket connection
	typedef boost::asio::ip::tcp::socket	Socket;

#ifdef PION_HAVE_SSL
	/// data type for an SSL socket connection
	typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket>	SSLSocket;

	/// data type for SSL configuration context
	typedef boost::asio::ssl::context								SSLContext;
#else
	typedef Socket	SSLSocket;
	typedef int		SSLContext;
#endif

	/// data type for the connection's lifecycle state
	enum LifecycleType {
		LIFECYCLE_CLOSE, LIFECYCLE_KEEPALIVE, LIFECYCLE_PIPELINED
	};

	
	/**
	 * creates new TCPConnection objects
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

	/// virtual destructor
	virtual ~TCPConnection() { close(); }

	/// closes the tcp socket
	inline void close(void) {
#ifdef PION_HAVE_SSL
		if (getSSLFlag())
			m_ssl_socket.lowest_layer().close();
		else 
#endif
			m_tcp_socket.close();
	}

	/**
	 * accepts a new tcp connection
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
	 * performs SSL handshake for a new connection (server side)
	 *
	 * @param handler called after the ssl handshake has completed
	 *
	 * @see boost::asio::ssl::stream::async_handshake()
	 */
	template <typename SSLHandshakeHandler>
	inline void ssl_handshake_server(SSLHandshakeHandler handler) {
#ifdef PION_HAVE_SSL
		if (getSSLFlag())
			m_ssl_socket.async_handshake(boost::asio::ssl::stream_base::server,
										 handler);
#endif
	}
	
	/**
	 * reads some data into the connection's read buffer
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
	 * reads data into the connection's read buffer until completion_condition is met
	 *
	 * @param completition_condition determines if the read operation is complete
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
	 * reads data from the connection until completion_condition is met
	 *
	 * @param buffers one or more buffers into which the data will be read
	 * @param completition_condition determines if the read operation is complete
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
	 * writes data to the connection
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
	
	
	/// This function must be called when a server has finished handling
	/// the connection
	inline void finish(void) { m_finished_handler(shared_from_this()); }

	/// returns true if the connection is encrypted using SSL
	inline bool getSSLFlag(void) const { return m_ssl_flag; }

	/// sets the lifecycle type for the connection
	inline void setLifecycle(LifecycleType t) { m_lifecycle = t; }
	
	/// returns the lifecycle type for the connection
	inline bool getLifecycle(void) const { return m_lifecycle; }
	
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

	/// returns the client's IP address
	inline boost::asio::ip::address getRemoteIp(void) const {
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
		return remote_endpoint.address();
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
		m_ssl_socket(io_service, ssl_context), m_ssl_flag(ssl_flag),
#else
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
