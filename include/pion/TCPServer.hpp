// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_TCPSERVER_HEADER__
#define __PION_TCPSERVER_HEADER__

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/asio.hpp>
#include <pion/net/PionConfig.hpp>
#include <pion/net/PionLogger.hpp>
#include <pion/net/TCPConnection.hpp>
#include <set>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)

///
/// TCPServer: a multi-threaded, asynchronous TCP server
/// 
class PION_LIBRARY_API TCPServer :
	private boost::noncopyable
{
public:

	/// default destructor
	virtual ~TCPServer() { if (m_is_listening) stop(); }
	
	/// starts listening for new connections
	void start(void);

	/// stops listening for new connections
	void stop(void);

	/// returns true if the server uses SSL to encrypt connections
	inline bool getSSLFlag(void) const { return m_ssl_flag; }
	
	/// sets value of SSL flag (true if the server uses SSL to encrypt connections)
	inline void setSSLFlag(bool b = true) { m_ssl_flag = b; }
	
	/// returns the SSL context for configuration
	inline TCPConnection::SSLContext& getSSLContext(void) { return m_ssl_context; }
	
	/// returns tcp port number server listens for connections on
	inline unsigned int getPort(void) const { return m_tcp_port; }

	/// sets the logger to be used
	inline void setLogger(PionLogger log_ptr) { m_logger = log_ptr; }
	
	/// returns the logger currently in use
	inline PionLogger getLogger(void) { return m_logger; }
	

protected:
		
	/**
	 * protect constructor so that only derived objects may be created
	 * 
	 * @param tcp_port port number used to listen for new connections
	 * @param ssl_flag if true, the server will use SSL to encrypt connections
	 */
	explicit TCPServer(const unsigned int tcp_port);
	
	/**
	 * handles a new TCP connection; derived classes SHOULD override this
	 * since the default behavior does nothing
	 * 
	 * @param tcp_conn the new TCP connection to handle
	 */
	virtual void handleConnection(TCPConnectionPtr& tcp_conn) {
		tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_CLOSE);	// make sure it will get closed
		tcp_conn->finish();
	}
	
	/// called before the TCP server starts listening for new connections
	virtual void beforeStarting(void) {}

	/// called after the TCP server has stopped listing for new connections
	virtual void afterStopping(void) {}
	
	
	/// primary logging interface used by this class
	PionLogger								m_logger;
	
	
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
	void handleAccept(TCPConnectionPtr& tcp_conn,
					  const boost::system::error_code& accept_error);

	/**
	 * handles new connections following an SSL handshake (checks for errors)
	 *
	 * @param tcp_conn the new TCP connection (if no error occurred)
	 * @param handshake_error true if an error occurred during the SSL handshake
	 */
	void handleSSLHandshake(TCPConnectionPtr& tcp_conn,
							const boost::system::error_code& handshake_error);
	
	/// This will be called by TCPConnection::finish() after a server has
	/// finished handling a connection.  If the keep_alive flag is true,
	/// it will call handleConnection(); otherwise, it will close the
	/// connection and remove it from the server's management pool
	void finishConnection(TCPConnectionPtr& tcp_conn);

	
	/// data type for a pool of TCP connections
	typedef std::set<TCPConnectionPtr>		ConnectionPool;

	/// mutex to make class thread-safe
	boost::mutex							m_mutex;

	/// manages async TCP connections
	boost::asio::ip::tcp::acceptor			m_tcp_acceptor;

	/// context used for SSL configuration
	TCPConnection::SSLContext				m_ssl_context;
		
	/// pool of active connections associated with this server 
	ConnectionPool							m_conn_pool;

	/// tcp port number server listens for connections on
	const unsigned int						m_tcp_port;

	/// true if the server uses SSL to encrypt connections
	bool									m_ssl_flag;

	/// set to true when the server is listening for new connections
	bool									m_is_listening;
};


/// data type for a TCPServer pointer
typedef boost::shared_ptr<TCPServer>	TCPServerPtr;


}	// end namespace net
}	// end namespace pion

#endif
