// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/PionConfig.hpp>
#include <pion/PionScheduler.hpp>
#include <pion/net/TCPServer.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/thread/thread.hpp>
#include <boost/test/unit_test.hpp>
#include <pion/net/HTTPRequest.hpp>
#include <pion/net/HTTPResponse.hpp>

using namespace std;
using namespace pion;
using namespace pion::net;
using boost::asio::ip::tcp;

/// sets up logging (run once only)
extern void setup_logging_for_unit_tests(void);


///
/// HelloServer: simple TCP server that sends "Hello there!" after receiving some data
/// 
class HelloServer
	: public pion::net::TCPServer
{
public:
	virtual ~HelloServer() {}

	/**
	 * creates a Hello server
	 *
	 * @param tcp_port port number used to listen for new connections (IPv4)
	 */
	HelloServer(const unsigned int tcp_port) : pion::net::TCPServer(tcp_port) {}
	
	/**
	 * handles a new TCP connection
	 * 
	 * @param tcp_conn the new TCP connection to handle
	 */
	virtual void handleConnection(pion::net::TCPConnectionPtr& tcp_conn) {
		static const std::string HELLO_MESSAGE("Hello there!\n");
		tcp_conn->setLifecycle(pion::net::TCPConnection::LIFECYCLE_CLOSE);	// make sure it will get closed
		tcp_conn->async_write(boost::asio::buffer(HELLO_MESSAGE),
							  boost::bind(&HelloServer::handleWrite, this, tcp_conn,
										  boost::asio::placeholders::error));
	}

	
private:
	
	/**
	 * called after the initial greeting has been sent
	 *
     * @param tcp_conn the TCP connection to the server
	 * @param write_error message that explains what went wrong (if anything)
	 */
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
	
	/**
	 * called after the client's greeting has been received
	 *
     * @param tcp_conn the TCP connection to the server
	 * @param read_error message that explains what went wrong (if anything)
	 * @param bytes_read number of bytes read from the client
	 */
	void handleRead(pion::net::TCPConnectionPtr& tcp_conn,
					const boost::system::error_code& read_error,
					std::size_t bytes_read)
	{
		static const std::string GOODBYE_MESSAGE("Goodbye!\n");
		if (read_error) {
			tcp_conn->finish();
		} else {
			tcp_conn->async_write(boost::asio::buffer(GOODBYE_MESSAGE),
								  boost::bind(&pion::net::TCPConnection::finish, tcp_conn));
		}
	}
};


///
/// HelloServerTests_F: fixture used for running (Hello) server tests
/// 
class HelloServerTests_F {
public:
	// default constructor and destructor
	HelloServerTests_F()
		: hello_server_ptr(new HelloServer(8080))
	{
		setup_logging_for_unit_tests();
		// start the HTTP server
		hello_server_ptr->start();
	}
	~HelloServerTests_F() {
		hello_server_ptr->stop();
	}
	inline TCPServerPtr& getServerPtr(void) { return hello_server_ptr; }

	/**
	 * put the current thread to sleep for an amount of time
	 *
	 * @param nsec number of nanoseconds (10^-9) to sleep for
	 */
	inline void sleep_briefly(unsigned long nsec)
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
	
	/**
	 * check at 0.1 second intervals for up to one second to see if the number
	 * of connections is as expected
	 *
	 * @param expectedNumberOfConnections expected number of connections
	 */
	void checkNumConnectionsForUpToOneSecond(std::size_t expectedNumberOfConnections)
	{
		for (int i = 0; i < 10; ++i) {
			if (getServerPtr()->getConnections() == expectedNumberOfConnections) break;
			sleep_briefly(100000000); // 0.1 seconds
		}
		BOOST_CHECK_EQUAL(getServerPtr()->getConnections(), expectedNumberOfConnections);
	}

private:
	TCPServerPtr	hello_server_ptr;
};


// HelloServer Test Cases

BOOST_FIXTURE_TEST_SUITE(HelloServerTests_S, HelloServerTests_F)

BOOST_AUTO_TEST_CASE(checkTCPServerIsListening) {
	BOOST_CHECK(getServerPtr()->isListening());
}

