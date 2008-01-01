// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <pion/net/TCPServer.hpp>
#include "ShutdownManager.hpp"

using namespace std;
using namespace pion;
using namespace pion::net;


/// simple TCP server that just sends "Hello there!" to each connection
class HelloServer : public TCPServer {
public:
	HelloServer(const unsigned int tcp_port) : TCPServer(tcp_port) {}
	virtual ~HelloServer() {}
	virtual void handleConnection(TCPConnectionPtr& tcp_conn)
	{
		static const std::string HELLO_MESSAGE("Hello there!\x0D\x0A");
		tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_CLOSE);	// make sure it will get closed
		tcp_conn->async_write(boost::asio::buffer(HELLO_MESSAGE),
							  boost::bind(&TCPConnection::finish, tcp_conn));
	}
};



/// main control function
int main (int argc, char *argv[])
{
	static const unsigned int DEFAULT_PORT = 8080;

	// parse command line: determine port number
	unsigned int port = DEFAULT_PORT;
	if (argc == 2) {
		port = strtoul(argv[1], 0, 10);
		if (port == 0) port = DEFAULT_PORT;
	} else if (argc != 1) {
		std::cerr << "usage: PionHelloServer [port]" << std::endl;
		return 1;
	}

	// setup signal handler
#ifdef PION_WIN32
	SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
#else
	signal(SIGINT, handle_signal);
#endif

	// initialize log system (use simple configuration)
	PionLogger main_log(PION_GET_LOGGER("PionHelloServer"));
	PionLogger pion_log(PION_GET_LOGGER("pion"));
	PION_LOG_SETLEVEL_INFO(main_log);
	PION_LOG_SETLEVEL_INFO(pion_log);
	PION_LOG_CONFIG_BASIC;
	
	try {
		
		// create a new server to handle the Hello TCP protocol
		TCPServerPtr hello_server(new HelloServer(port));
		hello_server->start();
		main_shutdown_manager.wait();

	} catch (std::exception& e) {
		PION_LOG_FATAL(main_log, e.what());
	}

	return 0;
}
