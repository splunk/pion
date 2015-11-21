// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/bind.hpp>
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <boost/thread.hpp>
#pragma GCC diagnostic warning "-Wunused-parameter"
#include <boost/function.hpp>
#include <boost/test/unit_test.hpp>
#include <pion/config.hpp>
#include <pion/scheduler.hpp>
#include <pion/tcp/stream.hpp>
#include <pion/stdx/asio.hpp>

using namespace std;
using namespace pion;


///
/// tcp_stream_tests_F: fixture used for performing tcp::stream tests
/// 
class tcp_stream_tests_F {
public:
    
    /// data type for a function that handles tcp::stream connections
    typedef boost::function1<void,tcp::stream&>   connection_handler;
    
    
    // default constructor and destructor
    tcp_stream_tests_F() {
    }
    virtual ~tcp_stream_tests_F() {}
    
    /**
     * listen for a TCP connection and call the connection handler when connected
     *
     * @param conn_handler function to call after a connection is established
     */
    void acceptConnection(connection_handler conn_handler) {
        // configure the acceptor service
        stdx::asio::ip::tcp::acceptor   tcp_acceptor(m_scheduler.get_io_service());
        stdx::asio::ip::tcp::endpoint   tcp_endpoint(stdx::asio::ip::tcp::v4(), 0);
        tcp_acceptor.open(tcp_endpoint.protocol());

        // allow the acceptor to reuse the address (i.e. SO_REUSEADDR)
        tcp_acceptor.set_option(stdx::asio::ip::tcp::acceptor::reuse_address(true));
        tcp_acceptor.bind(tcp_endpoint);
        tcp_acceptor.listen();

        // notify test thread that we are ready to accept a connection
        {
            // wait for test thread to be waiting on the signal
            stdx::unique_lock<stdx::mutex> accept_lock(m_accept_mutex);
            // trigger signal to wake test thread
            m_port = tcp_acceptor.local_endpoint().port();
            m_accept_ready.notify_one();
        }

        // schedule another thread to listen for a TCP connection
        tcp::stream listener_stream(m_scheduler.get_io_service());
        boost::system::error_code ec = listener_stream.accept(tcp_acceptor);
        tcp_acceptor.close();
        BOOST_REQUIRE(! ec);
        
        // call the connection handler
        conn_handler(listener_stream);
    }
    
    /// sends a "Hello" to a tcp::stream
    static void sendHello(tcp::stream& str) {
        str << "Hello" << std::endl;
        str.flush();
    }
    
    /// port where acceptor listens
    int     m_port;

    /// used to schedule work across multiple threads
    single_service_scheduler      m_scheduler;

    /// used to notify test thread when acceptConnection() is ready
    stdx::condition_variable                m_accept_ready;
    
    /// used to sync test thread with acceptConnection()
    stdx::mutex                    m_accept_mutex;
};


// tcp::stream Test Cases

BOOST_FIXTURE_TEST_SUITE(tcp_stream_tests_S, tcp_stream_tests_F)

BOOST_AUTO_TEST_CASE(checkTCPConnectToAnotherStream) {
    stdx::unique_lock<stdx::mutex> accept_lock(m_accept_mutex);

    // schedule another thread to listen for a TCP connection
    connection_handler conn_handler(boost::bind(&tcp_stream_tests_F::sendHello, _1));
    stdx::thread listener_thread(boost::bind(&tcp_stream_tests_F::acceptConnection,
                                              this, conn_handler) );
    m_scheduler.add_active_user();
    m_accept_ready.wait(accept_lock);

    // connect to the listener
    tcp::stream client_str(m_scheduler.get_io_service());
    boost::system::error_code ec;
    ec = client_str.connect(stdx::asio::ip::address::from_string("127.0.0.1"), m_port);
    BOOST_REQUIRE(! ec);
    
    // get the hello message
    std::string response_msg;
    client_str >> response_msg;
    BOOST_CHECK_EQUAL(response_msg, "Hello");

    client_str.close();
    listener_thread.join();
    m_scheduler.remove_active_user();
}

BOOST_AUTO_TEST_SUITE_END()

#define BIG_BUF_SIZE (12 * 1024)

///
/// tcp_stream_buffer_tests_F: fixture that includes a big data buffer used for tests
/// 
class tcp_stream_buffer_tests_F
    : public tcp_stream_tests_F
{
public:
    // default constructor and destructor
    tcp_stream_buffer_tests_F() {
        // fill the buffer with non-random characters
        for (unsigned long n = 0; n < BIG_BUF_SIZE; ++n) {
            m_big_buf[n] = char(n);
        }
    }
    virtual ~tcp_stream_buffer_tests_F() {}
    
    /// sends the big buffer contents to a tcp::stream
    void sendBigBuffer(tcp::stream& str) {
        str.write(m_big_buf, BIG_BUF_SIZE);
        str.flush();
    }
    
    /// big data buffer used for the tests
    char m_big_buf[BIG_BUF_SIZE];
};

    
BOOST_FIXTURE_TEST_SUITE(tcp_stream_buffer_tests_S, tcp_stream_buffer_tests_F)

BOOST_AUTO_TEST_CASE(checkSendAndReceiveBiggerThanBuffers) {
    stdx::unique_lock<stdx::mutex> accept_lock(m_accept_mutex);

    // schedule another thread to listen for a TCP connection
    connection_handler conn_handler(boost::bind(&tcp_stream_buffer_tests_F::sendBigBuffer, this, _1));
    stdx::thread listener_thread(boost::bind(&tcp_stream_buffer_tests_F::acceptConnection,
                                              this, conn_handler) );
    m_scheduler.add_active_user();
    m_accept_ready.wait(accept_lock);

    // connect to the listener
    tcp::stream client_str(m_scheduler.get_io_service());
    boost::system::error_code ec;
    ec = client_str.connect(stdx::asio::ip::address::from_string("127.0.0.1"), m_port);
    BOOST_REQUIRE(! ec);
    
    // read the big buffer contents
    char another_buf[BIG_BUF_SIZE];
    BOOST_REQUIRE(client_str.read(another_buf, BIG_BUF_SIZE));
    BOOST_CHECK_EQUAL(memcmp(m_big_buf, another_buf, BIG_BUF_SIZE), 0);

    client_str.close();
    listener_thread.join();
    m_scheduler.remove_active_user();
}

BOOST_AUTO_TEST_SUITE_END()
