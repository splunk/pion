// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//


#include <pion/config.hpp>
#include <asio.hpp>
#include <memory>
#include <mutex>
#include <regex>
#include <condition_variable>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <pion/plugin.hpp>
#include <pion/scheduler.hpp>
#include <pion/http/request.hpp>
#include <pion/http/response.hpp>
#include <pion/http/request_writer.hpp>
#include <pion/http/response_reader.hpp>
#include <pion/http/plugin_server.hpp>
#include <pion/user.hpp>
#include <pion/http/basic_auth.hpp>
#include <pion/http/cookie_auth.hpp>


using namespace std;
using namespace pion;

PION_DECLARE_PLUGIN(EchoService)
PION_DECLARE_PLUGIN(FileService)
PION_DECLARE_PLUGIN(HelloService)
PION_DECLARE_PLUGIN(LogService)
PION_DECLARE_PLUGIN(CookieService)

#if defined(PION_CMAKE_BUILD)
    #include "plugin_path.hpp"
#elif defined(PION_XCODE)
    static const std::string PATH_TO_PLUGINS("../bin/Debug");
    static const std::string SSL_PEM_FILE("../utils/sslkey.pem");
    static const std::string SERVICES_CONFIG_FILE("../tests/config/testservices.conf");
#else
    // same for Unix and Windows
    static const std::string PATH_TO_PLUGINS("../services/.libs");
    static const std::string SSL_PEM_FILE("../utils/sslkey.pem");
    static const std::string SERVICES_CONFIG_FILE("../tests/config/testservices.conf");
#endif


/// generates chunked POST requests for testing purposes
class ChunkedPostRequestSender : 
    public std::enable_shared_from_this<ChunkedPostRequestSender>,
    private boost::noncopyable
{
public:
    /**
     * creates new ChunkedPostRequestSender objects
     *
     * @param tcp_conn TCP connection used to send the file
     * @param resource
     */
    static inline std::shared_ptr<ChunkedPostRequestSender>
        create(const pion::tcp::connection_ptr& tcp_conn, const std::string& resource)
    {
        return std::shared_ptr<ChunkedPostRequestSender>(new ChunkedPostRequestSender(tcp_conn, resource));
    }
    
    ~ChunkedPostRequestSender() {
        for (m_chunk_iterator = m_chunks.begin(); m_chunk_iterator != m_chunks.end(); ++m_chunk_iterator) {
            delete[] m_chunk_iterator->second;
        }
    }
    
    void send(void);

    void addChunk(size_t size, const char* ptr) {
        char* localCopy = new char[size];
        memcpy(localCopy, ptr, size);
        m_chunks.push_back(Chunk(size, localCopy));
        m_chunk_iterator = m_chunks.begin();
    }

protected:

    ChunkedPostRequestSender(const pion::tcp::connection_ptr& tcp_conn,
                             const std::string& resource);
    
    /**
     * handler called after a send operation has completed
     *
     * @param write_error error status from the last write operation
     * @param bytes_written number of bytes sent by the last write operation
     */
    void handle_write(const asio::error_code& write_error,
                     std::size_t bytes_written);

private:

    typedef std::pair<size_t, char*> Chunk;

    /// primary logging interface used by this class
    pion::logger                        m_logger;

    /// the chunks we are sending
    std::vector<Chunk>                      m_chunks;
    
    std::vector<Chunk>::const_iterator      m_chunk_iterator;

    /// the HTTP request writer we are using
    pion::http::request_writer_ptr         m_writer;
};

ChunkedPostRequestSender::ChunkedPostRequestSender(const pion::tcp::connection_ptr& tcp_conn,
                                                   const std::string& resource)
    : m_logger(PION_GET_LOGGER("pion.ChunkedPostRequestSender")),
    m_writer(pion::http::request_writer::create(tcp_conn))
{
    m_writer->get_request().set_method("POST");
    m_writer->get_request().set_resource(resource);
    m_writer->get_request().set_chunks_supported(true);
    m_chunk_iterator = m_chunks.begin();
}

void ChunkedPostRequestSender::send(void)
{
    if (m_chunk_iterator == m_chunks.end()) {
        m_writer->send_final_chunk(std::bind(&ChunkedPostRequestSender::handle_write,
                                             shared_from_this(),
                                             std::placeholders::_1,
                                             std::placeholders::_2));
        return;
    }

    // write the current chunk
    m_writer->write_no_copy(m_chunk_iterator->second, m_chunk_iterator->first);
    
    if (++m_chunk_iterator == m_chunks.end()) {
        m_writer->send_final_chunk(std::bind(&ChunkedPostRequestSender::handle_write,
                                             shared_from_this(),
                                             std::placeholders::_1,
                                             std::placeholders::_2));
    } else {
        m_writer->send_chunk(std::bind(&ChunkedPostRequestSender::handle_write,
                                        shared_from_this(),
                                        std::placeholders::_1,
                                        std::placeholders::_2));
    }
}

void ChunkedPostRequestSender::handle_write(const asio::error_code& write_error,
                                           std::size_t bytes_written)
{
    (void)bytes_written;

    if (write_error) {
        // encountered error sending request data
        m_writer->get_connection()->set_lifecycle(pion::tcp::connection::LIFECYCLE_CLOSE); // make sure it will get closed
        PION_LOG_ERROR(m_logger, "Error sending chunked request (" << write_error.message() << ')');
    } else {
        // request data sent OK
        
        if (m_chunk_iterator == m_chunks.end()) {
            PION_LOG_DEBUG(m_logger, "Sent " << bytes_written << " bytes (finished)");
        } else {
            PION_LOG_DEBUG(m_logger, "Sent " << bytes_written << " bytes");
            m_writer->clear();
            send();
        }
    }
}

// sample passwords and corresponding hashes
static const std::string PASSWORD_1 = "Whatever";
static const std::string SHA_1_HASH_OF_PASSWORD_1 = "c916e71d733d06cb77a4775de5f77fd0b480a7e8";
static const std::string SHA_256_HASH_OF_PASSWORD_1 = "e497135e5c9481c39bc35e62927bc53b7cad4ed3193f1831e63ee66973b970b1";
static const std::string PASSWORD_2 = "Open, Sesame!";
static const std::string SHA_1_HASH_OF_PASSWORD_2 = "a46a5895a829d1fedc9bd4ef1801a2c99fd4f044";
static const std::string SHA_256_HASH_OF_PASSWORD_2 = "b620fa9f74d0173f84c8f27116766ef426d9beb0f38534555655a9e80a03a8c5";

