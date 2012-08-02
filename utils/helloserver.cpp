// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <pion/process.hpp>
#include <pion/tcp/server.hpp>

using namespace std;
using namespace pion;


/// simple TCP server that just sends "Hello there!" to each connection
class HelloServer : public tcp::server {
public:
    HelloServer(const unsigned int tcp_port) : tcp::server(tcp_port) {}
    virtual ~HelloServer() {}
    virtual void handleConnection(tcp::connection_ptr& tcp_conn)
    {
        static const std::string HELLO_MESSAGE("Hello there!\x0D\x0A");
        tcp_conn->set_lifecycle(connection::LIFECYCLE_CLOSE); // make sure it will get closed
        tcp_conn->async_write(boost::asio::buffer(HELLO_MESSAGE),
                              boost::bind(&connection::finish, tcp_conn));
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
        std::cerr << "usage: helloserver [port]" << std::endl;
        return 1;
    }

    // initialize signal handlers, etc.
    process::initialize();

    // initialize log system (use simple configuration)
    logger main_log(PION_GET_LOGGER("helloserver"));
    logger pion_log(PION_GET_LOGGER("pion"));
    PION_LOG_SETLEVEL_INFO(main_log);
    PION_LOG_SETLEVEL_INFO(pion_log);
    PION_LOG_CONFIG_BASIC;
    
    try {
        
        // create a new server to handle the Hello TCP protocol
        tcp::server_ptr hello_server(new HelloServer(port));
        hello_server->start();
        process::wait_for_shutdown();

    } catch (std::exception& e) {
        PION_LOG_FATAL(main_log, boost::diagnostic_information(e));
    }

    return 0;
}
