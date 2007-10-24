// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/bind.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/test/unit_test.hpp>
#include <pion/PionConfig.hpp>
#include <pion/net/PionNet.hpp>

using namespace std;
using namespace pion;
using namespace pion::net;
using boost::asio::ip::tcp;


///
/// HelloServer: simple TCP server that sends "Hello there!" after receiving some data
/// 
class HelloServer
	: public pion::net::TCPServer
{
public:
	HelloServer(const unsigned int tcp_port) : pion::net::TCPServer(tcp_port) {}
	virtual ~HelloServer() {}
	virtual void handleConnection(pion::net::TCPConnectionPtr& tcp_conn) {
		static const std::string HELLO_MESSAGE("Hello there!\n");
		tcp_conn->setLifecycle(pion::net::TCPConnection::LIFECYCLE_CLOSE);	// make sure it will get closed
		tcp_conn->async_write(boost::asio::buffer(HELLO_MESSAGE),
							  boost::bind(&HelloServer::handleWrite, this, tcp_conn,
										  boost::asio::placeholders::error));
	}

private:
	void handleWrite(pion::net::TCPConnectionPtr& tcp_conn,
					 const boost::system::error_code& write_error)
	{
		if (write_error) {
			tcp_conn->finish();
		} else {
			tcp_conn->async_read_some(boost::bind(&HelloServer::handleRead, this, tcp_conn,
												  boost::asio::placeholders::error,
												  boost::asio::placeholders::bytes_transferred));
		}
	}
	void handleRead(pion::net::TCPConnectionPtr& tcp_conn,
					const boost::system::error_code& read_error,
					std::size_t bytes_read)
	{
		static const std::string HELLO_MESSAGE("Goodbye!\n");
		if (read_error) {
			tcp_conn->finish();
		} else {
			tcp_conn->async_write(boost::asio::buffer(HELLO_MESSAGE),
								  boost::bind(&pion::net::TCPConnection::finish, tcp_conn));
		}
	}
};


///
/// HelloServerTests_F: fixture used for running (Hello) server tests
/// 
class HelloServerTests_F {
public:
	HelloServerTests_F() {}
	~HelloServerTests_F() {}	
	inline TCPServerPtr& getServerPtr(void) { return m_engine_mgr.hello_server_ptr; }

private:
	/// class used as static object to initialize the pion network engine
	class PionNetEngineManager
	{
	public:
		PionNetEngineManager() {
			// register a server with the network engine
			hello_server_ptr.reset(new HelloServer(8080));
			BOOST_CHECK(hello_server_ptr);	// shared pointer should not be null
			BOOST_CHECK(PionNet::addServer(hello_server_ptr));
		
			// startup pion
			PionNet::startup();
		}
		~PionNetEngineManager() {
			PionNet::shutdown();
		}	
		TCPServerPtr	hello_server_ptr;
	};
	static PionNetEngineManager	m_engine_mgr;
};

/// static object used to initialize the pion network engine
HelloServerTests_F::PionNetEngineManager	HelloServerTests_F::m_engine_mgr;


/**
 * put the current thread to sleep for an amount of time
 *
 * @param nsec number of nanoseconds (10^-9) to sleep for (default = 0.01 seconds)
 */
void sleep_briefly(unsigned long nsec = 10000000)
{
	boost::xtime stop_time;
	boost::xtime_get(&stop_time, boost::TIME_UTC);
	stop_time.nsec += nsec;
	if (stop_time.nsec >= 1000000000) {
		stop_time.sec++;
		stop_time.nsec -= 1000000000;
	}
	boost::thread::sleep(stop_time);
}


// HelloServer Test Cases

BOOST_FIXTURE_TEST_SUITE(HelloServerTests_S, HelloServerTests_F)

BOOST_AUTO_TEST_CASE(checkTCPServerIsListening) {
	BOOST_CHECK(getServerPtr()->isListening());
}

BOOST_AUTO_TEST_CASE(checkNumberOfActiveServerConnections) {
	// there should be no connections to start
	// sleep first just in case other tests ran before this one, which are still
	// in the process of closing down their connection(s)
	sleep_briefly();
	BOOST_CHECK_EQUAL(getServerPtr()->getConnections(), static_cast<unsigned long>(0));

	// open a connection
	tcp::endpoint localhost(boost::asio::ip::address::from_string("127.0.0.1"), 8080);
	tcp::iostream tcp_stream_a(localhost);
	// we need to wait for the server to accept the connection since it happens
	// in another thread.  This should always take less than 0.1 seconds
	sleep_briefly();
	BOOST_CHECK_EQUAL(getServerPtr()->getConnections(), static_cast<unsigned long>(1));

	// open a few more connections;
	tcp::iostream tcp_stream_b(localhost);
	sleep_briefly();
	BOOST_CHECK_EQUAL(getServerPtr()->getConnections(), static_cast<unsigned long>(2));

	tcp::iostream tcp_stream_c(localhost);
	sleep_briefly();
	BOOST_CHECK_EQUAL(getServerPtr()->getConnections(), static_cast<unsigned long>(3));

	tcp::iostream tcp_stream_d(localhost);
	sleep_briefly();
	BOOST_CHECK_EQUAL(getServerPtr()->getConnections(), static_cast<unsigned long>(4));
	
	// close connections	
	tcp_stream_a.close();
	sleep_briefly();
	BOOST_CHECK_EQUAL(getServerPtr()->getConnections(), static_cast<unsigned long>(3));

	tcp_stream_b.close();
	sleep_briefly();
	BOOST_CHECK_EQUAL(getServerPtr()->getConnections(), static_cast<unsigned long>(2));

	tcp_stream_c.close();
	sleep_briefly();
	BOOST_CHECK_EQUAL(getServerPtr()->getConnections(), static_cast<unsigned long>(1));

	tcp_stream_d.close();
	sleep_briefly();
	BOOST_CHECK_EQUAL(getServerPtr()->getConnections(), static_cast<unsigned long>(0));
}

BOOST_AUTO_TEST_CASE(checkServerConnectionBehavior) {
	// open a connection
	tcp::endpoint localhost(boost::asio::ip::address::from_string("127.0.0.1"), 8080);
	tcp::iostream tcp_stream_a(localhost);

	// read greeting from the server
	std::string message;
	std::getline(tcp_stream_a, message);
	BOOST_CHECK(message == "Hello there!");

	// open a second connection & read the greeting
	tcp::iostream tcp_stream_b(localhost);
	std::getline(tcp_stream_b, message);
	BOOST_CHECK(message == "Hello there!");

	// send greeting to the first server
	tcp_stream_a << "Hi!\n";
	tcp_stream_a.flush();

	// send greeting to the second server
	tcp_stream_b << "Hi!\n";
	tcp_stream_b.flush();
	
	// receive goodbye from the first server
	std::getline(tcp_stream_a, message);
	tcp_stream_a.close();
	BOOST_CHECK(message == "Goodbye!");

	// receive goodbye from the second server
	std::getline(tcp_stream_b, message);
	tcp_stream_b.close();
	BOOST_CHECK(message == "Goodbye!");
}

BOOST_AUTO_TEST_SUITE_END()