///
/// WebServerTests_F: fixture used for running web server tests
/// 
class WebServerTests_F {
public:
    
    // default constructor & destructor
    WebServerTests_F() : m_scheduler(), m_server(m_scheduler) {
        // initialize the list of directories in which to look for plug-ins
        plugin::reset_plugin_directories();
#ifndef PION_STATIC_LINKING
        plugin::add_plugin_directory(PATH_TO_PLUGINS);
#endif
    }
    ~WebServerTests_F() {
        m_server.stop();
        m_scheduler.shutdown();
    }
    
    /**
     * sends a request to the local HTTP server
     *
     * @param http_stream open stream to send the request via
     * @param resource name of the HTTP resource to request
     * @param content_length bytes available in the response, if successful
     */
    inline unsigned int sendRequest(asio::ip::tcp::iostream& http_stream,
                                    const std::string& resource,
                                    unsigned long& content_length)
    {
        const std::regex regex_get_response_code("^HTTP/1\\.1\\s(\\d+)\\s[^]*");
        const std::regex regex_response_header("^[A-Za-z0-9_-]+:\\s[^]*");
        const std::regex regex_content_length_header("^Content-Length:\\s(\\d+)[^]*", std::regex::icase);
        const std::regex regex_response_end("^\\s*$");

        // send HTTP request to the server
        http_stream << "GET " << resource << " HTTP/1.1" << http::types::STRING_CRLF << http::types::STRING_CRLF;
        http_stream.flush();
                
        // receive response from the server
        std::string rsp_line;
        std::smatch rx_matches;
        unsigned int response_code = 0;
        BOOST_REQUIRE(std::getline(http_stream, rsp_line));
        BOOST_REQUIRE(std::regex_match(rsp_line, rx_matches, regex_get_response_code));
        BOOST_REQUIRE(rx_matches.size() == 2);

        // extract response status code
        response_code = std::stoul(rx_matches[1]);
        BOOST_REQUIRE(response_code != 0);
        
        // read response headers
        content_length = 0;
        while (true) {
            BOOST_REQUIRE(std::getline(http_stream, rsp_line));
            // check for end of response headers (empty line)
            if (std::regex_match(rsp_line, rx_matches, regex_response_end))
                break;
            // check validity of response header
            BOOST_REQUIRE(std::regex_match(rsp_line, regex_response_header));
            // check for content-length response header
            if (std::regex_match(rsp_line, rx_matches, regex_content_length_header)) {
                if (rx_matches.size() == 2)
                    content_length = std::stoul(rx_matches[1]);
            }
        }
        
        return response_code;
    }
    
    /**
     * checks the local HTTP server's response code & validity using HelloService
     */
    inline void checkWebServerResponseCode(void) {
        // load simple Hello service and start the server
        m_server.load_service("/hello", "HelloService");
        m_server.start();
        
        // open a connection
        asio::ip::tcp::endpoint http_endpoint(asio::ip::address::from_string("127.0.0.1"), m_server.get_port());
        asio::ip::tcp::iostream http_stream(http_endpoint);
        
        // send valid request to the server
        unsigned int response_code;
        unsigned long content_length = 0;
        response_code = sendRequest(http_stream, "/hello", content_length);
        BOOST_CHECK(response_code == 200);
        BOOST_CHECK(content_length > 0);
        if (content_length > 0) {
            std::unique_ptr<char[]> content_buf(new char[content_length+1]);
            BOOST_CHECK(http_stream.read(content_buf.get(), content_length));
        }
        
        // send invalid request to the server
        response_code = sendRequest(http_stream, "/doesnotexist", content_length);
        BOOST_CHECK(response_code == 404);
    }
    
    /**
     * checks response content validity for the local HTTP server
     *
     * @param http_stream open stream to send the request via
     * @param resource name of the HTTP resource to request
     * @param content_regex regex that the response content should match
     */
    inline void checkWebServerResponseContent(asio::ip::tcp::iostream& http_stream,
                                              const std::string& resource,
                                              const std::regex& content_regex,
                                              unsigned int expectedResponseCode = 200)
    {
        // send valid request to the server
        unsigned int response_code;
        unsigned long content_length = 0;
        response_code = sendRequest(http_stream, resource, content_length);
        BOOST_CHECK(response_code == expectedResponseCode);
        BOOST_REQUIRE(content_length > 0);
        
        // read in the response content
        std::unique_ptr<char[]> content_buf(new char[content_length+1]);
        BOOST_CHECK(http_stream.read(content_buf.get(), content_length));
        content_buf[content_length] = '\0';
        
        // check the response content
        BOOST_CHECK(std::regex_match(content_buf.get(), content_regex));
    }

    /**
     * checks response content validity for the local HTTP server
     *
     * @param service name of the web service to load and query
     * @param resource name of the HTTP resource to request
     * @param content_regex regex that the response content should match
     */
    inline void checkWebServerResponseContent(const std::string& service,
                                              const std::string& resource,
                                              const std::regex& content_regex,
                                              unsigned int expectedResponseCode = 200)
    {
        // load specified service and start the server
        m_server.load_service(resource, service);
        m_server.start();
        
        // open a connection
        asio::ip::tcp::endpoint http_endpoint(asio::ip::address::from_string("127.0.0.1"), m_server.get_port());
        asio::ip::tcp::iostream http_stream(http_endpoint);

        // send request and check response
        checkWebServerResponseContent(http_stream, resource, content_regex, expectedResponseCode);
    }
    
    /**
     * checks if we can successfully send and receive HTTP messages
     * 
     * @param tcp_conn open TCP connection to use for the tests
     */
    inline void checkSendAndReceiveMessages(pion::tcp::connection& tcp_conn) {
        // send valid request to the server
        http::request http_request("/hello");
        asio::error_code error_code;
        http_request.send(tcp_conn, error_code);
        BOOST_REQUIRE(! error_code);

        // receive the response from the server
        http::response http_response(http_request);
        http_response.receive(tcp_conn, error_code);
        BOOST_REQUIRE(! error_code);
        
        // check that the response is OK
        std::regex hello_regex("[^]*Hello\\sWorld[^]*");
        BOOST_REQUIRE(http_response.get_status_code() == 200);
        BOOST_REQUIRE(http_response.get_content_length() > 0);
        BOOST_REQUIRE(std::regex_match(http_response.get_content(), hello_regex));
                
        // send invalid request to the server
        http_request.set_resource("/doesnotexist");
        http_request.send(tcp_conn, error_code);
        BOOST_REQUIRE(! error_code);
        http_response.receive(tcp_conn, error_code);
        BOOST_REQUIRE(! error_code);
        BOOST_CHECK_EQUAL(http_response.get_status_code(), 404U);
    }
    
