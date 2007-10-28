// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/bind.hpp>
#include <boost/scoped_array.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>
#include <pion/PionConfig.hpp>
#include <pion/PionPlugin.hpp>
#include <pion/net/HTTPServer.hpp>

using namespace std;
using namespace pion;
using namespace pion::net;
using boost::asio::ip::tcp;

#if defined(_MSC_VER)
	#if defined(_DEBUG)
		static const std::string PATH_TO_PLUGINS("../../bin/Debug_DLL");
	#else
		static const std::string PATH_TO_PLUGINS("../../bin/Release_DLL");
	#endif
	static const std::string SSL_PEM_FILE("../utils/sslkey.pem");
	static const std::string SERVICES_CONFIG_FILE("../utils/vcservices.conf");
#elif defined(PION_XCODE)
	static const std::string PATH_TO_PLUGINS(".");
	static const std::string SSL_PEM_FILE("../../net/utils/sslkey.pem");
	static const std::string SERVICES_CONFIG_FILE("../../net/utils/xcodeservices.conf");
#else
	// same for Unix and Cygwin
	static const std::string PATH_TO_PLUGINS("../services/.libs");
	static const std::string SSL_PEM_FILE("../utils/sslkey.pem");
	static const std::string SERVICES_CONFIG_FILE("../utils/testservices.conf");
#endif


/// sets up logging (run once only)
extern void setup_logging_for_unit_tests(void);

///
/// WebServerTests_F: fixture used for running web server tests
/// 
class WebServerTests_F {
public:
	
	// default constructor & destructor
	WebServerTests_F() {
		setup_logging_for_unit_tests();
		// add path to plug-in files
		PionPlugin::addPluginDirectory(PATH_TO_PLUGINS);
		// create an HTTP server for port 8080
		http_server_ptr = HTTPServer::create(8080);
		BOOST_REQUIRE(http_server_ptr);
	}
	~WebServerTests_F() {}
	
	/// returns a smart pointer to the HTTP server
	inline HTTPServerPtr& getServerPtr(void) { return http_server_ptr; }
	
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
		http_stream << "GET " << resource << " HTTP/1.1\r\n\r\n";
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
		getServerPtr()->loadService("/hello", "HelloService");
		getServerPtr()->start();
		
		// open a connection
		tcp::endpoint http_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 8080);
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
											  const boost::regex& content_regex)
	{
		// send valid request to the server
		unsigned int response_code;
		unsigned long content_length = 0;
		response_code = sendRequest(http_stream, resource, content_length);
		BOOST_CHECK(response_code == 200);
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
											  const boost::regex& content_regex)
	{
		// load simple Hello service and start the server
		getServerPtr()->loadService(resource, service);
		getServerPtr()->start();
		
		// open a connection
		tcp::endpoint http_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 8080);
		tcp::iostream http_stream(http_endpoint);

		// send request and check response
		checkWebServerResponseContent(http_stream, resource, content_regex);
	}
	
private:
	HTTPServerPtr	http_server_ptr;
};


// WebServer Test Cases

BOOST_FIXTURE_TEST_SUITE(WebServerTests_S, WebServerTests_F)

BOOST_AUTO_TEST_CASE(checkWebServerIsListening) {
	BOOST_CHECK(! getServerPtr()->isListening());
	getServerPtr()->start();
	BOOST_CHECK(getServerPtr()->isListening());
	getServerPtr()->stop();
	BOOST_CHECK(! getServerPtr()->isListening());
}

BOOST_AUTO_TEST_CASE(checkWebServerRespondsProperly) {
	checkWebServerResponseCode();
}

/*
#ifdef PION_HAVE_SSL
BOOST_AUTO_TEST_CASE(checkSSLWebServerRespondsProperly) {
	getServerPtr()->setSSLKeyFile(SSL_PEM_FILE);
	checkWebServerResponseCode();
}
#endif
*/

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
#if defined(PION_HAVE_LOG4CXX) || defined(PION_HAVE_LOG4CPP) || defined(PION_HAVE_LOG4CPLUS)
	checkWebServerResponseContent("LogService", "/log",
								  boost::regex(".*Loaded.*plug-in.*\\(/log\\):\\sLogService.*"));
#else
	checkWebServerResponseContent("LogService", "/log",
								  boost::regex(".*Logging\\sis\\sdisabled.*"));
#endif
}

BOOST_AUTO_TEST_CASE(checkFileServiceResponseContent) {
	// load simple Hello service and start the server
	getServerPtr()->loadServiceConfig(SERVICES_CONFIG_FILE);
	getServerPtr()->start();
	
	// open a connection
	tcp::endpoint http_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 8080);
	tcp::iostream http_stream(http_endpoint);
	
	// send request and check response (index page)
	const boost::regex index_page_regex(".*<html>.*Test\\sWebsite.*</html>.*");
	checkWebServerResponseContent(http_stream, "/" , index_page_regex);
	checkWebServerResponseContent(http_stream, "/index.html" , index_page_regex);

	// send request and check response (docs index page: requires net/doc/html doxygen files!)
	const boost::regex doc_index_regex(".*<html>.*pion-net\\sDocumentation.*</html>.*");
	checkWebServerResponseContent(http_stream, "/doc/index.html" , doc_index_regex);
}

BOOST_AUTO_TEST_SUITE_END()
