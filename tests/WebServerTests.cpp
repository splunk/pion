// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/PionConfig.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/scoped_array.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/filesystem.hpp>
#include <pion/PionPlugin.hpp>
#include <pion/PionScheduler.hpp>
#include <pion/net/HTTPRequest.hpp>
#include <pion/net/HTTPResponse.hpp>
#include <pion/net/HTTPRequestWriter.hpp>
#include <pion/net/HTTPResponseReader.hpp>
#include <pion/net/WebServer.hpp>
#include <pion/net/PionUser.hpp>
#include <pion/net/HTTPBasicAuth.hpp>
#include <pion/net/HTTPCookieAuth.hpp>


using namespace std;
using namespace pion;
using namespace pion::net;
using boost::asio::ip::tcp;

PION_DECLARE_PLUGIN(EchoService)
PION_DECLARE_PLUGIN(FileService)
PION_DECLARE_PLUGIN(HelloService)
PION_DECLARE_PLUGIN(LogService)
PION_DECLARE_PLUGIN(CookieService)

#if defined(PION_XCODE)
	static const std::string PATH_TO_PLUGINS(".");
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
	public boost::enable_shared_from_this<ChunkedPostRequestSender>,
	private boost::noncopyable
{
public:
	/**
	 * creates new ChunkedPostRequestSender objects
	 *
	 * @param tcp_conn TCP connection used to send the file
	 * @param resource
	 */
	static inline boost::shared_ptr<ChunkedPostRequestSender>
		create(pion::net::TCPConnectionPtr& tcp_conn, const std::string& resource) 
	{
		return boost::shared_ptr<ChunkedPostRequestSender>(new ChunkedPostRequestSender(tcp_conn, resource));
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

	ChunkedPostRequestSender(pion::net::TCPConnectionPtr& tcp_conn,
							 const std::string& resource);
	
	/**
	 * handler called after a send operation has completed
	 *
	 * @param write_error error status from the last write operation
	 * @param bytes_written number of bytes sent by the last write operation
	 */
	void handleWrite(const boost::system::error_code& write_error,
					 std::size_t bytes_written);

private:

	typedef std::pair<size_t, char*> Chunk;

	/// primary logging interface used by this class
	pion::PionLogger						m_logger;

	/// the chunks we are sending
	std::vector<Chunk>						m_chunks;
	
	std::vector<Chunk>::const_iterator		m_chunk_iterator;

	/// the HTTP request writer we are using
	pion::net::HTTPRequestWriterPtr			m_writer;
};

ChunkedPostRequestSender::ChunkedPostRequestSender(pion::net::TCPConnectionPtr& tcp_conn,
												   const std::string& resource)
	: m_logger(PION_GET_LOGGER("pion.ChunkedPostRequestSender")),
	m_writer(pion::net::HTTPRequestWriter::create(tcp_conn))
{
	m_writer->getRequest().setMethod("POST");
	m_writer->getRequest().setResource(resource);
	m_writer->getRequest().setChunksSupported(true);
	m_chunk_iterator = m_chunks.begin();
}

void ChunkedPostRequestSender::send(void)
{
	if (m_chunk_iterator == m_chunks.end()) {
		m_writer->sendFinalChunk(boost::bind(&ChunkedPostRequestSender::handleWrite,
											 shared_from_this(),
											 boost::asio::placeholders::error,
											 boost::asio::placeholders::bytes_transferred));
		return;
	}

	// write the current chunk
	m_writer->writeNoCopy(m_chunk_iterator->second, m_chunk_iterator->first);
	
	if (++m_chunk_iterator == m_chunks.end()) {
		m_writer->sendFinalChunk(boost::bind(&ChunkedPostRequestSender::handleWrite,
											 shared_from_this(),
											 boost::asio::placeholders::error,
											 boost::asio::placeholders::bytes_transferred));
	} else {
		m_writer->sendChunk(boost::bind(&ChunkedPostRequestSender::handleWrite,
										shared_from_this(),
										boost::asio::placeholders::error,
										boost::asio::placeholders::bytes_transferred));
	}
}

void ChunkedPostRequestSender::handleWrite(const boost::system::error_code& write_error,
										   std::size_t bytes_written)
{
	if (write_error) {
		// encountered error sending request data
		m_writer->getTCPConnection()->setLifecycle(TCPConnection::LIFECYCLE_CLOSE);	// make sure it will get closed
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

///
/// WebServerTests_F: fixture used for running web server tests
/// 
class WebServerTests_F {
public:
	
	// default constructor & destructor
	WebServerTests_F() : m_scheduler(), m_server(m_scheduler) {
		// initialize the list of directories in which to look for plug-ins
		PionPlugin::resetPluginDirectories();
#ifndef PION_STATIC_LINKING
		PionPlugin::addPluginDirectory(PATH_TO_PLUGINS);
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
	inline unsigned int sendRequest(tcp::iostream& http_stream,
									const std::string& resource,
									unsigned long& content_length)
	{
		const boost::regex regex_get_response_code("^HTTP/1\\.1\\s(\\d+)\\s.*");
		const boost::regex regex_response_header("^[A-Za-z0-9_-]+:\\s.*");
		const boost::regex regex_content_length_header("^Content-Length:\\s(\\d+).*", boost::regex::icase);
		const boost::regex regex_response_end("^\\s*$");

		// send HTTP request to the server
		http_stream << "GET " << resource << " HTTP/1.1" << HTTPTypes::STRING_CRLF << HTTPTypes::STRING_CRLF;
		http_stream.flush();
				
		// receive response from the server
		std::string rsp_line;
		boost::smatch rx_matches;
		unsigned int response_code = 0;
		BOOST_REQUIRE(std::getline(http_stream, rsp_line));
		BOOST_REQUIRE(boost::regex_match(rsp_line, rx_matches, regex_get_response_code));
		BOOST_REQUIRE(rx_matches.size() == 2);

		// extract response status code
		response_code = boost::lexical_cast<unsigned int>(rx_matches[1]);
		BOOST_REQUIRE(response_code != 0);
		
		// read response headers
		content_length = 0;
		while (true) {
			BOOST_REQUIRE(std::getline(http_stream, rsp_line));
			// check for end of response headers (empty line)
			if (boost::regex_match(rsp_line, rx_matches, regex_response_end))
				break;
			// check validity of response header
			BOOST_REQUIRE(boost::regex_match(rsp_line, rx_matches, regex_response_header));
			// check for content-length response header
			if (boost::regex_match(rsp_line, rx_matches, regex_content_length_header)) {
				if (rx_matches.size() == 2)
					content_length = boost::lexical_cast<unsigned long>(rx_matches[1]);
			}
		}
		
		return response_code;
	}
	
	/**
	 * checks the local HTTP server's response code & validity using HelloService
	 */
	inline void checkWebServerResponseCode(void) {
		// load simple Hello service and start the server
		m_server.loadService("/hello", "HelloService");
		m_server.start();
		
		// open a connection
		tcp::endpoint http_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), m_server.getPort());
		tcp::iostream http_stream(http_endpoint);
		
		// send valid request to the server
		unsigned int response_code;
		unsigned long content_length = 0;
		response_code = sendRequest(http_stream, "/hello", content_length);
		BOOST_CHECK(response_code == 200);
		BOOST_CHECK(content_length > 0);
		if (content_length > 0) {
			boost::scoped_array<char> content_buf(new char[content_length+1]);
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
	inline void checkWebServerResponseContent(tcp::iostream& http_stream,
											  const std::string& resource,
											  const boost::regex& content_regex,
											  unsigned int expectedResponseCode = 200)
	{
		// send valid request to the server
		unsigned int response_code;
		unsigned long content_length = 0;
		response_code = sendRequest(http_stream, resource, content_length);
		BOOST_CHECK(response_code == expectedResponseCode);
		BOOST_REQUIRE(content_length > 0);
		
		// read in the response content
		boost::scoped_array<char> content_buf(new char[content_length+1]);
		BOOST_CHECK(http_stream.read(content_buf.get(), content_length));
		content_buf[content_length] = '\0';
		
		// check the response content
		BOOST_CHECK(boost::regex_match(content_buf.get(), content_regex));
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
											  const boost::regex& content_regex,
											  unsigned int expectedResponseCode = 200)
	{
		// load specified service and start the server
		m_server.loadService(resource, service);
		m_server.start();
		
		// open a connection
		tcp::endpoint http_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), m_server.getPort());
		tcp::iostream http_stream(http_endpoint);

		// send request and check response
		checkWebServerResponseContent(http_stream, resource, content_regex, expectedResponseCode);
	}
	
	/**
	 * checks if we can successfully send and receive HTTP messages
	 * 
	 * @param tcp_conn open TCP connection to use for the tests
	 */
	inline void checkSendAndReceiveMessages(TCPConnection& tcp_conn) {
		// send valid request to the server
		HTTPRequest http_request("/hello");
		boost::system::error_code error_code;
		http_request.send(tcp_conn, error_code);
		BOOST_REQUIRE(! error_code);

		// receive the response from the server
		HTTPResponse http_response(http_request);
		http_response.receive(tcp_conn, error_code);
		BOOST_REQUIRE(! error_code);
		
		// check that the response is OK
		boost::regex hello_regex(".*Hello\\sWorld.*");
		BOOST_REQUIRE(http_response.getStatusCode() == 200);
		BOOST_REQUIRE(http_response.getContentLength() > 0);
		BOOST_REQUIRE(boost::regex_match(http_response.getContent(), hello_regex));
				
		// send invalid request to the server
		http_request.setResource("/doesnotexist");
		http_request.send(tcp_conn, error_code);
		BOOST_REQUIRE(! error_code);
		http_response.receive(tcp_conn, error_code);
		BOOST_REQUIRE(! error_code);
		BOOST_CHECK_EQUAL(http_response.getStatusCode(), 404U);
	}
	
	inline boost::asio::io_service& getIOService(void) { return m_scheduler.getIOService(); }
	
	PionSingleServiceScheduler	m_scheduler;
	WebServer					m_server;
};


// WebServer Test Cases

BOOST_FIXTURE_TEST_SUITE(WebServerTests_S, WebServerTests_F)

BOOST_AUTO_TEST_CASE(checkWebServerIsListening) {
	BOOST_CHECK(! m_server.isListening());
	m_server.start();
	BOOST_CHECK(m_server.isListening());
	m_server.stop();
	BOOST_CHECK(! m_server.isListening());
}

BOOST_AUTO_TEST_CASE(checkWebServerRespondsProperly) {
	checkWebServerResponseCode();
}

BOOST_AUTO_TEST_CASE(checkSendRequestsAndReceiveResponses) {
	// load simple Hello service and start the server
	m_server.loadService("/hello", "HelloService");
	m_server.start();
	
	// open a connection
	TCPConnection tcp_conn(getIOService());
	tcp_conn.setLifecycle(TCPConnection::LIFECYCLE_KEEPALIVE);
	boost::system::error_code error_code;
	error_code = tcp_conn.connect(boost::asio::ip::address::from_string("127.0.0.1"), m_server.getPort());
	BOOST_REQUIRE(! error_code);
	
	checkSendAndReceiveMessages(tcp_conn);
}

BOOST_AUTO_TEST_CASE(checkSendRequestAndReceiveResponseFromEchoService) {
	m_server.loadService("/echo", "EchoService");
	m_server.start();

	// open a connection
	TCPConnectionPtr tcp_conn(new TCPConnection(getIOService()));
	tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_KEEPALIVE);
	boost::system::error_code error_code;
	error_code = tcp_conn->connect(boost::asio::ip::address::from_string("127.0.0.1"), m_server.getPort());
	BOOST_REQUIRE(!error_code);

	HTTPRequestWriterPtr writer(HTTPRequestWriter::create(tcp_conn));
	writer->getRequest().setMethod("POST");
	writer->getRequest().setResource("/echo");

	writer << "junk";
	writer->send();

	// receive the response from the server
	HTTPResponse http_response(writer->getRequest());
	http_response.receive(*tcp_conn, error_code);
	BOOST_CHECK(!error_code);

	// check that the response is OK
	BOOST_CHECK(http_response.getStatusCode() == 200);
	BOOST_CHECK(http_response.getContentLength() > 0);

	// check the post content of the request, by parsing it out of the post content of the response
	boost::regex post_content(".*\\[POST Content]\\s*junk.*");
	BOOST_CHECK(boost::regex_match(http_response.getContent(), post_content));
}

BOOST_AUTO_TEST_CASE(checkRedirectHelloServiceToEchoService) {
	m_server.loadService("/hello", "HelloService");
	m_server.loadService("/echo", "EchoService");
	m_server.start();

	// open a connection
	tcp::endpoint http_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), m_server.getPort());
	tcp::iostream http_stream(http_endpoint);

	// send a request to /hello and check that the response is from HelloService
	checkWebServerResponseContent(http_stream, "/hello", boost::regex(".*Hello\\sWorld.*"));

	m_server.addRedirect("/hello", "/echo");

	// send a request to /hello and check that the response is from EchoService
	checkWebServerResponseContent(http_stream, "/hello", boost::regex(".*\\[Request\\sEcho\\].*"));
}