    inline asio::io_service& get_io_service(void) { return m_scheduler.get_io_service(); }
    
    single_service_scheduler	m_scheduler;
	http::plugin_server			m_server;
};


// plugin_server Test Cases

BOOST_FIXTURE_TEST_SUITE(WebServerTests_S, WebServerTests_F)

BOOST_AUTO_TEST_CASE(checkWebServerIsListening) {
    BOOST_CHECK(! m_server.is_listening());
    m_server.start();
    BOOST_CHECK(m_server.is_listening());
    m_server.stop();
    BOOST_CHECK(! m_server.is_listening());
}

BOOST_AUTO_TEST_CASE(checkWebServerRespondsProperly) {
    checkWebServerResponseCode();
}

BOOST_AUTO_TEST_CASE(checkSendRequestsAndReceiveResponses) {
    // load simple Hello service and start the server
    m_server.load_service("/hello", "HelloService");
    m_server.start();
    
    // open a connection
    pion::tcp::connection tcp_conn(get_io_service());
    tcp_conn.set_lifecycle(pion::tcp::connection::LIFECYCLE_KEEPALIVE);
    asio::error_code error_code;
    error_code = tcp_conn.connect(asio::ip::address::from_string("127.0.0.1"), m_server.get_port());
    BOOST_REQUIRE(! error_code);
    
    checkSendAndReceiveMessages(tcp_conn);
}

BOOST_AUTO_TEST_CASE(checkSendRequestsAndReceiveResponseLeftoverConnection) {
    // load simple Hello service and start the server
    m_server.load_service("/hello", "HelloService");
    m_server.start();
    
    // open a connection
    pion::tcp::connection tcp_conn(get_io_service());
    tcp_conn.set_lifecycle(pion::tcp::connection::LIFECYCLE_KEEPALIVE);
    asio::error_code error_code;
    error_code = tcp_conn.connect(asio::ip::address::from_string("127.0.0.1"), m_server.get_port());
    BOOST_REQUIRE(! error_code);
    
    // send valid request to the server
    http::request http_request("/hello");
    http_request.send(tcp_conn, error_code);
    BOOST_REQUIRE(! error_code);
    
    // receive the response from the server
    http::response http_response(http_request);
    http_response.receive(tcp_conn, error_code);
    BOOST_REQUIRE(! error_code);
    BOOST_CHECK_EQUAL(http_response.get_header(http::types::HEADER_CONNECTION), "Keep-Alive");
    
    // check that the response is OK
    std::regex hello_regex("[^]*Hello\\sWorld[^]*");
    BOOST_REQUIRE(http_response.get_status_code() == 200);
    BOOST_REQUIRE(http_response.get_content_length() > 0);
    BOOST_REQUIRE(std::regex_match(http_response.get_content(), hello_regex));
    
    // shut down the server while the connection is still alive and waiting for data
    m_server.stop();
}

BOOST_AUTO_TEST_CASE(checkSendRequestAndReceiveResponseFromEchoService) {
    m_server.load_service("/echo", "EchoService");
    m_server.start();

    // open a connection
    tcp::connection_ptr tcp_conn(new pion::tcp::connection(get_io_service()));
    tcp_conn->set_lifecycle(pion::tcp::connection::LIFECYCLE_KEEPALIVE);
    asio::error_code error_code;
    error_code = tcp_conn->connect(asio::ip::address::from_string("127.0.0.1"), m_server.get_port());
    BOOST_REQUIRE(!error_code);

    pion::http::request_writer_ptr writer(pion::http::request_writer::create(tcp_conn));
    writer->get_request().set_method("POST");
    writer->get_request().set_resource("/echo");

    writer << "junk";
    writer->send();

    // receive the response from the server
    http::response http_response(writer->get_request());
    http_response.receive(*tcp_conn, error_code);
    BOOST_CHECK(!error_code);

    // check that the response is OK
    BOOST_CHECK(http_response.get_status_code() == 200);
    BOOST_CHECK(http_response.get_content_length() > 0);

    // check the post content of the request, by parsing it out of the post content of the response
    std::regex post_content("[^]*\\[POST Content]\\s*junk[^]*");
    BOOST_CHECK(std::regex_match(http_response.get_content(), post_content));
}

BOOST_AUTO_TEST_CASE(checkRedirectHelloServiceToEchoService) {
    m_server.load_service("/hello", "HelloService");
    m_server.load_service("/echo", "EchoService");
    m_server.start();

    // open a connection
    asio::ip::tcp::endpoint http_endpoint(asio::ip::address::from_string("127.0.0.1"), m_server.get_port());
    asio::ip::tcp::iostream http_stream(http_endpoint);

    // send a request to /hello and check that the response is from HelloService
    checkWebServerResponseContent(http_stream, "/hello", std::regex("[^]*Hello\\sWorld[^]*"));

    m_server.add_redirect("/hello", "/echo");

    // send a request to /hello and check that the response is from EchoService
    checkWebServerResponseContent(http_stream, "/hello", std::regex("[^]*\\[Request\\sEcho\\][^]*"));
}

BOOST_AUTO_TEST_CASE(checkOriginalResourceAvailableAfterRedirect) {
    m_server.load_service("/hello", "HelloService");
    m_server.load_service("/echo", "EchoService");
    m_server.start();

    // open a connection
    asio::ip::tcp::endpoint http_endpoint(asio::ip::address::from_string("127.0.0.1"), m_server.get_port());
    asio::ip::tcp::iostream http_stream(http_endpoint);

    m_server.add_redirect("/hello", "/echo");

    // send a request to /hello and check the reported values of the original resource and the delivered resource
    std::regex regex_expected_content("[^]*Resource\\soriginally\\srequested:\\s/hello[^]*Resource\\sdelivered:\\s/echo[^]*");
    checkWebServerResponseContent(http_stream, "/hello", regex_expected_content);
}

