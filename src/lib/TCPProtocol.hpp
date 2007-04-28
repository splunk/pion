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

#ifndef __PION_TCPPROTOCOL_HEADER__
#define __PION_TCPPROTOCOL_HEADER__

#include "TCPConnection.hpp"
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>


namespace pion {	// begin namespace pion

///
/// TCPProtocol: interface class for TCP protocol handlers
/// 
class TCPProtocol : private boost::noncopyable {
public:

	/// default constructor
	explicit TCPProtocol(void) {}

	/// virtual destructor
	virtual ~TCPProtocol() {}

	/**
     * handles a new TCP connection; default behavior does nothing
     * 
     * @param tcp_conn the connection to handle
	 */
	virtual void handleConnection(TCPConnectionPtr& tcp_conn) {
		tcp_conn->finish();
	}
};


/// data type for a tcp protocol handler pointer
typedef boost::shared_ptr<TCPProtocol>		TCPProtocolPtr;


}	// end namespace pion

#endif
