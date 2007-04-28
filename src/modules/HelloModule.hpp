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

#ifndef __PION_HELLOMODULE_HEADER__
#define __PION_HELLOMODULE_HEADER__

#include "HTTPModule.hpp"
#include "HTTPResponse.hpp"


namespace pion {	// begin namespace pion

///
/// HelloModule: simple HTTP module that responds with "Hello World!"
///
class HelloModule : public HTTPModule {
public:
	// default constructor & destructor
	virtual ~HelloModule() {}
	HelloModule(void) : HTTPModule("/hello") {}
	
	/// responds to requests with "Hello World!"
	virtual bool handleRequest(HTTPRequestPtr& request, TCPConnectionPtr& tcp_conn,
							   TCPConnection::ConnectionHandler& keepalive_handler)
	{
		static const std::string HELLO_HTML = "<html><body>Hello World!</body></html>\r\n\r\n";
		HTTPResponsePtr response(HTTPResponse::create(keepalive_handler, tcp_conn));
		response->writeNoCopy(HELLO_HTML);
		response->send(request->checkKeepAlive());
		return true;
	}
};


}	// end namespace pion

#endif