BOOST_AUTO_TEST_CASE(checkRecursiveRedirect) {
    m_server.load_service("/hello", "HelloService");
    m_server.load_service("/echo", "EchoService");
    m_server.load_service("/cookie", "CookieService");
    m_server.start();

    // open a connection
    asio::ip::tcp::endpoint http_endpoint(asio::ip::address::from_string("127.0.0.1"), m_server.get_port());
    asio::ip::tcp::iostream http_stream(http_endpoint);

    m_server.add_redirect("/hello", "/echo");
    m_server.add_redirect("/echo", "/cookie");

    // send a request to /hello and check that the response is from CookieService
    checkWebServerResponseContent(http_stream, "/hello", std::regex("[^]*<html>[^]*Cookie\\sService[^]*</html>[^]*"));
}

BOOST_AUTO_TEST_CASE(checkCircularRedirect) {
    m_server.load_service("/hello", "HelloService");
    m_server.load_service("/cookie", "CookieService");
    m_server.load_service("/echo", "EchoService");
    m_server.start();

    // open a connection
    asio::ip::tcp::endpoint http_endpoint(asio::ip::address::from_string("127.0.0.1"), m_server.get_port());
    asio::ip::tcp::iostream http_stream(http_endpoint);

    // set up a circular set of redirects
    m_server.add_redirect("/hello", "/echo");
    m_server.add_redirect("/echo", "/cookie");
    m_server.add_redirect("/cookie", "/hello");

    // send request and check that server returns expected status code and error message
    checkWebServerResponseContent(http_stream, "/hello",
                                  std::regex("[^]*Maximum number of redirects[^]*exceeded[^]*"),
                                  http::types::RESPONSE_CODE_SERVER_ERROR);
}

BOOST_AUTO_TEST_CASE(checkSendChunkedRequestAndReceiveResponse) {
    m_server.load_service("/echo", "EchoService");
    m_server.start();

    // open a connection
    tcp::connection_ptr tcp_conn(new pion::tcp::connection(get_io_service()));
    tcp_conn->set_lifecycle(pion::tcp::connection::LIFECYCLE_KEEPALIVE);
    asio::error_code error_code;
    error_code = tcp_conn->connect(asio::ip::address::from_string("127.0.0.1"), m_server.get_port());
    BOOST_REQUIRE(!error_code);

    std::shared_ptr<ChunkedPostRequestSender> sender = ChunkedPostRequestSender::create(tcp_conn, "/echo");
    sender->addChunk(5, "klmno");
    sender->addChunk(4, "1234");
    sender->addChunk(10, "abcdefghij");
    sender->send();

    // receive the response from the server
    http::response http_response("GET");
    http_response.receive(*tcp_conn, error_code);
    BOOST_CHECK(!error_code);

    // check that the response is OK
    BOOST_CHECK(http_response.get_status_code() == 200);
    BOOST_CHECK(http_response.get_content_length() > 0);

    // check the content length of the request, by parsing it out of the post content of the response
    std::regex content_length_of_request("[^]*Content length\\: 19[^]*");
    BOOST_CHECK(std::regex_match(http_response.get_content(), content_length_of_request));

    // check the post content of the request, by parsing it out of the post content of the response
    std::regex post_content_of_request("[^]*\\[POST Content]\\s*klmno1234abcdefghij[^]*");
    BOOST_CHECK(std::regex_match(http_response.get_content(), post_content_of_request));
}

BOOST_AUTO_TEST_CASE(checkSendChunkedRequestWithOneChunkAndReceiveResponse) {
    m_server.load_service("/echo", "EchoService");
    m_server.start();

    // open a connection
    tcp::connection_ptr tcp_conn(new pion::tcp::connection(get_io_service()));
    tcp_conn->set_lifecycle(pion::tcp::connection::LIFECYCLE_KEEPALIVE);
    asio::error_code error_code;
    error_code = tcp_conn->connect(asio::ip::address::from_string("127.0.0.1"), m_server.get_port());
    BOOST_REQUIRE(!error_code);

    std::shared_ptr<ChunkedPostRequestSender> sender = ChunkedPostRequestSender::create(tcp_conn, "/echo");
    sender->addChunk(10, "abcdefghij");
    sender->send();

    // receive the response from the server
    http::response http_response("GET");
    http_response.receive(*tcp_conn, error_code);
    BOOST_CHECK(!error_code);

    // check that the response is OK
    BOOST_CHECK(http_response.get_status_code() == 200);
    BOOST_CHECK(http_response.get_content_length() > 0);

    // check the post content of the request, by parsing it out of the post content of the response
    std::regex post_content("[^]*\\[POST Content]\\s*abcdefghij[^]*");
    BOOST_CHECK(std::regex_match(http_response.get_content(), post_content));
}

BOOST_AUTO_TEST_CASE(checkSendChunkedRequestWithNoChunksAndReceiveResponse) {
    m_server.load_service("/echo", "EchoService");
    m_server.start();

    // open a connection
    tcp::connection_ptr tcp_conn(new pion::tcp::connection(get_io_service()));
    tcp_conn->set_lifecycle(pion::tcp::connection::LIFECYCLE_KEEPALIVE);
    asio::error_code error_code;
    error_code = tcp_conn->connect(asio::ip::address::from_string("127.0.0.1"), m_server.get_port());
    BOOST_REQUIRE(!error_code);

    std::shared_ptr<ChunkedPostRequestSender> sender = ChunkedPostRequestSender::create(tcp_conn, "/echo");
    sender->send();

    // receive the response from the server
    http::response http_response("GET");
    http_response.receive(*tcp_conn, error_code);
    BOOST_CHECK(!error_code);

    // check that the response is OK
    BOOST_CHECK(http_response.get_status_code() == 200);
    BOOST_CHECK(http_response.get_content_length() > 0);

    // check the content length of the request, by parsing it out of the post content of the response
    std::regex content_length_of_request("[^]*Content length\\: 0[^]*");
    BOOST_CHECK(std::regex_match(http_response.get_content(), content_length_of_request));
}