BOOST_AUTO_TEST_CASE(checkNumberOfActiveServerConnections) {
	// there should be no connections to start, but wait if needed
	// just in case other tests ran before this one, which are still connected
	checkNumConnectionsForUpToOneSecond(static_cast<std::size_t>(0));

	// open a connection
	tcp::endpoint localhost(boost::asio::ip::address::from_string("127.0.0.1"), 8080);
	tcp::iostream tcp_stream_a(localhost);
	// we need to wait for the server to accept the connection since it happens
	// in another thread.  This should always take less than one second.
	checkNumConnectionsForUpToOneSecond(static_cast<std::size_t>(1));

	// open a few more connections;
	tcp::iostream tcp_stream_b(localhost);
	checkNumConnectionsForUpToOneSecond(static_cast<std::size_t>(2));

	tcp::iostream tcp_stream_c(localhost);
	checkNumConnectionsForUpToOneSecond(static_cast<std::size_t>(3));

	tcp::iostream tcp_stream_d(localhost);
	checkNumConnectionsForUpToOneSecond(static_cast<std::size_t>(4));
	
	// close connections	
	tcp_stream_a.close();
	checkNumConnectionsForUpToOneSecond(static_cast<std::size_t>(3));

	tcp_stream_b.close();
	checkNumConnectionsForUpToOneSecond(static_cast<std::size_t>(2));

	tcp_stream_c.close();
	checkNumConnectionsForUpToOneSecond(static_cast<std::size_t>(1));

	tcp_stream_d.close();
	checkNumConnectionsForUpToOneSecond(static_cast<std::size_t>(0));
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

///
/// MockSyncServer: simple TCP server that synchronously receives HTTP requests using HTTPMessage::receive(),
/// and checks that the received request object has some expected properties.
///
class MockSyncServer
	: public pion::net::TCPServer
{
public:
	/**
	 * MockSyncServer constructor
	 *
	 * @param tcp_port port number used to listen for new connections (IPv4)
	 */
	MockSyncServer(const unsigned int tcp_port) : pion::net::TCPServer(tcp_port) {}
	
	virtual ~MockSyncServer() {}

	/**
	 * handles a new TCP connection
	 * 
	 * @param tcp_conn the new TCP connection to handle
	 */
	virtual void handleConnection(pion::net::TCPConnectionPtr& tcp_conn) {
		// wait until an HTTP request is received or an error occurs
		boost::system::error_code error_code;
		HTTPRequest http_request;
		http_request.receive(*tcp_conn, error_code);
		BOOST_REQUIRE(!error_code);

		// check the received request for expected headers and content
		for (std::map<std::string, std::string>::const_iterator i = m_expectedHeaders.begin(); i != m_expectedHeaders.end(); ++i) {
			BOOST_CHECK_EQUAL(http_request.getHeader(i->first), i->second);
		}
		BOOST_CHECK(strncmp(http_request.getContent(), m_expectedContent.c_str(), m_expectedContent.size()) == 0);

		// send a simple response as evidence that this part of the code was reached
		static const std::string GOODBYE_MESSAGE("Goodbye!\n");
		tcp_conn->write(boost::asio::buffer(GOODBYE_MESSAGE), error_code);

		// wrap up
		tcp_conn->setLifecycle(pion::net::TCPConnection::LIFECYCLE_CLOSE);
		tcp_conn->finish();
	}

	static void setExpectations(const std::map<std::string, std::string>& expectedHeaders, 
								const std::string& expectedContent)
	{
		m_expectedHeaders = expectedHeaders;
		m_expectedContent = expectedContent;
	}

private:
	static std::map<std::string, std::string> m_expectedHeaders;
	static std::string m_expectedContent;
};

std::map<std::string, std::string> MockSyncServer::m_expectedHeaders;
std::string MockSyncServer::m_expectedContent;

///
/// MockSyncServerTests_F: fixture used for running MockSyncServer tests
/// 
class MockSyncServerTests_F {
public:
	MockSyncServerTests_F()
		: sync_server_ptr(new MockSyncServer(8080))
	{
		setup_logging_for_unit_tests();
		sync_server_ptr->start();
	}
	~MockSyncServerTests_F() {
		sync_server_ptr->stop();
	}
	inline TCPServerPtr& getServerPtr(void) { return sync_server_ptr; }

private:
	TCPServerPtr	sync_server_ptr;
};


// MockSyncServer Test Cases

BOOST_FIXTURE_TEST_SUITE(MockSyncServerTests_S, MockSyncServerTests_F)

BOOST_AUTO_TEST_CASE(checkMockSyncServerIsListening) {
	BOOST_CHECK(getServerPtr()->isListening());
}

BOOST_AUTO_TEST_CASE(checkReceivedRequestUsingStream) {
	// open a connection
	tcp::endpoint localhost(boost::asio::ip::address::from_string("127.0.0.1"), 8080);
	tcp::iostream tcp_stream(localhost);

	// send request to the server
	tcp_stream << "POST /resource1 HTTP/1.1" << HTTPTypes::STRING_CRLF;
	tcp_stream << HTTPTypes::HEADER_CONTENT_LENGTH << ": 8" << HTTPTypes::STRING_CRLF << HTTPTypes::STRING_CRLF;
	tcp_stream << "12345678";
	tcp_stream.flush();

	// set expectations for received request
	std::map<std::string, std::string> expectedHeaders;
	expectedHeaders[HTTPTypes::HEADER_CONTENT_LENGTH] = "8";
	expectedHeaders[HTTPTypes::HEADER_TRANSFER_ENCODING] = ""; // i.e. check that there is no transfer encoding header
	MockSyncServer::setExpectations(expectedHeaders, "12345678");
	
	// receive goodbye from the first server
	std::string message;
	std::getline(tcp_stream, message);
	BOOST_CHECK(message == "Goodbye!");
	tcp_stream.close();
}

BOOST_AUTO_TEST_CASE(checkReceivedRequestUsingChunkedStream) {
	// open a connection
	tcp::endpoint localhost(boost::asio::ip::address::from_string("127.0.0.1"), 8080);
	tcp::iostream tcp_stream(localhost);

	// send request to the server
	tcp_stream << "POST /resource1 HTTP/1.1" << HTTPTypes::STRING_CRLF;
	tcp_stream << HTTPTypes::HEADER_TRANSFER_ENCODING << ": chunked" << HTTPTypes::STRING_CRLF << HTTPTypes::STRING_CRLF;
	// write first chunk size
	tcp_stream << "A" << HTTPTypes::STRING_CRLF;
	// write first chunk 
	tcp_stream << "abcdefghij" << HTTPTypes::STRING_CRLF;
	// write second chunk size
	tcp_stream << "5" << HTTPTypes::STRING_CRLF;
	// write second chunk 
	tcp_stream << "klmno" << HTTPTypes::STRING_CRLF;
	// write final chunk size
	tcp_stream << "0" << HTTPTypes::STRING_CRLF;
	tcp_stream << HTTPTypes::STRING_CRLF;
	tcp_stream.flush();

	// set expectations for received request
	std::map<std::string, std::string> expectedHeaders;
	expectedHeaders[HTTPTypes::HEADER_TRANSFER_ENCODING] = "chunked";
	expectedHeaders[HTTPTypes::HEADER_CONTENT_LENGTH] = ""; // i.e. check that there is no content length header
	MockSyncServer::setExpectations(expectedHeaders, "abcdefghijklmno");
	
	// receive goodbye from the first server
	std::string message;
	std::getline(tcp_stream, message);
	BOOST_CHECK(message == "Goodbye!");
	tcp_stream.close();
}

BOOST_AUTO_TEST_CASE(checkReceivedRequestUsingRequestObject) {
	// open a connection
	TCPConnection tcp_conn(PionScheduler::getInstance().getIOService());
	boost::system::error_code error_code;
	error_code = tcp_conn.connect(boost::asio::ip::address::from_string("127.0.0.1"), 8080);
	BOOST_REQUIRE(!error_code);

	// send request to the server
	HTTPRequest http_request;
	http_request.addHeader("foo", "bar");
	http_request.setContentLength(4);
	http_request.createContentBuffer();
	memcpy(http_request.getContent(), "wxyz", 4);
	http_request.send(tcp_conn, error_code);
	BOOST_REQUIRE(!error_code);

	std::map<std::string, std::string> expectedHeaders;
	expectedHeaders[HTTPTypes::HEADER_CONTENT_LENGTH] = "4";
	expectedHeaders[HTTPTypes::HEADER_TRANSFER_ENCODING] = ""; // i.e. check that there is no transfer encoding header
	expectedHeaders["foo"] = "bar";
	MockSyncServer::setExpectations(expectedHeaders, "wxyz");

	// receive the response from the server
	tcp_conn.read_some(error_code);
	BOOST_CHECK(!error_code);
	BOOST_CHECK(strncmp(tcp_conn.getReadBuffer().data(), "Goodbye!", strlen("Goodbye!")) == 0);
}

BOOST_AUTO_TEST_SUITE_END()
