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
#include <../services/FileService.hpp>

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

#include <pion/PionScheduler.hpp>

#ifndef PION_STATIC_LINKING

struct PluginPtrWithPluginLoaded_F : public PionPluginPtr<FileService> {
	PluginPtrWithPluginLoaded_F() { 
		setup_logging_for_unit_tests();
		PionPlugin::addPluginDirectory(PATH_TO_PLUGINS);
		s = NULL;
		open("FileService");
	}
	~PluginPtrWithPluginLoaded_F() {
		if (s) destroy(s);
	}

	FileService* s;
};

BOOST_FIXTURE_TEST_SUITE(PluginPtrWithPluginLoaded_S, PluginPtrWithPluginLoaded_F)

BOOST_AUTO_TEST_CASE(checkIsOpenReturnsTrue) {
	BOOST_CHECK(is_open());
}

BOOST_AUTO_TEST_CASE(checkGetPluginNameReturnsPluginName) {
	BOOST_CHECK_EQUAL(getPluginName(), "FileService");
}

BOOST_AUTO_TEST_CASE(checkCreateReturnsSomething) {
	BOOST_CHECK((s = create()) != NULL);
}

BOOST_AUTO_TEST_CASE(checkDestroyDoesntThrowExceptionAfterCreate) {
	s = create();
	BOOST_CHECK_NO_THROW(destroy(s));
	s = NULL;
}

BOOST_AUTO_TEST_SUITE_END()

#endif // PION_STATIC_LINKING

class NewlyLoadedFileService_F {
public:
	NewlyLoadedFileService_F() {
		PionPlugin::addPluginDirectory(PATH_TO_PLUGINS);
		m_http_server_ptr = HTTPServer::create(8080);
		BOOST_REQUIRE(m_http_server_ptr);

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
		BOOST_REQUIRE(boost::filesystem::create_directory("sandbox/dir1"));

		getServerPtr()->loadService("/resource1", "FileService");
	}
	~NewlyLoadedFileService_F() {
		boost::filesystem::remove_all("sandbox");
	}
	
	/// returns a smart pointer to the HTTP server
	inline HTTPServerPtr& getServerPtr(void) { return m_http_server_ptr; }

private:
	HTTPServerPtr m_http_server_ptr;
};

BOOST_FIXTURE_TEST_SUITE(NewlyLoadedFileService_S, NewlyLoadedFileService_F)

BOOST_AUTO_TEST_CASE(checkSetServiceOptionDirectoryWithExistingDirectoryDoesntThrow) {
	BOOST_CHECK_NO_THROW(getServerPtr()->setServiceOption("/resource1", "directory", "sandbox"));
}

BOOST_AUTO_TEST_CASE(checkSetServiceOptionDirectoryWithNonexistentDirectoryThrows) {
	BOOST_CHECK_THROW(getServerPtr()->setServiceOption("/resource1", "directory", "NotADirectory"), HTTPServer::WebServiceException);
}

BOOST_AUTO_TEST_CASE(checkSetServiceOptionFileWithExistingFileDoesntThrow) {
	BOOST_CHECK_NO_THROW(getServerPtr()->setServiceOption("/resource1", "file", "sandbox/file1"));
}

BOOST_AUTO_TEST_CASE(checkSetServiceOptionFileWithNonexistentFileThrows) {
	BOOST_CHECK_THROW(getServerPtr()->setServiceOption("/resource1", "file", "NotAFile"), HTTPServer::WebServiceException);
}

// TODO: tests for options "cache", "scan", "max_chunk_size"

BOOST_AUTO_TEST_CASE(checkSetServiceOptionWritableToTrueDoesntThrow) {
	BOOST_CHECK_NO_THROW(getServerPtr()->setServiceOption("/resource1", "writable", "true"));
}

BOOST_AUTO_TEST_CASE(checkSetServiceOptionWritableToFalseDoesntThrow) {
	BOOST_CHECK_NO_THROW(getServerPtr()->setServiceOption("/resource1", "writable", "false"));
}

BOOST_AUTO_TEST_CASE(checkSetServiceOptionWritableToNonBooleanThrows) {
	BOOST_REQUIRE_THROW(getServerPtr()->setServiceOption("/resource1", "writable", "3"), HTTPServer::WebServiceException);
	try {
		getServerPtr()->setServiceOption("/resource1", "writable", "3");
	} catch (HTTPServer::WebServiceException& e) {
		BOOST_CHECK_EQUAL(e.what(), "WebService (/resource1): FileService invalid value for writable option: 3");
	}
	//Original exception is FileService::InvalidOptionValueException
}

BOOST_AUTO_TEST_CASE(checkSetServiceOptionWithInvalidOptionNameThrows) {
	BOOST_CHECK_THROW(getServerPtr()->setServiceOption("/resource1", "NotAnOption", "value1"), HTTPServer::WebServiceException);
}