#ifdef PION_HAVE_SSL
BOOST_AUTO_TEST_CASE(checkSendRequestsAndReceiveResponsesUsingSSL) {
    // load simple Hello service and start the server
    m_server.set_ssl_key_file(SSL_PEM_FILE);
    m_server.load_service("/hello", "HelloService");
    m_server.start();

    // open a connection
    pion::tcp::connection tcp_conn(get_io_service(), true);
    tcp_conn.set_lifecycle(pion::tcp::connection::LIFECYCLE_KEEPALIVE);
    asio::error_code error_code;
    error_code = tcp_conn.connect(asio::ip::address::from_string("127.0.0.1"), m_server.get_port());
    BOOST_REQUIRE(! error_code);
    error_code = tcp_conn.handshake_client();
    BOOST_REQUIRE(! error_code);

    checkSendAndReceiveMessages(tcp_conn);
}

BOOST_AUTO_TEST_CASE(checkSendRequestsAndReceiveResponseLeftoverConnectionUsingSSL) {
    // load simple Hello service and start the server
    m_server.set_ssl_key_file(SSL_PEM_FILE);
    m_server.load_service("/hello", "HelloService");
    m_server.start();
    
    // open a connection
    pion::tcp::connection tcp_conn(get_io_service(), true);
    tcp_conn.set_lifecycle(pion::tcp::connection::LIFECYCLE_KEEPALIVE);
    asio::error_code error_code;
    error_code = tcp_conn.connect(asio::ip::address::from_string("127.0.0.1"), m_server.get_port());
    BOOST_REQUIRE(! error_code);
    error_code = tcp_conn.handshake_client();
    BOOST_REQUIRE(! error_code);
    
    // send valid request to the server
    http::request http_request("/hello");
    http_request.send(tcp_conn, error_code);
    BOOST_REQUIRE(! error_code);
    
    // receive the response from the server
    http::response http_response(http_request);
    http_response.receive(tcp_conn, error_code);
    BOOST_REQUIRE(! error_code);
    BOOST_CHECK_EQUAL(http_response.get_header(http::types::HEADER_CONNECTION), "Keep-Alive");
    
    // check that the response is OK
    std::regex hello_regex("[^]*Hello\\sWorld[^]*");
    BOOST_REQUIRE(http_response.get_status_code() == 200);
    BOOST_REQUIRE(http_response.get_content_length() > 0);
    BOOST_REQUIRE(std::regex_match(http_response.get_content(), hello_regex));
    
    // shut down the server while the connection is still alive and waiting for data
    m_server.stop();
}
#endif

BOOST_AUTO_TEST_CASE(checkHelloServiceResponseContent) {
    checkWebServerResponseContent("HelloService", "/hello",
                                  std::regex("[^]*Hello\\sWorld[^]*"));
}

BOOST_AUTO_TEST_CASE(checkCookieServiceResponseContent) {
    checkWebServerResponseContent("CookieService", "/cookie",
                                  std::regex("[^]*<html>[^]*Cookie\\sService[^]*</html>[^]*"));
}

BOOST_AUTO_TEST_CASE(checkEchoServiceResponseContent) {
    checkWebServerResponseContent("EchoService", "/echo",
                                  std::regex("[^]*\\[Request\\sEcho\\][^]*\\[POST\\sContent\\][^]*"));
}

BOOST_AUTO_TEST_CASE(checkLogServiceResponseContent) {
#if defined(PION_USE_LOG4CXX) || defined(PION_USE_LOG4CPLUS) || defined(PION_USE_LOG4CPP)
    // make sure that the log level is high enough so that the entry will be recorded
    pion::logger log_ptr = PION_GET_LOGGER("pion");
    PION_LOG_SETLEVEL_INFO(log_ptr);
    // make sure that the log service includes an entry for loading itself
    checkWebServerResponseContent("LogService", "/log",
                                  std::regex("[^]*Loaded[^]*plug-in[^]*\\(/log\\):\\sLogService[^]*"));
    // bump the log level back down when we are done with the test
    PION_LOG_SETLEVEL_WARN(log_ptr);
#elif defined(PION_DISABLE_LOGGING)
    checkWebServerResponseContent("LogService", "/log",
                                  std::regex("[^]*Logging\\sis\\sdisabled[^]*"));
#else
    checkWebServerResponseContent("LogService", "/log",
                                  std::regex("[^]*Using\\sostream\\slogging[^]*"));
#endif
}

#if defined(PION_USE_LOG4CPLUS)
BOOST_AUTO_TEST_CASE(checkCircularBufferAppender) {
	// Create a circular buffer appender and add it:
	pion::log_appender_ptr appender(new pion::circular_buffer_appender);
	appender->setName("CircularBufferAppender");
	pion::logger::getRoot().addAppender(appender);

	// Log an error so we can check if it gets appended:
	pion::logger log_ptr = PION_GET_LOGGER("pion");
	PION_LOG_ERROR(log_ptr, "X happened");

	// Get a reference to the log event buffer.
	pion::log_appender_ptr cba_ptr = pion::logger::getRoot().getAppender("CircularBufferAppender");
	const pion::circular_buffer_appender& cba = dynamic_cast<const pion::circular_buffer_appender&>(*cba_ptr.get());
	const pion::circular_buffer_appender::LogEventBuffer& events = cba.getLogIterator();
	pion::circular_buffer_appender::LogEventBuffer::const_iterator it;

	// Check that the log event buffer has exactly one event, with the expected message:
	it = events.begin();
	BOOST_REQUIRE(it != events.end());
	BOOST_CHECK_EQUAL(it->getMessage(), "X happened");
	BOOST_CHECK(++it == events.end());

	// Log a second error:
	PION_LOG_ERROR(log_ptr, "Y happened");

	// Check that the log event buffer has exactly two events, with the expected messages:
	it = events.begin();
	BOOST_REQUIRE(it != events.end());
	BOOST_CHECK_EQUAL(it->getMessage(), "X happened");
	BOOST_REQUIRE((++it) != events.end());
	BOOST_CHECK_EQUAL(it->getMessage(), "Y happened");
	BOOST_CHECK(++it == events.end());

	// Now remove the appender and log a third error:
	pion::logger::getRoot().removeAppender(appender);
	PION_LOG_ERROR(log_ptr, "Z happened");

	// Check that the log event buffer still has only the same two events:
	it = events.begin();
	BOOST_REQUIRE(it != events.end());
	BOOST_CHECK_EQUAL(it->getMessage(), "X happened");
	BOOST_REQUIRE((++it) != events.end());
	BOOST_CHECK_EQUAL(it->getMessage(), "Y happened");
	BOOST_CHECK(++it == events.end());
}
#endif

