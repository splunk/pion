// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/bind.hpp>
#include <pion/net/PionNet.hpp>
#include <iostream>
#ifndef PION_WIN32
	#include <signal.h>
#endif

using namespace std;
using namespace pion;
using namespace pion::net;


/// stops Pion when it receives signals
#ifdef PION_WIN32
BOOL WINAPI console_ctrl_handler(DWORD ctrl_type)
{
	switch(ctrl_type) {
		case CTRL_C_EVENT:
		case CTRL_BREAK_EVENT:
		case CTRL_CLOSE_EVENT:
		case CTRL_SHUTDOWN_EVENT:
			PionNet::shutdown();
			return TRUE;
		default:
			return FALSE;
	}
}
#else
void handle_signal(int sig)
{
	PionNet::shutdown();
}
#endif


/// simple TCP server that just sends "Hello there!" to each connection
class HelloServer : public TCPServer {
public:
	HelloServer(const unsigned int tcp_port) : TCPServer(tcp_port) {}
	virtual ~HelloServer() {}
	virtual void handleConnection(TCPConnectionPtr& tcp_conn)
	{
		static const std::string HELLO_MESSAGE("Hello there!\r\n");
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
		std::cerr << "usage: PionServerTest [port]" << std::endl;
		return 1;
	}

	// setup signal handler
#ifdef PION_WIN32
	SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
#else
	signal(SIGINT, handle_signal);
#endif

	// initialize log system (use simple configuration)
	PionLogger main_log(PION_GET_LOGGER("PionServerTest"));
	PionLogger pion_log(PION_GET_LOGGER("Pion"));
	PION_LOG_SETLEVEL_DEBUG(main_log);
	PION_LOG_SETLEVEL_DEBUG(pion_log);
	PION_LOG_CONFIG_BASIC;
	
	try {
		
		// create a new server to handle the Hello TCP protocol
		TCPServerPtr hello_server(new HelloServer(port));
		if (! PionNet::addServer(hello_server)) {
			PION_LOG_FATAL(main_log, "Failed to add HelloServer on port " << port);
			return 1;
		}
	
		// startup pion
		PionNet::startup();
		
		// run until stopped
		PionNet::join();

	} catch (std::exception& e) {
		PION_LOG_FATAL(main_log, e.what());
	}

	return 0;
}