BOOST_AUTO_TEST_SUITE_END()	

class RunningFileService_F : public NewlyLoadedFileService_F {
public:
	RunningFileService_F() {
		m_content_length = 0;
		getServerPtr()->setServiceOption("/resource1", "directory", "sandbox");
		getServerPtr()->setServiceOption("/resource1", "file", "sandbox/file1");
		getServerPtr()->start();

		// open a connection
		tcp::endpoint http_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 8080);
		m_http_stream.connect(http_endpoint);
	}
	~RunningFileService_F() {
	}
	
	/**
	 * sends a request to the local HTTP server
	 *
	 * @param http_stream open stream to send the request via
	 * @param request_method e.g., HEAD, GET, POST
	 * @param resource name of the HTTP resource to request
	 * @param expectedResponseCode expected status code of the response
	 */
	inline void sendRequestAndCheckResponseHead(tcp::iostream& http_stream,
												const std::string& request_method,
												const std::string& resource,
												unsigned int expectedResponseCode = 200)
	{
		// send HTTP request to the server
		http_stream << request_method << " " << resource << " HTTP/1.1\r\n\r\n";
		http_stream.flush();

		checkResponseHead(http_stream, expectedResponseCode);
	}
	
	/**
	 * check status line and headers of response
	 *
	 * @param http_stream open stream that was sent a request
	 * @param expectedResponseCode expected status code of the response
	 */
	inline void checkResponseHead(tcp::iostream& http_stream,
								  unsigned int expectedResponseCode = 200)
	{
		const boost::regex regex_get_response_code("^HTTP/1\\.1\\s(\\d+)\\s.*");
		const boost::regex regex_response_header("^[A-Za-z0-9_-]+:\\s.*");
		const boost::regex regex_content_length_header("^Content-Length:\\s(\\d+).*", boost::regex::icase);
		const boost::regex regex_response_end("^\\s*$");

		// receive status line from the server
		std::string rsp_line;
		boost::smatch rx_matches;
		unsigned int response_code = 0;
		BOOST_REQUIRE(std::getline(http_stream, rsp_line));
		BOOST_REQUIRE(boost::regex_match(rsp_line, rx_matches, regex_get_response_code));
		BOOST_REQUIRE(rx_matches.size() == 2);

		// extract response status code
		response_code = boost::lexical_cast<unsigned int>(rx_matches[1]);
		BOOST_CHECK_EQUAL(response_code, expectedResponseCode);

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
		BOOST_CHECK(m_content_length > 0);

		// read in the response content
		boost::scoped_array<char> content_buf(new char[m_content_length + 1]);
		BOOST_CHECK(http_stream.read(content_buf.get(), m_content_length));
		content_buf[m_content_length] = '\0';
		
		// check the response content
		BOOST_CHECK(boost::regex_match(content_buf.get(), content_regex));
	}
	
	unsigned long m_content_length;
	tcp::iostream m_http_stream;
};

BOOST_FIXTURE_TEST_SUITE(RunningFileService_S, RunningFileService_F)