#ifndef PION_STATIC_LINKING
BOOST_AUTO_TEST_CASE(checkAllowNothingServiceResponseContent) {
    checkWebServerResponseContent("AllowNothingService", "/deny",
                                  std::regex("[^]*No, you can't[^]*"),
                                  http::types::RESPONSE_CODE_METHOD_NOT_ALLOWED);
}
#endif // PION_STATIC_LINKING

BOOST_AUTO_TEST_CASE(checkFileServiceResponseContent) {
    // load multiple services and start the server
    try {
        m_server.load_service_config(SERVICES_CONFIG_FILE);
    } catch (error::directory_not_found&) {}
    m_server.start();
    
    // open a connection
    asio::ip::tcp::endpoint http_endpoint(asio::ip::address::from_string("127.0.0.1"), m_server.get_port());
    asio::ip::tcp::iostream http_stream(http_endpoint);
    
    // send request and check response (index page)
    const std::regex index_page_regex("[^]*<html>[^]*Test\\sWebsite[^]*</html>[^]*");
    checkWebServerResponseContent(http_stream, "/" , index_page_regex);
    checkWebServerResponseContent(http_stream, "/index.html" , index_page_regex);

    // send request and check response (copy of docs index page generated by doxygen)
    const std::regex doc_index_regex("[^]*<html>[^]*pion-[^]*Documentation[^]*</html>[^]*");
    checkWebServerResponseContent(http_stream, "/doc/index.html" , doc_index_regex);
}

BOOST_AUTO_TEST_CASE(checkPionUserPasswordSanity) {
    const std::string clear_pw("deadmeat");
    user u("test-user");
    u.set_password(clear_pw);
    BOOST_CHECK(u.match_password(clear_pw));

#ifdef PION_HAVE_SSL
    std::string encrypted_pw = u.get_password();
    BOOST_CHECK_EQUAL(encrypted_pw.size(), static_cast<unsigned int>(SHA256_DIGEST_LENGTH * 2));
    BOOST_CHECK(clear_pw != encrypted_pw);
    
    u.set_password_hash(encrypted_pw);
    BOOST_CHECK_EQUAL(encrypted_pw, u.get_password());   // should still be identical
    BOOST_CHECK(u.match_password(clear_pw));
#endif
}

BOOST_AUTO_TEST_CASE(checkMatchPassword) {
    user u("test-user", PASSWORD_1);
    BOOST_CHECK(u.match_password(PASSWORD_1));
    BOOST_CHECK(! u.match_password(PASSWORD_2));
}

#ifdef PION_HAVE_SSL
BOOST_AUTO_TEST_CASE(checkSetPasswordCreatesSha256PasswordHash) {
    user u("test-user");
    u.set_password(PASSWORD_1);
    BOOST_CHECK_EQUAL(u.get_password(), SHA_256_HASH_OF_PASSWORD_1);
}

BOOST_AUTO_TEST_CASE(checkNewUserGetsSha256PasswordHash) {
    user u("test-user", PASSWORD_1);
    BOOST_CHECK_EQUAL(u.get_password(), SHA_256_HASH_OF_PASSWORD_1);
}

BOOST_AUTO_TEST_CASE(checkAddUserCreatesSha256PasswordHash) {
    user_manager userManager;
    BOOST_CHECK(userManager.add_user("test-user", PASSWORD_1));
    user_ptr u = userManager.get_user("test-user");
    BOOST_CHECK_EQUAL(u->get_password(), SHA_256_HASH_OF_PASSWORD_1);
}

BOOST_AUTO_TEST_CASE(checkUpdateUserCreatesSha256PasswordHash) {
    user_manager userManager;
    BOOST_REQUIRE(userManager.add_user("test-user", PASSWORD_1));

    BOOST_CHECK(userManager.update_user("test-user", PASSWORD_2));
    user_ptr u = userManager.get_user("test-user");
    BOOST_CHECK_EQUAL(u->get_password(), SHA_256_HASH_OF_PASSWORD_2);
}

BOOST_AUTO_TEST_CASE(checkAddUserHashWorksWithSha256PasswordHash) {
    user_manager userManager;
    BOOST_CHECK(userManager.add_user_hash("test-user", SHA_256_HASH_OF_PASSWORD_1));
    user_ptr u = userManager.get_user("test-user");
    BOOST_CHECK(u->match_password(PASSWORD_1));
}

// Check that SHA-1 (legacy) password hashes still work.
BOOST_AUTO_TEST_CASE(checkSha1PasswordHashStillWorks) {
    user u("test-user");
    u.set_password_hash(SHA_1_HASH_OF_PASSWORD_1);
    BOOST_CHECK(u.match_password(PASSWORD_1));
}

BOOST_AUTO_TEST_CASE(checkAddUserHashWorksWithLegacySha1PasswordHash) {
    user_manager userManager;
    BOOST_CHECK(userManager.add_user_hash("test-user", SHA_1_HASH_OF_PASSWORD_2));
    user_ptr u = userManager.get_user("test-user");
    BOOST_CHECK(u->match_password(PASSWORD_2));
}
#endif

BOOST_AUTO_TEST_CASE(checkBasicAuthServiceFailure) {
    m_server.load_service("/auth", "EchoService");
    user_manager_ptr userManager(new user_manager());
    http::auth_ptr my_auth_ptr(new http::basic_auth(userManager));
    m_server.set_authentication(my_auth_ptr);
    my_auth_ptr->add_restrict("/auth");
    my_auth_ptr->add_user("mike", "123456");
    m_server.start();
    
    // open a connection
    tcp::connection_ptr tcp_conn(new pion::tcp::connection(get_io_service()));
    tcp_conn->set_lifecycle(pion::tcp::connection::LIFECYCLE_KEEPALIVE);
    asio::error_code error_code;
    error_code = tcp_conn->connect(asio::ip::address::from_string("127.0.0.1"), m_server.get_port());
    BOOST_REQUIRE(!error_code);
    
    pion::http::request_writer_ptr writer(pion::http::request_writer::create(tcp_conn));
    writer->get_request().set_method("POST");
    writer->get_request().set_resource("/auth/something/somewhere");
    
    writer << "junk";
    writer->send();
    
    // receive the response from the server
    http::response http_response(writer->get_request());
    http_response.receive(*tcp_conn, error_code);
    BOOST_CHECK(!error_code);
    
    // check that the response is RESPONSE_CODE_UNAUTHORIZED
    BOOST_CHECK(http_response.get_status_code() == http::types::RESPONSE_CODE_UNAUTHORIZED);
    BOOST_CHECK(http_response.get_content_length() > 0);
    
    // check the post content of the request, by parsing it out of the post content of the response
    std::regex post_content("[^]*\\[POST Content]\\s*junk[^]*");
    BOOST_CHECK(!std::regex_match(http_response.get_content(), post_content));
}