BOOST_AUTO_TEST_CASE(checkOriginalResourceAvailableAfterRedirect) {
	m_server.loadService("/hello", "HelloService");
	m_server.loadService("/echo", "EchoService");
	m_server.start();

	// open a connection
	tcp::endpoint http_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), m_server.getPort());
	tcp::iostream http_stream(http_endpoint);

	m_server.addRedirect("/hello", "/echo");

	// send a request to /hello and check the reported values of the original resource and the delivered resource
	boost::regex regex_expected_content(".*Resource\\soriginally\\srequested:\\s/hello.*Resource\\sdelivered:\\s/echo.*");
	checkWebServerResponseContent(http_stream, "/hello", regex_expected_content);
}

BOOST_AUTO_TEST_CASE(checkRecursiveRedirect) {
	m_server.loadService("/hello", "HelloService");
	m_server.loadService("/echo", "EchoService");
	m_server.loadService("/cookie", "CookieService");
	m_server.start();

	// open a connection
	tcp::endpoint http_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), m_server.getPort());
	tcp::iostream http_stream(http_endpoint);

	m_server.addRedirect("/hello", "/echo");
	m_server.addRedirect("/echo", "/cookie");

	// send a request to /hello and check that the response is from CookieService
	checkWebServerResponseContent(http_stream, "/hello", boost::regex(".*<html>.*Cookie\\sService.*</html>.*"));
}

