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

#ifndef __PION_TCPSERVER_HEADER__
#define __PION_TCPSERVER_HEADER__

#include "PionLogger.hpp"
#include "TCPProtocol.hpp"
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/asio.hpp>
#include <set>


namespace pion {	// begin namespace pion

///
/// TCPServer: a multi-threaded, asynchronous TCP server
/// 
class TCPServer : public boost::enable_shared_from_this<TCPServer>, private boost::noncopyable {
public:

	/// default destructor
	virtual ~TCPServer() { if (m_is_listening) handleStopRequest(); }
	
	/**
     * constructs server using asio service
     * 
     * @param io_service asio service associate with the server
     * @param port port number used to listen for connections
	 */
	TCPServer(boost::asio::io_service& io_service, const unsigned int port);

	/// starts listening for new connections
	void start(void);

	/// stops listening for new connections
	void stop(void);

	/// returns tcp port number server listens for connections on
	inline unsigned int getPort(void) const { return m_tcp_port; }

	/// sets the protocol handler to be used by this server
	inline void setProtocol(TCPProtocolPtr protocol) { m_protocol = protocol; }

	/// returns the protocol handler currently in use
	inline TCPProtocolPtr getProtocol(void) { return m_protocol; }
	
	/// sets the logger to be used
	inline void setLogger(log4cxx::LoggerPtr log_ptr) { m_logger = log_ptr; }
	
	/// returns the logger currently in use
	inline log4cxx::LoggerPtr getLogger(void) { return m_logger; }
	

protected:

	/// listens for a new connection
	void listen(void);

	/// handles a request to stop the server
	void handleStopRequest(void);

	/// handles new connections
	void handleConnection(TCPConnectionPtr& conn,
						  const boost::asio::error& accept_error);

	/// called after we are finished handling a connection
	void finishConnection(TCPConnectionPtr& conn);

	
private:

	// data type for a pool of TCP connections
	typedef std::set<TCPConnectionPtr>		ConnectionPool;

	/// primary logging interface used by this class
	log4cxx::LoggerPtr						m_logger;

	/// mutex to make class thread-safe
	boost::mutex							m_mutex;

	/// manages async tcp connections
	boost::asio::ip::tcp::acceptor			m_tcp_acceptor;

	/// protocol used to handle new connections
	TCPProtocolPtr							m_protocol;

	/// pool of active connections associated with this server 
	ConnectionPool							m_conn_pool;

	/// tcp port number server listens for connections on
	const unsigned int						m_tcp_port;

	/// set to true when we are listening for new connections
	bool									m_is_listening;
};


/// data type for a TCPServer pointer
typedef boost::shared_ptr<TCPServer>	TCPServerPtr;


}	// end namespace pion

#endif