BOOST_AUTO_TEST_CASE(checkResponseToGetRequestForDefaultFile) {
	sendRequestAndCheckResponseHead(m_http_stream, "GET", "/resource1");
	checkWebServerResponseContent(m_http_stream, boost::regex("abc\\s*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToHeadRequestForDefaultFile) {
	sendRequestAndCheckResponseHead(m_http_stream, "HEAD", "/resource1");
	BOOST_CHECK(m_content_length == 0);
}

BOOST_AUTO_TEST_CASE(checkResponseToGetRequestForDefaultFileAfterDeletingIt) {
	boost::filesystem::remove("sandbox/file1");
	sendRequestAndCheckResponseHead(m_http_stream, "GET", "/resource1", 404);
	checkWebServerResponseContent(m_http_stream, boost::regex(".*404\\sNot\\sFound.*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToHeadRequestForDefaultFileAfterDeletingIt) {
	boost::filesystem::remove("sandbox/file1");
	sendRequestAndCheckResponseHead(m_http_stream, "HEAD", "/resource1", 404);
	BOOST_CHECK(m_content_length == 0);
}

BOOST_AUTO_TEST_CASE(checkResponseToGetRequestForSpecifiedFile) {
	sendRequestAndCheckResponseHead(m_http_stream, "GET", "/resource1/file2");
	checkWebServerResponseContent(m_http_stream, boost::regex("xyz\\s*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToGetRequestForEmptyFile) {
	sendRequestAndCheckResponseHead(m_http_stream, "GET", "/resource1/emptyFile");
	BOOST_CHECK(m_content_length == 0);
}

BOOST_AUTO_TEST_CASE(checkResponseToGetRequestForNonexistentFile) {
	sendRequestAndCheckResponseHead(m_http_stream, "GET", "/resource1/file3", 404);
	checkWebServerResponseContent(m_http_stream, boost::regex(".*404\\sNot\\sFound.*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToHeadRequestForNonexistentFile) {
	sendRequestAndCheckResponseHead(m_http_stream, "HEAD", "/resource1/file3", 404);
	BOOST_CHECK(m_content_length == 0);
}

BOOST_AUTO_TEST_CASE(checkResponseToPostRequestForDefaultFile) {
	sendRequestAndCheckResponseHead(m_http_stream, "POST", "/resource1", 405);
	checkWebServerResponseContent(m_http_stream, boost::regex(".*405\\sMethod\\sNot\\sAllowed.*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToPostRequestForNonexistentFile) {
	sendRequestAndCheckResponseHead(m_http_stream, "POST", "/resource1/file3", 405);
	checkWebServerResponseContent(m_http_stream, boost::regex(".*405\\sMethod\\sNot\\sAllowed.*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToPutRequestForDefaultFile) {
	sendRequestAndCheckResponseHead(m_http_stream, "PUT", "/resource1", 405);
	checkWebServerResponseContent(m_http_stream, boost::regex(".*405\\sMethod\\sNot\\sAllowed.*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToPutRequestForNonexistentFile) {
	sendRequestAndCheckResponseHead(m_http_stream, "PUT", "/resource1/file3", 405);
	checkWebServerResponseContent(m_http_stream, boost::regex(".*405\\sMethod\\sNot\\sAllowed.*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToDeleteRequestForDefaultFile) {
	sendRequestAndCheckResponseHead(m_http_stream, "DELETE", "/resource1", 405);
	checkWebServerResponseContent(m_http_stream, boost::regex(".*405\\sMethod\\sNot\\sAllowed.*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToDeleteRequestForNonexistentFile) {
	sendRequestAndCheckResponseHead(m_http_stream, "DELETE", "/resource1/file3", 405);
	checkWebServerResponseContent(m_http_stream, boost::regex(".*405\\sMethod\\sNot\\sAllowed.*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToTraceRequestForDefaultFile) {
	sendRequestAndCheckResponseHead(m_http_stream, "TRACE", "/resource1", 501);
	checkWebServerResponseContent(m_http_stream, boost::regex(".*501\\sNot\\sImplemented.*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToRequestWithBogusMethod) {
	sendRequestAndCheckResponseHead(m_http_stream, "BOGUS", "/resource1", 501);
	checkWebServerResponseContent(m_http_stream, boost::regex(".*501\\sNot\\sImplemented.*"));
}

BOOST_AUTO_TEST_SUITE_END()

class RunningFileServiceWithWritingEnabled_F : public RunningFileService_F {
public:
	RunningFileServiceWithWritingEnabled_F() {
		getServerPtr()->setServiceOption("/resource1", "writable", "true");
	}
	~RunningFileServiceWithWritingEnabled_F() {
	}

	void checkFileContents(const std::string& filename,
						   const std::string& expectedContents)
	{
		boost::filesystem::ifstream in(filename);
		std::string actualContents;
		char c;
		while (in.get(c)) actualContents += c;
		BOOST_CHECK_EQUAL(actualContents, expectedContents);
	}
};	

BOOST_FIXTURE_TEST_SUITE(RunningFileServiceWithWritingEnabled_S, RunningFileServiceWithWritingEnabled_F)

// Doesn't pass yet.
//BOOST_AUTO_TEST_CASE(checkResponseToPostRequestForDefaultFile) {
//	m_http_stream << "POST /resource1 HTTP/1.1\r\n";
//	m_http_stream << "Content-Length: 4\r\n\r\n";
//	m_http_stream << "1234";
//	m_http_stream.flush();
//	checkResponseHead(m_http_stream, 204);
//	BOOST_CHECK(m_content_length == 0);
//	checkFileContents("sandbox/file1", "abc\n1234\n");
//}

BOOST_AUTO_TEST_CASE(checkResponseToPostRequestForSpecifiedFile) {
	m_http_stream << "POST /resource1/file2 HTTP/1.1\r\n";
	m_http_stream << "Content-Length: 4\r\n\r\n";
	m_http_stream << "1234";
	m_http_stream.flush();
	checkResponseHead(m_http_stream, 204);
	BOOST_CHECK(m_content_length == 0);
	checkFileContents("sandbox/file2", "xyz\n1234\n");
}

BOOST_AUTO_TEST_CASE(checkResponseToPostRequestForNonexistentFile) {
	m_http_stream << "POST /resource1/file3 HTTP/1.1\r\n";
	m_http_stream << "Content-Length: 4\r\n\r\n";
	m_http_stream << "1234";
	m_http_stream.flush();
	checkResponseHead(m_http_stream, 201);
	//TODO: check Location: header.
	checkWebServerResponseContent(m_http_stream, boost::regex(".*201\\sCreated.*"));
	checkFileContents("sandbox/file3", "1234\n");
}

// Doesn't pass yet.
//BOOST_AUTO_TEST_CASE(checkResponseToPutRequestForDefaultFile) {
//	m_http_stream << "PUT /resource1 HTTP/1.1\r\n";
//	m_http_stream << "Content-Length: 4\r\n\r\n";
//	m_http_stream << "1234";
//	m_http_stream.flush();
//	checkResponseHead(m_http_stream, 204);
//	BOOST_CHECK(m_content_length == 0);
//	checkFileContents("sandbox/file1", "1234\n");
//}

BOOST_AUTO_TEST_CASE(checkResponseToPutRequestForSpecifiedFile) {
	m_http_stream << "PUT /resource1/file2 HTTP/1.1\r\n";
	m_http_stream << "Content-Length: 4\r\n\r\n";
	m_http_stream << "1234";
	m_http_stream.flush();
	checkResponseHead(m_http_stream, 204);
	BOOST_CHECK(m_content_length == 0);
	checkFileContents("sandbox/file2", "1234\n");
}

/* TODO: write a couple tests of PUT requests with header 'If-None-Match: *'.
   The response should be 412 (Precondition Failed) if the file exists, and
   204 if it doesn't.
*/

BOOST_AUTO_TEST_CASE(checkResponseToPutRequestForNonexistentFile) {
	m_http_stream << "PUT /resource1/file3 HTTP/1.1\r\n";
	m_http_stream << "Content-Length: 4\r\n\r\n";
	m_http_stream << "1234";
	m_http_stream.flush();
	checkResponseHead(m_http_stream, 201);
	//TODO: check Location: header.
	checkWebServerResponseContent(m_http_stream, boost::regex(".*201\\sCreated.*"));
	checkFileContents("sandbox/file3", "1234\n");
}

// This doesn't pass, and maybe it shouldn't.  Currently it returns 500 (Server Error).
//BOOST_AUTO_TEST_CASE(checkResponseToPutRequestForFileInNonexistentDirectory) {
//	m_http_stream << "PUT /resource1/dir2/file4 HTTP/1.1\r\n";
//	m_http_stream << "Content-Length: 4\r\n\r\n";
//	m_http_stream << "1234";
//	m_http_stream.flush();
//	checkResponseHead(m_http_stream, 201);
//	//TODO: check Location: header.
//	checkWebServerResponseContent(m_http_stream, boost::regex(".*201\\sCreated.*"));
//	checkFileContents("sandbox/dir2/file4", "1234\n");
//}

/* TODO: write a test with a PUT request with a file that's not in the configured directory.
   This is also needed for other methods.
*/

/* TODO: write a test with a PUT request with no content and check that it throws an 
   appropriate exception.  Currently it crashes.
*/

// Doesn't pass yet.
//BOOST_AUTO_TEST_CASE(checkResponseToDeleteRequestForDefaultFile) {
//	sendRequestAndCheckResponseHead(m_http_stream, "DELETE", "/resource1", 204);
//	BOOST_CHECK(m_content_length == 0);
//}

BOOST_AUTO_TEST_CASE(checkResponseToDeleteRequestForSpecifiedFile) {
	sendRequestAndCheckResponseHead(m_http_stream, "DELETE", "/resource1/file2", 204);
	BOOST_CHECK(m_content_length == 0);
	BOOST_CHECK(!boost::filesystem::exists("sandbox/file2"));
}

BOOST_AUTO_TEST_CASE(checkResponseToDeleteRequestForNonexistentFile) {
	sendRequestAndCheckResponseHead(m_http_stream, "DELETE", "/resource1/file3", 404);
	checkWebServerResponseContent(m_http_stream, boost::regex(".*404\\sNot\\sFound.*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToDeleteRequestForDirectory) {
	sendRequestAndCheckResponseHead(m_http_stream, "DELETE", "/resource1/dir1", 204);
	BOOST_CHECK(m_content_length == 0);
	BOOST_CHECK(!boost::filesystem::exists("sandbox/dir1"));
}

BOOST_AUTO_TEST_CASE(checkResponseToDeleteRequestForOpenFile) {
	boost::filesystem::ofstream openFile("sandbox/file2");
	sendRequestAndCheckResponseHead(m_http_stream, "DELETE", "/resource1/file2", 500);
	checkWebServerResponseContent(m_http_stream, boost::regex(".*500\\sServer\\sError.*"));
}

BOOST_AUTO_TEST_SUITE_END()