BOOST_AUTO_TEST_CASE(checkCircularRedirect) {
	m_server.loadService("/hello", "HelloService");
	m_server.loadService("/cookie", "CookieService");
	m_server.loadService("/echo", "EchoService");
	m_server.start();

	// open a connection
	tcp::endpoint http_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), m_server.getPort());
	tcp::iostream http_stream(http_endpoint);

	// set up a circular set of redirects
	m_server.addRedirect("/hello", "/echo");
	m_server.addRedirect("/echo", "/cookie");
	m_server.addRedirect("/cookie", "/hello");

	// send request and check that server returns expected status code and error message
	checkWebServerResponseContent(http_stream, "/hello",
								  boost::regex(".*Maximum number of redirects.*exceeded.*"),
								  HTTPTypes::RESPONSE_CODE_SERVER_ERROR);
}

BOOST_AUTO_TEST_CASE(checkSendChunkedRequestAndReceiveResponse) {
	m_server.loadService("/echo", "EchoService");
	m_server.start();

	// open a connection
	TCPConnectionPtr tcp_conn(new TCPConnection(getIOService()));
	tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_KEEPALIVE);
	boost::system::error_code error_code;
	error_code = tcp_conn->connect(boost::asio::ip::address::from_string("127.0.0.1"), m_server.getPort());
	BOOST_REQUIRE(!error_code);

	boost::shared_ptr<ChunkedPostRequestSender> sender = ChunkedPostRequestSender::create(tcp_conn, "/echo");
	sender->addChunk(5, "klmno");
	sender->addChunk(4, "1234");
	sender->addChunk(10, "abcdefghij");
	sender->send();

	// receive the response from the server
	HTTPResponse http_response("GET");
	http_response.receive(*tcp_conn, error_code);
	BOOST_CHECK(!error_code);

	// check that the response is OK
	BOOST_CHECK(http_response.getStatusCode() == 200);
	BOOST_CHECK(http_response.getContentLength() > 0);

	// check the content length of the request, by parsing it out of the post content of the response
	boost::regex content_length_of_request(".*Content length\\: 19.*");
	BOOST_CHECK(boost::regex_match(http_response.getContent(), content_length_of_request));

	// check the post content of the request, by parsing it out of the post content of the response
	boost::regex post_content_of_request(".*\\[POST Content]\\s*klmno1234abcdefghij.*");
	BOOST_CHECK(boost::regex_match(http_response.getContent(), post_content_of_request));
}