BOOST_AUTO_TEST_CASE(checkBasicAuthServiceLogin) {
    m_server.load_service("/auth", "EchoService");
    user_manager_ptr userManager(new user_manager());
    http::auth_ptr my_auth_ptr(new http::basic_auth(userManager));
    m_server.set_authentication(my_auth_ptr);
    my_auth_ptr->add_restrict("/auth");
    my_auth_ptr->add_user("mike", "123456");
    m_server.start();
    
    // open a connection
    tcp::connection_ptr tcp_conn(new pion::tcp::connection(get_io_service()));
    tcp_conn->set_lifecycle(pion::tcp::connection::LIFECYCLE_KEEPALIVE);
    asio::error_code error_code;
    error_code = tcp_conn->connect(asio::ip::address::from_string("127.0.0.1"), m_server.get_port());
    BOOST_REQUIRE(!error_code);
    
    pion::http::request_writer_ptr writer(pion::http::request_writer::create(tcp_conn));
    writer->get_request().set_method("POST");
    writer->get_request().set_resource("/auth/something/somewhere");
    // add an authentication for "mike:123456"
    writer->get_request().add_header(http::types::HEADER_AUTHORIZATION, "Basic bWlrZToxMjM0NTY=");
    
    writer << "junk";
    writer->send();
    
    // receive the response from the server
    http::response http_response(writer->get_request());
    http_response.receive(*tcp_conn, error_code);
    BOOST_CHECK(!error_code);
    
    // check that the response is OK
    BOOST_CHECK(http_response.get_status_code() == 200);
    BOOST_CHECK(http_response.get_content_length() > 0);
    
    // check the post content of the request, by parsing it out of the post content of the response
    std::regex post_content("[^]*\\[POST Content]\\s*junk[^]*");
    BOOST_CHECK(std::regex_match(http_response.get_content(), post_content));
}

BOOST_AUTO_TEST_CASE(checkCookieAuthServiceFailure) {
    m_server.load_service("/auth", "EchoService");
    user_manager_ptr userManager(new user_manager());
    http::auth_ptr my_auth_ptr(new http::cookie_auth(userManager));
    m_server.set_authentication(my_auth_ptr);
    my_auth_ptr->add_restrict("/auth");
    my_auth_ptr->add_user("mike", "123456");
    m_server.start();

    // open a connection
    tcp::connection_ptr tcp_conn(new pion::tcp::connection(get_io_service()));
    tcp_conn->set_lifecycle(pion::tcp::connection::LIFECYCLE_KEEPALIVE);
    asio::error_code error_code;
    error_code = tcp_conn->connect(asio::ip::address::from_string("127.0.0.1"), m_server.get_port());
    BOOST_REQUIRE(!error_code);

    pion::http::request_writer_ptr writer(pion::http::request_writer::create(tcp_conn));
    writer->get_request().set_method("POST");
    writer->get_request().set_resource("/auth/something/somewhere");

    writer << "junk";
    writer->send();

    // receive the response from the server
    http::response http_response(writer->get_request());
    http_response.receive(*tcp_conn, error_code);
    BOOST_CHECK(!error_code);

    // check that the response is RESPONSE_CODE_UNAUTHORIZED
    BOOST_CHECK(http_response.get_status_code() == http::types::RESPONSE_CODE_UNAUTHORIZED);
    BOOST_CHECK(http_response.get_content_length() > 0);

    // check the post content of the request, by parsing it out of the post content of the response
    std::regex post_content("[^]*\\[POST Content]\\s*junk[^]*");
    BOOST_CHECK(!std::regex_match(http_response.get_content(), post_content));
}

BOOST_AUTO_TEST_CASE(checkCookieAuthServiceLogin) {
    m_server.load_service("/auth", "EchoService");
    user_manager_ptr userManager(new user_manager());
    http::auth_ptr my_auth_ptr(new http::cookie_auth(userManager));
    m_server.set_authentication(my_auth_ptr);
    my_auth_ptr->add_restrict("/auth");
    my_auth_ptr->add_user("mike", "123456");
    m_server.start();

    // open a login connection
    tcp::connection_ptr tcp_conn(new pion::tcp::connection(get_io_service()));
    tcp_conn->set_lifecycle(pion::tcp::connection::LIFECYCLE_KEEPALIVE);
    asio::error_code error_code;
    error_code = tcp_conn->connect(asio::ip::address::from_string("127.0.0.1"), m_server.get_port());
    BOOST_REQUIRE(!error_code);

    pion::http::request_writer_ptr writer(pion::http::request_writer::create(tcp_conn));
    writer->get_request().set_method("GET");
    // login as "mike:123456"
    writer->get_request().set_resource("/login?user=mike&pass=123456");

    //writer << "junk";
    writer->send();

    // receive the response from the server
    http::response http_response(writer->get_request());
    http_response.receive(*tcp_conn, error_code);
    BOOST_CHECK(!error_code);

    // check that the response is OK
    BOOST_CHECK(http_response.get_status_code() == 204);
    BOOST_CHECK(http_response.get_content_length() == 0);
    BOOST_CHECK(http_response.has_header(http::types::HEADER_SET_COOKIE));
    // get Cookies
    std::string cookie = http_response.get_header(http::types::HEADER_SET_COOKIE);

    // now try to connect to protected area using login cookie

    pion::http::request_writer_ptr writer2(pion::http::request_writer::create(tcp_conn));
    writer2->get_request().set_method("POST");
    writer2->get_request().set_resource("/auth/something/somewhere");
    // add an authentications for "mike:123456"
    writer2->get_request().add_header(http::types::HEADER_COOKIE,cookie);

    writer2 << "junk";
    writer2->send();

    // receive the response from the server
    http::response http_response2(writer2->get_request());
    http_response2.receive(*tcp_conn, error_code);
    BOOST_CHECK(!error_code);

    // check that the response is OK
    BOOST_CHECK(http_response2.get_status_code() == 200);
    BOOST_CHECK(http_response2.get_content_length() > 0);

    // check the post content of the request, by parsing it out of the post content of the response
    std::regex post_content("[^]*\\[POST Content]\\s*junk[^]*");
    BOOST_CHECK(std::regex_match(http_response2.get_content(), post_content));
}

