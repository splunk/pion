// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/bind.hpp>
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
#elif defined(PION_WIN32)
	static const std::string PATH_TO_PLUGINS(".");	// ??
	static const std::string SSL_PEM_FILE(".");	// ??
	static const std::string SERVICES_CONFIG_FILE(".");	// ??
#elif defined(PION_XCODE)
	static const std::string PATH_TO_PLUGINS(".");
	static const std::string SSL_PEM_FILE("../../net/utils/sslkey.pem");
	static const std::string SERVICES_CONFIG_FILE("../../net/utils/xcodeservices.conf");
#else
	static const std::string PATH_TO_PLUGINS("../services/.libs");
	static const std::string SSL_PEM_FILE("../utils/sslkey.pem");
	static const std::string SERVICES_CONFIG_FILE("../utils/testservices.conf");
#endif


///
/// WebServerTests_F: fixture used for running web server tests
/// 
class WebServerTests_F {
public:
	WebServerTests_F() {
		// add path to plug-in files
		PionPlugin::addPluginDirectory(PATH_TO_PLUGINS);
		
		// create an HTTP server for port 8080
		http_server_ptr = HTTPServer::create(8080);
		BOOST_CHECK(http_server_ptr);

		// load services using the configuration file
		http_server_ptr->loadServiceConfig(SERVICES_CONFIG_FILE);
		
		// start the server
		http_server_ptr->start();
		
#ifdef PION_HAVE_SSL
		// create an HTTPS server for port 8443
		https_server_ptr = HTTPServer::create(8443);
		BOOST_CHECK(https_server_ptr);
		// configure server for SSL
		https_server_ptr->setSSLFlag(true);
		boost::asio::ssl::context& ssl_context = https_server_ptr->getSSLContext();
		ssl_context.set_options(boost::asio::ssl::context::default_workarounds
								| boost::asio::ssl::context::no_sslv2
								| boost::asio::ssl::context::single_dh_use);
		ssl_context.use_certificate_file(SSL_PEM_FILE, boost::asio::ssl::context::pem);
		ssl_context.use_private_key_file(SSL_PEM_FILE, boost::asio::ssl::context::pem);
	
		// load services using the configuration file
		https_server_ptr->loadServiceConfig(SERVICES_CONFIG_FILE);
		
		// start the server
		https_server_ptr->start();
#endif
	}
	~WebServerTests_F() {
		http_server_ptr->stop();
#ifdef PION_HAVE_SSL
		https_server_ptr->stop();
#endif
	}	
	inline HTTPServerPtr& getHTTPServerPtr(void) { return http_server_ptr; }
	inline HTTPServerPtr& getHTTPSServerPtr(void) { return https_server_ptr; }

private:
	HTTPServerPtr	http_server_ptr;
	HTTPServerPtr	https_server_ptr;
};


// WebServer Test Cases

BOOST_FIXTURE_TEST_SUITE(WebServerTests_S, WebServerTests_F)

BOOST_AUTO_TEST_CASE(checkWebServerIsListening) {
	BOOST_CHECK(getHTTPServerPtr()->isListening());
}

BOOST_AUTO_TEST_CASE(checkServerConnectionBehavior) {
	// open a connection
	tcp::endpoint http_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 8080);
	tcp::iostream http_stream(http_endpoint);

	// send HTTP request to the server
	http_stream << "GET / HTTP/1.1\r\n\r\n";
	http_stream.flush();

	// receive response from the server
	std::string message;
	std::getline(http_stream, message);
	http_stream.close();
	BOOST_CHECK(message == "HTTP/1.1 200 OK\r");
}

BOOST_AUTO_TEST_SUITE_END()