BOOST_AUTO_TEST_CASE(checkSendChunkedRequestWithOneChunkAndReceiveResponse) {
	m_server.loadService("/echo", "EchoService");
	m_server.start();

	// open a connection
	TCPConnectionPtr tcp_conn(new TCPConnection(getIOService()));
	tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_KEEPALIVE);
	boost::system::error_code error_code;
	error_code = tcp_conn->connect(boost::asio::ip::address::from_string("127.0.0.1"), m_server.getPort());
	BOOST_REQUIRE(!error_code);

	boost::shared_ptr<ChunkedPostRequestSender> sender = ChunkedPostRequestSender::create(tcp_conn, "/echo");
	sender->addChunk(10, "abcdefghij");
	sender->send();

	// receive the response from the server
	HTTPResponse http_response("GET");
	http_response.receive(*tcp_conn, error_code);
	BOOST_CHECK(!error_code);

	// check that the response is OK
	BOOST_CHECK(http_response.getStatusCode() == 200);
	BOOST_CHECK(http_response.getContentLength() > 0);

	// check the post content of the request, by parsing it out of the post content of the response
	boost::regex post_content(".*\\[POST Content]\\s*abcdefghij.*");
	BOOST_CHECK(boost::regex_match(http_response.getContent(), post_content));
}