BOOST_AUTO_TEST_SUITE_END()


#define BIG_BUF_SIZE (12 * 1024)

///
/// ContentResponseWithoutLengthTests_F: 
/// this uses a "big content buffer" to make sure that reading the response
/// content works across multiple packets (and asio_read_some() calls)
/// and when no content-length is specified (it should read through the end)
/// 
class ContentResponseWithoutLengthTests_F
    : public WebServerTests_F
{
public:
    // default constructor and destructor
    ContentResponseWithoutLengthTests_F() {
        // fill the buffer with non-random characters
        for (unsigned long n = 0; n < BIG_BUF_SIZE; ++n) {
            m_big_buf[n] = char(n);
        }
    }
    virtual ~ContentResponseWithoutLengthTests_F() {}
    
    /**
     * sends an HTTP response with content, but not content-length provided
     *
     * @param http_request_ptr the HTTP request to respond to
     * @param tcp_conn the TCP connection to send the response over
     */
    void sendResponseWithContentButNoLength(const http::request_ptr& http_request_ptr,
                                            const tcp::connection_ptr& tcp_conn)
    {
        // make sure it will get closed when finished
        tcp_conn->set_lifecycle(pion::tcp::connection::LIFECYCLE_CLOSE);
        
        // prepare the response headers
        http::response http_response(*http_request_ptr);
        http_response.set_do_not_send_content_length();
        
        // send the response headers
        asio::error_code error_code;
        http_response.send(*tcp_conn, error_code);
        BOOST_REQUIRE(! error_code);
        
        // send the content buffer
        tcp_conn->write(asio::buffer(m_big_buf, BIG_BUF_SIZE), error_code);
        BOOST_REQUIRE(! error_code);
        
        // finish (and close) the connection
        tcp_conn->finish();
    }
    
    /// reads in a HTTP response asynchronously
    void readAsyncResponse(const tcp::connection_ptr& tcp_conn)
    {
        http::request http_request("GET");
		http::response_reader_ptr my_reader_ptr(http::response_reader::create(tcp_conn, http_request,
                                                                    std::bind(&ContentResponseWithoutLengthTests_F::checkResponse2,
                                                                    this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
        my_reader_ptr->receive();
    }

    /// checks the validity of the HTTP response
    void checkResponse(const http::response& http_response)
    {
        BOOST_REQUIRE(http_response.get_status_code() == 200);
        BOOST_CHECK(! http_response.has_header(http::types::HEADER_CONTENT_LENGTH));
        BOOST_REQUIRE(http_response.get_content_length() == BIG_BUF_SIZE);
        BOOST_CHECK_EQUAL(memcmp(http_response.get_content(), m_big_buf, BIG_BUF_SIZE), 0);
    }
    
    /// checks the validity of the HTTP response
    void checkResponse2(http::response_ptr& http_response_ptr,
        tcp::connection_ptr& /* conn_ptr */, const asio::error_code& /* ec */)
    {
        checkResponse(*http_response_ptr);
        std::unique_lock<std::mutex> async_lock(m_mutex);
        m_async_test_finished.notify_one();
    }

    /// big data buffer used for the tests
    char                m_big_buf[BIG_BUF_SIZE];
    
    /// signaled after the async response check has finished
    std::condition_variable    m_async_test_finished;

    /// used to protect the asynchronous operations
    std::mutex        m_mutex;
};


// ContentResponseWithoutLengthTests_F Test Cases

BOOST_FIXTURE_TEST_SUITE(ContentResponseWithoutLengthTests_S, ContentResponseWithoutLengthTests_F)

BOOST_AUTO_TEST_CASE(checkSendContentWithoutLengthAndReceiveSyncResponse) {
    // startup the server 
    m_server.add_resource("/big", std::bind(&ContentResponseWithoutLengthTests_F::sendResponseWithContentButNoLength,
                                             this, std::placeholders::_1, std::placeholders::_2));
    m_server.start();
    
    // open a connection
    tcp::connection_ptr tcp_conn(new pion::tcp::connection(get_io_service()));
    asio::error_code error_code;
    error_code = tcp_conn->connect(asio::ip::address::from_string("127.0.0.1"), m_server.get_port());
    BOOST_REQUIRE(!error_code);

    // send an HTTP request
    http::request http_request("/big");
    http_request.send(*tcp_conn, error_code);
    BOOST_REQUIRE(! error_code);
    
    // receive the response from the server
    http::response http_response(http_request);
    http_response.receive(*tcp_conn, error_code);
    BOOST_REQUIRE(! error_code);
    
    // check that the response is OK
    checkResponse(http_response);
}

BOOST_AUTO_TEST_CASE(checkSendContentWithoutLengthAndReceiveAsyncResponse) {
    // startup the server 
    m_server.add_resource("/big", std::bind(&ContentResponseWithoutLengthTests_F::sendResponseWithContentButNoLength,
                                             this, std::placeholders::_1, std::placeholders::_2));
    m_server.start();
    
    // open a connection
    tcp::connection_ptr tcp_conn(new pion::tcp::connection(get_io_service()));
    asio::error_code error_code;
    error_code = tcp_conn->connect(asio::ip::address::from_string("127.0.0.1"), m_server.get_port());
    BOOST_REQUIRE(!error_code);
    
    // send an HTTP request
    std::unique_lock<std::mutex> async_lock(m_mutex);
    pion::http::request_writer_ptr writer_ptr(pion::http::request_writer::create(tcp_conn,
                                     std::bind(&ContentResponseWithoutLengthTests_F::readAsyncResponse,
                                                 this, tcp_conn)));
    writer_ptr->get_request().set_resource("/big");
    writer_ptr->send();
    
    // wait until the test is finished (and async calls have finished)
    m_async_test_finished.wait(async_lock);
}

BOOST_AUTO_TEST_SUITE_END()
