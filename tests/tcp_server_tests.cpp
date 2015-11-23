// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/config.hpp>
#include <pion/scheduler.hpp>
#include <pion/tcp/server.hpp>
#include <boost/test/unit_test.hpp>
#include <pion/http/request.hpp>
#include <pion/http/response.hpp>
#include <pion/stdx/asio.hpp>
#include <pion/stdx/thread.hpp>
#include <pion/stdx/functional.hpp>

using namespace std;
using namespace pion;


///
/// HelloServer: simple TCP server that sends "Hello there!" after receiving some data
/// 
class HelloServer
    : public pion::tcp::server
{
public:
    virtual ~HelloServer() {}

    /**
     * creates a Hello server
     *
     * @param tcp_port port number used to listen for new connections (IPv4)
     */
    HelloServer(const unsigned int tcp_port = 0) : pion::tcp::server(tcp_port) {}
    
    /**
     * handles a new TCP connection
     * 
     * @param tcp_conn the new TCP connection to handle
     */
    virtual void handle_connection(const pion::tcp::connection_ptr& tcp_conn) {
        static const std::string HELLO_MESSAGE("Hello there!\n");
        tcp_conn->set_lifecycle(pion::tcp::connection::LIFECYCLE_CLOSE);  // make sure it will get closed
        tcp_conn->async_write(stdx::asio::buffer(HELLO_MESSAGE),
                              stdx::bind(&HelloServer::handle_write, this, tcp_conn,
                                          stdx::asio::placeholders::error));
    }

    
private:
    
    /**
     * called after the initial greeting has been sent
     *
     * @param tcp_conn the TCP connection to the server
     * @param write_error message that explains what went wrong (if anything)
     */
    void handle_write(const pion::tcp::connection_ptr& tcp_conn,
                     const stdx::error_code& write_error)
    {
        if (write_error) {
            tcp_conn->finish();
        } else {
            tcp_conn->async_read_some(stdx::bind(&HelloServer::handleRead, this, tcp_conn,
                                                  stdx::asio::placeholders::error,
                                                  stdx::asio::placeholders::bytes_transferred));
        }
    }
    
    /**
     * called after the client's greeting has been received
     *
     * @param tcp_conn the TCP connection to the server
     * @param read_error message that explains what went wrong (if anything)
     * @param bytes_read number of bytes read from the client
     */
    void handleRead(const pion::tcp::connection_ptr& tcp_conn,
                    const stdx::error_code& read_error,
                    std::size_t bytes_read)
    {
        static const std::string GOODBYE_MESSAGE("Goodbye!\n");
        if (read_error) {
            tcp_conn->finish();
        } else if (bytes_read == 5 && memcmp(tcp_conn->get_read_buffer().data(), "throw", 5) == 0) {
            throw int(1);
        } else {
            tcp_conn->async_write(stdx::asio::buffer(GOODBYE_MESSAGE),
                                  stdx::bind(&pion::tcp::connection::finish, tcp_conn));
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
        : hello_server_ptr(new HelloServer)
    {
        // start the HTTP server
        hello_server_ptr->start();
    }
    ~HelloServerTests_F() {
        hello_server_ptr->stop();
    }
    inline tcp::server_ptr& getServerPtr(void) { return hello_server_ptr; }

    /**
     * check at 0.1 second intervals for up to one second to see if the number
     * of connections is as expected
     *
     * @param expectedNumberOfConnections expected number of connections
     */
    void checkNumConnectionsForUpToOneSecond(std::size_t expectedNumberOfConnections)
    {
        for (int i = 0; i < 10; ++i) {
            if (getServerPtr()->get_connections() == expectedNumberOfConnections) break;
            scheduler::sleep(0, 100000000); // 0.1 seconds
        }
        BOOST_CHECK_EQUAL(getServerPtr()->get_connections(), expectedNumberOfConnections);
    }

private:
    tcp::server_ptr    hello_server_ptr;
};


// HelloServer Test Cases

BOOST_FIXTURE_TEST_SUITE(HelloServerTests_S, HelloServerTests_F)

BOOST_AUTO_TEST_CASE(checkTcpServerIsListening) {
    BOOST_CHECK(getServerPtr()->is_listening());
}

BOOST_AUTO_TEST_CASE(checkNumberOfActiveServerConnections) {
    // there should be no connections to start, but wait if needed
    // just in case other tests ran before this one, which are still connected
    checkNumConnectionsForUpToOneSecond(static_cast<std::size_t>(0));

    // open a connection
    stdx::asio::ip::tcp::endpoint localhost(stdx::asio::ip::address::from_string("127.0.0.1"), getServerPtr()->get_port());
    stdx::asio::ip::tcp::iostream tcp_stream_a(localhost);
    // we need to wait for the server to accept the connection since it happens
    // in another thread.  This should always take less than one second.
    checkNumConnectionsForUpToOneSecond(static_cast<std::size_t>(1));

    // open a few more connections;
    stdx::asio::ip::tcp::iostream tcp_stream_b(localhost);
    checkNumConnectionsForUpToOneSecond(static_cast<std::size_t>(2));

    stdx::asio::ip::tcp::iostream tcp_stream_c(localhost);
    checkNumConnectionsForUpToOneSecond(static_cast<std::size_t>(3));

    stdx::asio::ip::tcp::iostream tcp_stream_d(localhost);
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
    stdx::asio::ip::tcp::endpoint localhost(stdx::asio::ip::address::from_string("127.0.0.1"), getServerPtr()->get_port());
    stdx::asio::ip::tcp::iostream tcp_stream_a(localhost);

    // read greeting from the server
    std::string str;
    std::getline(tcp_stream_a, str);
    BOOST_CHECK(str == "Hello there!");

    // open a second connection & read the greeting
    stdx::asio::ip::tcp::iostream tcp_stream_b(localhost);
    std::getline(tcp_stream_b, str);
    BOOST_CHECK(str == "Hello there!");

    // send greeting to the first server
    tcp_stream_a << "Hi!\n";
    tcp_stream_a.flush();

    // send greeting to the second server
    tcp_stream_b << "Hi!\n";
    tcp_stream_b.flush();
    
    // receive goodbye from the first server
    std::getline(tcp_stream_a, str);
    tcp_stream_a.close();
    BOOST_CHECK(str == "Goodbye!");

    // receive goodbye from the second server
    std::getline(tcp_stream_b, str);
    tcp_stream_b.close();
    BOOST_CHECK(str == "Goodbye!");
}

BOOST_AUTO_TEST_CASE(checkServerExceptionsGetCaught) {
    // open a connection
    stdx::asio::ip::tcp::endpoint localhost(stdx::asio::ip::address::from_string("127.0.0.1"), getServerPtr()->get_port());
    stdx::asio::ip::tcp::iostream tcp_stream_a(localhost);

    // read greeting from the server
    std::string str;
    std::getline(tcp_stream_a, str);
    BOOST_CHECK(str == "Hello there!");

    // send throw request to the server
    tcp_stream_a << "throw";
    tcp_stream_a.flush();
    tcp_stream_a.close();
}

BOOST_AUTO_TEST_SUITE_END()

///
/// MockSyncServer: simple TCP server that synchronously receives HTTP requests using http::message::receive(),
/// and checks that the received request object has some expected properties.
///
class MockSyncServer
    : public pion::tcp::server
{
public:
    /**
     * MockSyncServer constructor
     *
     * @param sched the scheduler that will be used to manage worker threads
     * @param tcp_port port number used to listen for new connections (IPv4)
     */
    MockSyncServer(scheduler& sched, const unsigned int tcp_port = 0)
        : pion::tcp::server(sched, tcp_port) {}
    
    virtual ~MockSyncServer() {}

    /**
     * handles a new TCP connection
     * 
     * @param tcp_conn the new TCP connection to handle
     */
    virtual void handle_connection(const pion::tcp::connection_ptr& tcp_conn) {
        // wait until an HTTP request is received or an error occurs
        stdx::error_code error_code;
        http::request http_request;
        http_request.receive(*tcp_conn, error_code);
        BOOST_REQUIRE(!error_code);

        // check the received request for expected headers and content
        for (std::map<std::string, std::string>::const_iterator i = m_expectedHeaders.begin(); i != m_expectedHeaders.end(); ++i) {
            BOOST_CHECK_EQUAL(http_request.get_header(i->first), i->second);
        }
        BOOST_CHECK_EQUAL(m_expectedContent, http_request.get_content());

        if (m_additional_request_test)
            BOOST_CHECK(m_additional_request_test(http_request));

        // send a simple response as evidence that this part of the code was reached
        static const std::string GOODBYE_MESSAGE("Goodbye!\n");
        tcp_conn->write(stdx::asio::buffer(GOODBYE_MESSAGE), error_code);

        // wrap up
        tcp_conn->set_lifecycle(pion::tcp::connection::LIFECYCLE_CLOSE);
        tcp_conn->finish();
    }

    void setExpectations(const std::map<std::string, std::string>& expectedHeaders, 
                         const std::string& expectedContent,
                         stdx::function<bool(http::request&)> additional_request_test = NULL)
    {
        m_expectedHeaders = expectedHeaders;
        m_expectedContent = expectedContent;
        m_additional_request_test = additional_request_test;
    }

private:
    std::map<std::string, std::string> m_expectedHeaders;
    std::string m_expectedContent;
    stdx::function<bool(http::request&)> m_additional_request_test;
};


///
/// MockSyncServerTests_F: fixture used for running MockSyncServer tests
/// 
class MockSyncServerTests_F {
public:
    MockSyncServerTests_F()
        : m_scheduler(), m_sync_server_ptr(new MockSyncServer(m_scheduler))
    {
        m_sync_server_ptr->start();
    }
    ~MockSyncServerTests_F() {
        m_sync_server_ptr->stop();
    }
    inline stdx::shared_ptr<MockSyncServer>& getServerPtr(void) { return m_sync_server_ptr; }
    inline stdx::asio::io_service& get_io_service(void) { return m_scheduler.get_io_service(); }

private:
    single_service_scheduler          m_scheduler;
    stdx::shared_ptr<MockSyncServer>   m_sync_server_ptr;
};


// MockSyncServer Test Cases

BOOST_FIXTURE_TEST_SUITE(MockSyncServerTests_S, MockSyncServerTests_F)

BOOST_AUTO_TEST_CASE(checkMockSyncServerIsListening) {
    BOOST_CHECK(getServerPtr()->is_listening());
}

BOOST_AUTO_TEST_CASE(checkReceivedRequestUsingStream) {
    // open a connection
    stdx::asio::ip::tcp::endpoint localhost(stdx::asio::ip::address::from_string("127.0.0.1"), getServerPtr()->get_port());
    stdx::asio::ip::tcp::iostream tcp_stream(localhost);

    // set expectations for received request
    std::map<std::string, std::string> expectedHeaders;
    expectedHeaders[http::types::HEADER_CONTENT_LENGTH] = "8";
    expectedHeaders[http::types::HEADER_TRANSFER_ENCODING] = ""; // i.e. check that there is no transfer encoding header
    getServerPtr()->setExpectations(expectedHeaders, "12345678");
    
    // send request to the server
    tcp_stream << "POST /resource1 HTTP/1.1" << http::types::STRING_CRLF;
    tcp_stream << http::types::HEADER_CONTENT_LENGTH << ": 8" << http::types::STRING_CRLF << http::types::STRING_CRLF;
    tcp_stream << "12345678";
    tcp_stream.flush();

    // receive goodbye from the server
    std::string str;
    std::getline(tcp_stream, str);
    BOOST_CHECK(str == "Goodbye!");
    tcp_stream.close();
}

BOOST_AUTO_TEST_CASE(checkReceivedRequestUsingChunkedStream) {
    // open a connection
    stdx::asio::ip::tcp::endpoint localhost(stdx::asio::ip::address::from_string("127.0.0.1"), getServerPtr()->get_port());
    stdx::asio::ip::tcp::iostream tcp_stream(localhost);

    // set expectations for received request
    std::map<std::string, std::string> expectedHeaders;
    expectedHeaders[http::types::HEADER_TRANSFER_ENCODING] = "chunked";
    expectedHeaders[http::types::HEADER_CONTENT_LENGTH] = ""; // i.e. check that there is no content length header
    getServerPtr()->setExpectations(expectedHeaders, "abcdefghijklmno");
    
    // send request to the server
    tcp_stream << "POST /resource1 HTTP/1.1" << http::types::STRING_CRLF;
    tcp_stream << http::types::HEADER_TRANSFER_ENCODING << ": chunked" << http::types::STRING_CRLF << http::types::STRING_CRLF;
    // write first chunk size
    tcp_stream << "A" << http::types::STRING_CRLF;
    // write first chunk 
    tcp_stream << "abcdefghij" << http::types::STRING_CRLF;
    // write second chunk size
    tcp_stream << "5" << http::types::STRING_CRLF;
    // write second chunk 
    tcp_stream << "klmno" << http::types::STRING_CRLF;
    // write final chunk size
    tcp_stream << "0" << http::types::STRING_CRLF;
    tcp_stream << http::types::STRING_CRLF;
    tcp_stream.flush();

    // receive goodbye from the server
    std::string str;
    std::getline(tcp_stream, str);
    BOOST_CHECK(str == "Goodbye!");
    tcp_stream.close();
}

BOOST_AUTO_TEST_CASE(checkReceivedRequestUsingExtraWhiteSpaceAroundChunkSizes) {
    // open a connection
    stdx::asio::ip::tcp::endpoint localhost(stdx::asio::ip::address::from_string("127.0.0.1"), getServerPtr()->get_port());
    stdx::asio::ip::tcp::iostream tcp_stream(localhost);

    // set expectations for received request
    std::map<std::string, std::string> expectedHeaders;
    expectedHeaders[http::types::HEADER_TRANSFER_ENCODING] = "chunked";
    getServerPtr()->setExpectations(expectedHeaders, "abcdefghijklmno");
    
    // send request to the server
    tcp_stream << "POST /resource1 HTTP/1.1" << http::types::STRING_CRLF;
    tcp_stream << http::types::HEADER_TRANSFER_ENCODING << ": chunked" << http::types::STRING_CRLF << http::types::STRING_CRLF;

    // write some chunks with chunk sizes with leading and/or trailing tabs and spaces
    tcp_stream <<      " 2"       << http::types::STRING_CRLF << "ab" << http::types::STRING_CRLF;
    tcp_stream <<       "2\t \t " << http::types::STRING_CRLF << "cd" << http::types::STRING_CRLF;
    tcp_stream <<     "  2  "     << http::types::STRING_CRLF << "ef" << http::types::STRING_CRLF;
    tcp_stream << "\t \t 2\t\t"   << http::types::STRING_CRLF << "gh" << http::types::STRING_CRLF;

    // write chunks with extra CRLF before chunk size
    // (extra CRLF after chunk size not allowed, since it would be ambiguous)
    tcp_stream << http::types::STRING_CRLF << "2"   << http::types::STRING_CRLF << "ij" << http::types::STRING_CRLF;
    tcp_stream << http::types::STRING_CRLF << " 5 " << http::types::STRING_CRLF << "klmno" << http::types::STRING_CRLF;

    // write final chunk size
    tcp_stream << "0" << http::types::STRING_CRLF;
    tcp_stream << http::types::STRING_CRLF;
    tcp_stream.flush();

    // receive goodbye from the server
    std::string str;
    std::getline(tcp_stream, str);
    BOOST_CHECK(str == "Goodbye!");
    tcp_stream.close();
}

BOOST_AUTO_TEST_CASE(checkReceivedRequestUsingRequestObject) {
    // open a connection
    pion::tcp::connection tcp_conn(get_io_service());
    stdx::error_code error_code;
    error_code = tcp_conn.connect(stdx::asio::ip::address::from_string("127.0.0.1"), getServerPtr()->get_port());
    BOOST_REQUIRE(!error_code);

    std::map<std::string, std::string> expectedHeaders;
    expectedHeaders[http::types::HEADER_CONTENT_LENGTH] = "4";
    expectedHeaders[http::types::HEADER_TRANSFER_ENCODING] = ""; // i.e. check that there is no transfer encoding header
    expectedHeaders["foo"] = "bar";
    getServerPtr()->setExpectations(expectedHeaders, "wxyz");
    
    // send request to the server
    http::request http_request;
    http_request.add_header("foo", "bar");
    http_request.set_content_length(4);
    http_request.create_content_buffer();
    memcpy(http_request.get_content(), "wxyz", 4);
    http_request.send(tcp_conn, error_code);
    BOOST_REQUIRE(!error_code);

    // receive the response from the server
    tcp_conn.read_some(error_code);
    BOOST_CHECK(!error_code);
    BOOST_CHECK(strncmp(tcp_conn.get_read_buffer().data(), "Goodbye!", strlen("Goodbye!")) == 0);
}

bool queryKeyXHasValueY(http::request& http_request) {
    return http_request.get_query("x") == "y";
}

BOOST_AUTO_TEST_CASE(checkQueryOfReceivedRequestParsed) {
    // open a connection
    stdx::asio::ip::tcp::endpoint localhost(stdx::asio::ip::address::from_string("127.0.0.1"), getServerPtr()->get_port());
    stdx::asio::ip::tcp::iostream tcp_stream(localhost);

    // set expectations for received request
    std::map<std::string, std::string> empty_map;
    getServerPtr()->setExpectations(empty_map, "", queryKeyXHasValueY);
    
    // send request to the server
    tcp_stream << "GET /resource1?x=y HTTP/1.1" << http::types::STRING_CRLF << http::types::STRING_CRLF;
    tcp_stream.flush();

    // receive goodbye from the server
    std::string str;
    std::getline(tcp_stream, str);
    BOOST_CHECK(str == "Goodbye!");
    tcp_stream.close();
}

BOOST_AUTO_TEST_CASE(checkUrlEncodedQueryInPostContentParsed) {
    // open a connection
    stdx::asio::ip::tcp::endpoint localhost(stdx::asio::ip::address::from_string("127.0.0.1"), getServerPtr()->get_port());
    stdx::asio::ip::tcp::iostream tcp_stream(localhost);

    // set expectations for received request
    std::map<std::string, std::string> expectedHeaders;
    expectedHeaders[http::types::HEADER_CONTENT_LENGTH] = "3";
    getServerPtr()->setExpectations(expectedHeaders, "x=y", queryKeyXHasValueY);
    
    // send request to the server
    tcp_stream << "POST /resource1 HTTP/1.1" << http::types::STRING_CRLF
               << http::types::HEADER_CONTENT_LENGTH << ": 3" << http::types::STRING_CRLF
               << http::types::HEADER_CONTENT_TYPE << ": " << http::types::CONTENT_TYPE_URLENCODED << "; charset=ECMA-cyrillic"
               << http::types::STRING_CRLF << http::types::STRING_CRLF
               << "x=y";
    tcp_stream.flush();

    // receive goodbye from the server
    std::string str;
    std::getline(tcp_stream, str);
    BOOST_CHECK(str == "Goodbye!");
    tcp_stream.close();
}
/*
Charset parsing removed due to performance concerns, but might be restored later.

bool charsetIsEcmaCyrillic(http::request& http_request) {
    return http_request.getCharset() == "ECMA-cyrillic";
}

BOOST_AUTO_TEST_CASE(checkCharsetOfReceivedRequest) {
    // open a connection
    stdx::asio::ip::tcp::endpoint localhost(stdx::asio::ip::address::from_string("127.0.0.1"), getServerPtr()->get_port());
    stdx::asio::ip::tcp::iostream tcp_stream(localhost);

    // set expectations for received request
    std::map<std::string, std::string> expectedHeaders;
    expectedHeaders[http::types::HEADER_CONTENT_LENGTH] = "3";
    getServerPtr()->setExpectations(expectedHeaders, "x=y", charsetIsEcmaCyrillic);
    
    // send request to the server
    tcp_stream << "POST /resource1 HTTP/1.1" << http::types::STRING_CRLF
               << http::types::HEADER_CONTENT_LENGTH << ": 3" << http::types::STRING_CRLF
               << http::types::HEADER_CONTENT_TYPE << ": " << http::types::CONTENT_TYPE_URLENCODED << "; charset=ECMA-cyrillic"
               << http::types::STRING_CRLF << http::types::STRING_CRLF
               << "x=y";
    tcp_stream.flush();

    // receive goodbye from the server
    std::string str;
    std::getline(tcp_stream, str);
    BOOST_CHECK(str == "Goodbye!");
    tcp_stream.close();
}
*/

BOOST_AUTO_TEST_SUITE_END()