BOOST_AUTO_TEST_CASE(checkSendChunkedRequestWithNoChunksAndReceiveResponse) {
	m_server.loadService("/echo", "EchoService");
	m_server.start();

	// open a connection
	TCPConnectionPtr tcp_conn(new TCPConnection(getIOService()));
	tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_KEEPALIVE);
	boost::system::error_code error_code;
	error_code = tcp_conn->connect(boost::asio::ip::address::from_string("127.0.0.1"), m_server.getPort());
	BOOST_REQUIRE(!error_code);

	boost::shared_ptr<ChunkedPostRequestSender> sender = ChunkedPostRequestSender::create(tcp_conn, "/echo");
	sender->send();

	// receive the response from the server
	HTTPResponse http_response("GET");
	http_response.receive(*tcp_conn, error_code);
	BOOST_CHECK(!error_code);

	// check that the response is OK
	BOOST_CHECK(http_response.getStatusCode() == 200);
	BOOST_CHECK(http_response.getContentLength() > 0);

	// check the content length of the request, by parsing it out of the post content of the response
	boost::regex content_length_of_request(".*Content length\\: 0.*");
	BOOST_CHECK(boost::regex_match(http_response.getContent(), content_length_of_request));
}

#ifdef PION_HAVE_SSL
BOOST_AUTO_TEST_CASE(checkSendRequestsAndReceiveResponsesUsingSSL) {
	// load simple Hello service and start the server
	m_server.setSSLKeyFile(SSL_PEM_FILE);
	m_server.loadService("/hello", "HelloService");
	m_server.start();

	// open a connection
	TCPConnection tcp_conn(getIOService(), true);
	tcp_conn.setLifecycle(TCPConnection::LIFECYCLE_KEEPALIVE);
	boost::system::error_code error_code;
	error_code = tcp_conn.connect(boost::asio::ip::address::from_string("127.0.0.1"), m_server.getPort());
	BOOST_REQUIRE(! error_code);
	error_code = tcp_conn.handshake_client();
	BOOST_REQUIRE(! error_code);

	checkSendAndReceiveMessages(tcp_conn);
}
#endif

BOOST_AUTO_TEST_CASE(checkHelloServiceResponseContent) {
	checkWebServerResponseContent("HelloService", "/hello",
								  boost::regex(".*Hello\\sWorld.*"));
}

BOOST_AUTO_TEST_CASE(checkCookieServiceResponseContent) {
	checkWebServerResponseContent("CookieService", "/cookie",
								  boost::regex(".*<html>.*Cookie\\sService.*</html>.*"));
}

BOOST_AUTO_TEST_CASE(checkEchoServiceResponseContent) {
	checkWebServerResponseContent("EchoService", "/echo",
								  boost::regex(".*\\[Request\\sEcho\\].*\\[POST\\sContent\\].*"));
}

BOOST_AUTO_TEST_CASE(checkLogServiceResponseContent) {
#if defined(PION_USE_LOG4CXX) || defined(PION_USE_LOG4CPLUS) || defined(PION_USE_LOG4CPP)
	// make sure that the log level is high enough so that the entry will be recorded
	pion::PionLogger log_ptr = PION_GET_LOGGER("pion.net");
	PION_LOG_SETLEVEL_INFO(log_ptr);
	// make sure that the log service includes an entry for loading itself
	checkWebServerResponseContent("LogService", "/log",
								  boost::regex(".*Loaded.*plug-in.*\\(/log\\):\\sLogService.*"));
	// bump the log level back down when we are done with the test
	PION_LOG_SETLEVEL_WARN(log_ptr);
#elif defined(PION_DISABLE_LOGGING)
	checkWebServerResponseContent("LogService", "/log",
								  boost::regex(".*Logging\\sis\\sdisabled.*"));
#else
	checkWebServerResponseContent("LogService", "/log",
								  boost::regex(".*Using\\sostream\\slogging.*"));
#endif
}

#ifndef PION_STATIC_LINKING
BOOST_AUTO_TEST_CASE(checkAllowNothingServiceResponseContent) {
	checkWebServerResponseContent("AllowNothingService", "/deny",
								  boost::regex(".*No, you can't.*"),
								  HTTPTypes::RESPONSE_CODE_METHOD_NOT_ALLOWED);
}
#endif // PION_STATIC_LINKING

BOOST_AUTO_TEST_CASE(checkFileServiceResponseContent) {
	// load multiple services and start the server
	m_server.loadServiceConfig(SERVICES_CONFIG_FILE);
	m_server.start();
	
	// open a connection
	tcp::endpoint http_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), m_server.getPort());
	tcp::iostream http_stream(http_endpoint);
	
	// send request and check response (index page)
	const boost::regex index_page_regex(".*<html>.*Test\\sWebsite.*</html>.*");
	checkWebServerResponseContent(http_stream, "/" , index_page_regex);
	checkWebServerResponseContent(http_stream, "/index.html" , index_page_regex);

	// send request and check response (copy of docs index page generated by doxygen)
	const boost::regex doc_index_regex(".*<html>.*pion-.*Documentation.*</html>.*");
	checkWebServerResponseContent(http_stream, "/doc/index.html" , doc_index_regex);
}

