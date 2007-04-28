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

#ifndef __PION_TCPCONNECTION_HEADER__
#define __PION_TCPCONNECTION_HEADER__

#include "PionLogger.hpp"
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/function.hpp>
#include <string>


namespace pion {	// begin namespace pion

///
/// TCPConnection: represents a single tcp connection
/// 
class TCPConnection : public boost::enable_shared_from_this<TCPConnection>, private boost::noncopyable {
public:

	// function that handles TCP connection objects
	typedef boost::function1<void, boost::shared_ptr<TCPConnection> >	ConnectionHandler;
	
	/// data type for a socket connection
	typedef boost::asio::ip::tcp::socket			Socket;

	/// data type for a data buffer
	typedef boost::array<char, 8192>				Buffer;

	/// default destructor
	virtual ~TCPConnection() {}

	/**
	 * constructor to create new TCP connections
	 *
     * @param io_service asio service associated with the connection
     * @param finished_handler function called when the connection
     *                         is finished
	 */
	explicit TCPConnection(boost::asio::io_service& io_service, ConnectionHandler finished_handler)
		: m_tcp_socket(io_service), m_finished_handler(finished_handler) {}
	
	/// close the tcp socket
	inline void close(void) { m_tcp_socket.close(); }

	/// should be called after a protocol is finished with the connection
	inline void finish(void) { close(); m_finished_handler(shared_from_this()); }

	// public accessors
	inline Socket& getSocket(void) { return m_tcp_socket; }
	inline Buffer& getReadBuffer(void) { return m_read_buffer; }


private:

	/// tcp connection socket
	Socket						m_tcp_socket;

	/// function called when the connection is finished
	ConnectionHandler			m_finished_handler;

	/// buffer used for reading data from the connection (input)
	Buffer						m_read_buffer;
};


/// data type for a TCPConnection pointer
typedef boost::shared_ptr<TCPConnection>	TCPConnectionPtr;


}	// end namespace pion

#endif
