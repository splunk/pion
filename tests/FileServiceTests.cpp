// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/PionConfig.hpp>
#include <pion/PionPlugin.hpp>
#include <pion/net/HTTPServer.hpp>
#include <boost/bind.hpp>
#include <boost/scoped_array.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>

using namespace pion;
using namespace pion::net;
using boost::asio::ip::tcp;

PION_DECLARE_PLUGIN(FileService)

#if defined(_MSC_VER)
	#if defined(_DEBUG)
		static const std::string PATH_TO_PLUGINS("../../bin/Debug_DLL");
	#else
		static const std::string PATH_TO_PLUGINS("../../bin/Release_DLL");
	#endif
#elif defined(PION_XCODE)
	static const std::string PATH_TO_PLUGINS(".");
#else
	// same for Unix and Cygwin
	static const std::string PATH_TO_PLUGINS("../services/.libs");
#endif


/// sets up logging (run once only)
extern void setup_logging_for_unit_tests(void);

class FileServiceTests_F {
public:
	FileServiceTests_F() {
		setup_logging_for_unit_tests();
		PionPlugin::addPluginDirectory(PATH_TO_PLUGINS);
		m_http_server_ptr = HTTPServer::create(8080);
		BOOST_REQUIRE(m_http_server_ptr);
		m_content_length = 0;

		boost::filesystem::remove_all("sandbox");
		BOOST_REQUIRE(boost::filesystem::create_directory("sandbox"));
		boost::filesystem::ofstream file1("sandbox/file1");
		file1 << "abc" << std::endl;
		file1.close();
		boost::filesystem::ofstream file2("sandbox/file2");
		file2 << "xyz" << std::endl;
		file2.close();
		boost::filesystem::ofstream emptyFile("sandbox/emptyFile");
		emptyFile.close();

		getServerPtr()->loadService("/resource1", "FileService");
		getServerPtr()->setServiceOption("/resource1", "directory", "sandbox");
		getServerPtr()->setServiceOption("/resource1", "file", "sandbox/file1");
		getServerPtr()->start();

		// open a connection
		tcp::endpoint http_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 8080);
		m_http_stream.connect(http_endpoint);
	}
	~FileServiceTests_F() {
		boost::filesystem::remove_all("sandbox");
	}
	
	/// returns a smart pointer to the HTTP server
	inline HTTPServerPtr& getServerPtr(void) { return m_http_server_ptr; }
	
	/**
	 * sends a request to the local HTTP server
	 *
	 * @param http_stream open stream to send the request via
	 * @param request_method GET, POST, PUT or DELETE
	 * @param resource name of the HTTP resource to request
	 */
	inline unsigned int sendRequest(tcp::iostream& http_stream,
									const std::string& request_method,
									const std::string& resource)
	{
		const boost::regex regex_get_response_code("^HTTP/1\\.1\\s(\\d+)\\s.*");
		const boost::regex regex_response_header("^[A-Za-z0-9_-]+:\\s.*");
		const boost::regex regex_content_length_header("^Content-Length:\\s(\\d+).*", boost::regex::icase);
		const boost::regex regex_response_end("^\\s*$");

		// send HTTP request to the server
		http_stream << request_method << " " << resource << " HTTP/1.1\r\n\r\n";
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
					m_content_length = boost::lexical_cast<unsigned long>(rx_matches[1]);
			}
		}
		
		return response_code;
	}
	
	/**
	 * checks response content validity for the local HTTP server
	 *
	 * @param http_stream open stream that was sent a request
	 * @param content_regex regex that the response content should match
	 */
	inline void checkWebServerResponseContent(tcp::iostream& http_stream,
											  const boost::regex& content_regex)
	{
		// read in the response content
		boost::scoped_array<char> content_buf(new char[m_content_length + 1]);
		BOOST_CHECK(http_stream.read(content_buf.get(), m_content_length));
		content_buf[m_content_length] = '\0';
		
		// check the response content
		BOOST_CHECK(boost::regex_match(content_buf.get(), content_regex));
	}
	
	unsigned long m_content_length;
	tcp::iostream m_http_stream;

private:
	HTTPServerPtr m_http_server_ptr;
};

BOOST_FIXTURE_TEST_SUITE(FileServiceTests_S, FileServiceTests_F)

BOOST_AUTO_TEST_CASE(checkResponseToGetRequestForDefaultFile) {
	unsigned int response_code = sendRequest(m_http_stream, "GET", "/resource1");
	BOOST_CHECK(response_code == 200);
	BOOST_REQUIRE(m_content_length > 0);
	checkWebServerResponseContent(m_http_stream, boost::regex("abc\\s*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToGetRequestForSpecifiedFile) {
	unsigned int response_code = sendRequest(m_http_stream, "GET", "/resource1/file2");
	BOOST_CHECK(response_code == 200);
	BOOST_REQUIRE(m_content_length > 0);
	checkWebServerResponseContent(m_http_stream, boost::regex("xyz\\s*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToGetRequestForEmptyFile) {
	unsigned int response_code = sendRequest(m_http_stream, "GET", "/resource1/emptyFile");
	BOOST_CHECK(response_code == 200);
	BOOST_REQUIRE(m_content_length == 0);
}

BOOST_AUTO_TEST_CASE(checkResponseToGetRequestForNonexistentFile) {
	unsigned int response_code = sendRequest(m_http_stream, "GET", "/resource1/file3");
	BOOST_CHECK(response_code == 404);
	BOOST_CHECK(m_content_length > 0);
	checkWebServerResponseContent(m_http_stream, boost::regex(".*404\\sNot\\sFound.*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToPostRequestForNonexistentFile) {
	unsigned int response_code = sendRequest(m_http_stream, "POST", "/resource1/file3");
	BOOST_CHECK(response_code == 404);
	BOOST_CHECK(m_content_length > 0);
	checkWebServerResponseContent(m_http_stream, boost::regex(".*404\\sNot\\sFound.*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToPutRequestForNonexistentFile) {
	unsigned int response_code = sendRequest(m_http_stream, "PUT", "/resource1/file3");
	BOOST_CHECK(response_code == 404);
	BOOST_CHECK(m_content_length > 0);
	checkWebServerResponseContent(m_http_stream, boost::regex(".*404\\sNot\\sFound.*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToDeleteRequestForNonexistentFile) {
	unsigned int response_code = sendRequest(m_http_stream, "DELETE", "/resource1/file3");
	BOOST_CHECK(response_code == 404);
	BOOST_CHECK(m_content_length > 0);
	checkWebServerResponseContent(m_http_stream, boost::regex(".*404\\sNot\\sFound.*"));
}

BOOST_AUTO_TEST_SUITE_END()