BOOST_AUTO_TEST_CASE(checkPionUserPasswordSanity) {
	const std::string clear_pw("deadmeat");
	PionUser u("test-user");
	u.setPassword(clear_pw);
	BOOST_CHECK(u.matchPassword(clear_pw));

#ifdef PION_HAVE_SSL
	std::string encrypted_pw = u.getPassword();
	BOOST_CHECK_EQUAL(encrypted_pw.size(), static_cast<unsigned int>(SHA_DIGEST_LENGTH * 2));
	BOOST_CHECK(clear_pw != encrypted_pw);
	
	u.setPasswordHash(encrypted_pw);
	BOOST_CHECK_EQUAL(encrypted_pw, u.getPassword());	// should still be identical
	BOOST_CHECK(u.matchPassword(clear_pw));
#endif
}

BOOST_AUTO_TEST_CASE(checkBasicAuthServiceFailure) {
	m_server.loadService("/auth", "EchoService");
	PionUserManagerPtr userManager(new PionUserManager());
	HTTPAuthPtr auth_ptr(new HTTPBasicAuth(userManager));
	m_server.setAuthentication(auth_ptr);
	auth_ptr->addRestrict("/auth");
	auth_ptr->addUser("mike", "123456");
	m_server.start();
	
	// open a connection
	TCPConnectionPtr tcp_conn(new TCPConnection(getIOService()));
	tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_KEEPALIVE);
	boost::system::error_code error_code;
	error_code = tcp_conn->connect(boost::asio::ip::address::from_string("127.0.0.1"), m_server.getPort());
	BOOST_REQUIRE(!error_code);
	
	HTTPRequestWriterPtr writer(HTTPRequestWriter::create(tcp_conn));
	writer->getRequest().setMethod("POST");
	writer->getRequest().setResource("/auth/something/somewhere");
	
	writer << "junk";
	writer->send();
	
	// receive the response from the server
	HTTPResponse http_response(writer->getRequest());
	http_response.receive(*tcp_conn, error_code);
	BOOST_CHECK(!error_code);
	
	// check that the response is RESPONSE_CODE_UNAUTHORIZED
	BOOST_CHECK(http_response.getStatusCode() == HTTPTypes::RESPONSE_CODE_UNAUTHORIZED);
	BOOST_CHECK(http_response.getContentLength() > 0);
	
	// check the post content of the request, by parsing it out of the post content of the response
	boost::regex post_content(".*\\[POST Content]\\s*junk.*");
	BOOST_CHECK(!boost::regex_match(http_response.getContent(), post_content));
}

BOOST_AUTO_TEST_CASE(checkBasicAuthServiceLogin) {
	m_server.loadService("/auth", "EchoService");
	PionUserManagerPtr userManager(new PionUserManager());
	HTTPAuthPtr auth_ptr(new HTTPBasicAuth(userManager));
	m_server.setAuthentication(auth_ptr);
	auth_ptr->addRestrict("/auth");
	auth_ptr->addUser("mike", "123456");
	m_server.start();
	
	// open a connection
	TCPConnectionPtr tcp_conn(new TCPConnection(getIOService()));
	tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_KEEPALIVE);
	boost::system::error_code error_code;
	error_code = tcp_conn->connect(boost::asio::ip::address::from_string("127.0.0.1"), m_server.getPort());
	BOOST_REQUIRE(!error_code);
	
	HTTPRequestWriterPtr writer(HTTPRequestWriter::create(tcp_conn));
	writer->getRequest().setMethod("POST");
	writer->getRequest().setResource("/auth/something/somewhere");
	// add an authentication for "mike:123456"
	writer->getRequest().addHeader(HTTPTypes::HEADER_AUTHORIZATION, "Basic bWlrZToxMjM0NTY=");
	
	writer << "junk";
	writer->send();
	
	// receive the response from the server
	HTTPResponse http_response(writer->getRequest());
	http_response.receive(*tcp_conn, error_code);
	BOOST_CHECK(!error_code);
	
	// check that the response is OK
	BOOST_CHECK(http_response.getStatusCode() == 200);
	BOOST_CHECK(http_response.getContentLength() > 0);
	
	// check the post content of the request, by parsing it out of the post content of the response
	boost::regex post_content(".*\\[POST Content]\\s*junk.*");
	BOOST_CHECK(boost::regex_match(http_response.getContent(), post_content));
}

