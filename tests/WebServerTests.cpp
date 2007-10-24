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
#include <pion/net/PionNet.hpp>

using namespace std;
using namespace pion;
using namespace pion::net;
using boost::asio::ip::tcp;

#if defined(PION_WIN32) && defined(_MSC_VER)
	static const std::string PATH_TO_PLUGINS(".");	// ??
	static const std::string SSL_PEM_FILE(".");	// ??
	static const std::string SERVICES_CONFIG_FILE(".");	// ??
#elif defined(PION_XCODE)
	static const std::string PATH_TO_PLUGINS(".");
	static const std::string SSL_PEM_FILE("../net/utils/sslkey.pem");
	static const std::string SERVICES_CONFIG_FILE("../net/utils/testservices.conf");
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
	WebServerTests_F() {}
	~WebServerTests_F() {}	
	inline HTTPServerPtr& getHTTPServerPtr(void) { return m_engine_mgr.http_server_ptr; }
	inline HTTPServerPtr& getHTTPSServerPtr(void) { return m_engine_mgr.https_server_ptr; }

private:
	/// class used as static object to initialize the pion network engine
	class PionNetEngineManager
	{
	public:
		PionNetEngineManager() {
			// add path to plug-in files
			PionNet::addPluginDirectory(PATH_TO_PLUGINS);

			// create an HTTP server for port 8080
			http_server_ptr = PionNet::addHTTPServer(8080);
			BOOST_CHECK(http_server_ptr);
			// load services using the configuration file
			http_server_ptr->loadServiceConfig(SERVICES_CONFIG_FILE);
	
			#ifdef PION_HAVE_SSL
				// create an HTTPS server for port 8443
				https_server_ptr = PionNet::addHTTPServer(8443);
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
			#endif
	
			// startup pion
			PionNet::startup();
		}
		~PionNetEngineManager() {
			PionNet::shutdown();
		}	
		HTTPServerPtr	http_server_ptr;
		HTTPServerPtr	https_server_ptr;
	};
	static PionNetEngineManager	m_engine_mgr;
};

/// static object used to initialize the pion network engine
WebServerTests_F::PionNetEngineManager	WebServerTests_F::m_engine_mgr;


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
	http_stream << "GET / HTTP/1.0\r\n";
	http_stream.flush();

	// receive response from the server
	std::string message;
	std::getline(http_stream, message);
	http_stream.close();
	std::cout << "message: [" << message << "]" << std::endl;
	BOOST_CHECK(message == "Goodbye!");
}

BOOST_AUTO_TEST_SUITE_END()