BOOST_AUTO_TEST_CASE(checkCookieAuthServiceFailure) {
	m_server.loadService("/auth", "EchoService");
	PionUserManagerPtr userManager(new PionUserManager());
	HTTPAuthPtr auth_ptr(new HTTPCookieAuth(userManager));
	m_server.setAuthentication(auth_ptr);
	auth_ptr->addRestrict("/auth");
	auth_ptr->addUser("mike", "123456");
	m_server.start();

	// open a connection
	TCPConnectionPtr tcp_conn(new TCPConnection(getIOService()));
	tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_KEEPALIVE);
	boost::system::error_code error_code;
	error_code = tcp_conn->connect(boost::asio::ip::address::from_string("127.0.0.1"), m_server.getPort());
	BOOST_REQUIRE(!error_code);

	HTTPRequestWriterPtr writer(HTTPRequestWriter::create(tcp_conn));
	writer->getRequest().setMethod("POST");
	writer->getRequest().setResource("/auth/something/somewhere");

	writer << "junk";
	writer->send();

	// receive the response from the server
	HTTPResponse http_response(writer->getRequest());
	http_response.receive(*tcp_conn, error_code);
	BOOST_CHECK(!error_code);

	// check that the response is RESPONSE_CODE_UNAUTHORIZED
	BOOST_CHECK(http_response.getStatusCode() == HTTPTypes::RESPONSE_CODE_UNAUTHORIZED);
	BOOST_CHECK(http_response.getContentLength() > 0);

	// check the post content of the request, by parsing it out of the post content of the response
	boost::regex post_content(".*\\[POST Content]\\s*junk.*");
	BOOST_CHECK(!boost::regex_match(http_response.getContent(), post_content));
}

BOOST_AUTO_TEST_CASE(checkCookieAuthServiceLogin) {
	m_server.loadService("/auth", "EchoService");
	PionUserManagerPtr userManager(new PionUserManager());
	HTTPAuthPtr auth_ptr(new HTTPCookieAuth(userManager));
	m_server.setAuthentication(auth_ptr);
	auth_ptr->addRestrict("/auth");
	auth_ptr->addUser("mike", "123456");
	m_server.start();

	// open a login connection
	TCPConnectionPtr tcp_conn(new TCPConnection(getIOService()));
	tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_KEEPALIVE);
	boost::system::error_code error_code;
	error_code = tcp_conn->connect(boost::asio::ip::address::from_string("127.0.0.1"), m_server.getPort());
	BOOST_REQUIRE(!error_code);

	HTTPRequestWriterPtr writer(HTTPRequestWriter::create(tcp_conn));
	writer->getRequest().setMethod("GET");
	// login as "mike:123456"
	writer->getRequest().setResource("/login?user=mike&pass=123456");

	//writer << "junk";
	writer->send();

	// receive the response from the server
	HTTPResponse http_response(writer->getRequest());
	http_response.receive(*tcp_conn, error_code);
	BOOST_CHECK(!error_code);

	// check that the response is OK
	BOOST_CHECK(http_response.getStatusCode() == 204);
	BOOST_CHECK(http_response.getContentLength() == 0);
	BOOST_CHECK(http_response.hasHeader(HTTPTypes::HEADER_SET_COOKIE));
	// get Cookies
	std::string cookie = http_response.getHeader(HTTPTypes::HEADER_SET_COOKIE);

	// now try to connect to protected area using login cookie

	HTTPRequestWriterPtr writer2(HTTPRequestWriter::create(tcp_conn));
	writer2->getRequest().setMethod("POST");
	writer2->getRequest().setResource("/auth/something/somewhere");
	// add an authentications for "mike:123456"
	writer2->getRequest().addHeader(HTTPTypes::HEADER_COOKIE,cookie);

	writer2 << "junk";
	writer2->send();

	// receive the response from the server
	HTTPResponse http_response2(writer2->getRequest());
	http_response2.receive(*tcp_conn, error_code);
	BOOST_CHECK(!error_code);

	// check that the response is OK
	BOOST_CHECK(http_response2.getStatusCode() == 200);
	BOOST_CHECK(http_response2.getContentLength() > 0);

	// check the post content of the request, by parsing it out of the post content of the response
	boost::regex post_content(".*\\[POST Content]\\s*junk.*");
	BOOST_CHECK(boost::regex_match(http_response2.getContent(), post_content));
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
	 * @param request the HTTP request to respond to
	 * @param tcp_conn the TCP connection to send the response over
	 */
	void sendResponseWithContentButNoLength(HTTPRequestPtr& request,
											TCPConnectionPtr& tcp_conn)
	{
		// make sure it will get closed when finished
		tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_CLOSE);
		
		// prepare the response headers
		HTTPResponse http_response(*request);
		http_response.setDoNotSendContentLength();
		
		// send the response headers
		boost::system::error_code error_code;
		http_response.send(*tcp_conn, error_code);
		BOOST_REQUIRE(! error_code);
		
		// send the content buffer
		tcp_conn->write(boost::asio::buffer(m_big_buf, BIG_BUF_SIZE), error_code);
		BOOST_REQUIRE(! error_code);
		
		// finish (and close) the connection
		tcp_conn->finish();
	}
	
	/// reads in a HTTP response asynchronously
	void readAsyncResponse(TCPConnectionPtr& tcp_conn)
	{
		HTTPRequest http_request("GET");
		HTTPResponseReaderPtr reader_ptr(HTTPResponseReader::create(tcp_conn, http_request,
																	boost::bind(&ContentResponseWithoutLengthTests_F::checkResponse,
																	this, _1, _2, _3)));
		reader_ptr->receive();
	}

	/// checks the validity of the HTTP response
	void checkResponse(const HTTPResponse& http_response)
	{
		BOOST_REQUIRE(http_response.getStatusCode() == 200);
		BOOST_CHECK(! http_response.hasHeader(HTTPTypes::HEADER_CONTENT_LENGTH));
		BOOST_REQUIRE(http_response.getContentLength() == BIG_BUF_SIZE);
		BOOST_CHECK_EQUAL(memcmp(http_response.getContent(), m_big_buf, BIG_BUF_SIZE), 0);
	}
	
	/// checks the validity of the HTTP response
	void checkResponse(HTTPResponsePtr& response_ptr,
		TCPConnectionPtr& conn_ptr, const boost::system::error_code& ec)
	{
		checkResponse(*response_ptr);
		boost::mutex::scoped_lock async_lock(m_mutex);
		m_async_test_finished.notify_one();
	}

	/// big data buffer used for the tests
	char				m_big_buf[BIG_BUF_SIZE];
	
	/// signaled after the async response check has finished
	boost::condition	m_async_test_finished;

	/// used to protect the asynchronous operations
	boost::mutex		m_mutex;
};


// ContentResponseWithoutLengthTests_F Test Cases

BOOST_FIXTURE_TEST_SUITE(ContentResponseWithoutLengthTests_S, ContentResponseWithoutLengthTests_F)

BOOST_AUTO_TEST_CASE(checkSendContentWithoutLengthAndReceiveSyncResponse) {
	// startup the server 
	m_server.addResource("/big", boost::bind(&ContentResponseWithoutLengthTests_F::sendResponseWithContentButNoLength,
											 this, _1, _2));
	m_server.start();
	
	// open a connection
	TCPConnectionPtr tcp_conn(new TCPConnection(getIOService()));
	boost::system::error_code error_code;
	error_code = tcp_conn->connect(boost::asio::ip::address::from_string("127.0.0.1"), m_server.getPort());
	BOOST_REQUIRE(!error_code);

	// send an HTTP request
	HTTPRequest http_request("/big");
	http_request.send(*tcp_conn, error_code);
	BOOST_REQUIRE(! error_code);
	
	// receive the response from the server
	HTTPResponse http_response(http_request);
	http_response.receive(*tcp_conn, error_code);
	BOOST_REQUIRE(! error_code);
	
	// check that the response is OK
	checkResponse(http_response);
}

BOOST_AUTO_TEST_CASE(checkSendContentWithoutLengthAndReceiveAsyncResponse) {
	// startup the server 
	m_server.addResource("/big", boost::bind(&ContentResponseWithoutLengthTests_F::sendResponseWithContentButNoLength,
											 this, _1, _2));
	m_server.start();
	
	// open a connection
	TCPConnectionPtr tcp_conn(new TCPConnection(getIOService()));
	boost::system::error_code error_code;
	error_code = tcp_conn->connect(boost::asio::ip::address::from_string("127.0.0.1"), m_server.getPort());
	BOOST_REQUIRE(!error_code);
	
	// send an HTTP request
	boost::mutex::scoped_lock async_lock(m_mutex);
	HTTPRequestWriterPtr request_ptr(HTTPRequestWriter::create(tcp_conn,
									 boost::bind(&ContentResponseWithoutLengthTests_F::readAsyncResponse,
												 this, tcp_conn)));
	request_ptr->getRequest().setResource("/big");
	request_ptr->send();
	
	// wait until the test is finished (and async calls have finished)
	m_async_test_finished.wait(async_lock);
}

BOOST_AUTO_TEST_SUITE_END()
