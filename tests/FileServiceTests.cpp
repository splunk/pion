// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/scoped_array.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/string/find.hpp>
#include <pion/PionConfig.hpp>
#include <pion/PionPlugin.hpp>
#include <pion/PionScheduler.hpp>
#include <pion/net/HTTPRequest.hpp>
#include <pion/net/HTTPResponse.hpp>
#include <pion/net/WebService.hpp>
#include <pion/net/WebServer.hpp>

using namespace pion;
using namespace pion::net;
using boost::asio::ip::tcp;

PION_DECLARE_PLUGIN(FileService)

#if defined(_MSC_VER)
	#if defined(_DEBUG) && defined(PION_FULL)
		static const std::string PATH_TO_PLUGINS("../../bin/Debug_DLL_full");
	#elif defined(_DEBUG) && !defined(PION_FULL)
		static const std::string PATH_TO_PLUGINS("../../bin/Debug_DLL");
	#elif defined(NDEBUG) && defined(PION_FULL)
		static const std::string PATH_TO_PLUGINS("../../bin/Release_DLL_full");
	#elif defined(NDEBUG) && !defined(PION_FULL)
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


#ifndef PION_STATIC_LINKING

struct PluginPtrWithPluginLoaded_F : public PionPluginPtr<WebService> {
	PluginPtrWithPluginLoaded_F() { 
		setup_logging_for_unit_tests();
		PionPlugin::resetPluginDirectories();
		PionPlugin::addPluginDirectory(PATH_TO_PLUGINS);
		s = NULL;
		open("FileService");
	}
	~PluginPtrWithPluginLoaded_F() {
		if (s) destroy(s);
	}

	WebService* s;
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
	NewlyLoadedFileService_F() : m_scheduler(), m_server(m_scheduler, 8080) {
		PionPlugin::resetPluginDirectories();
		PionPlugin::addPluginDirectory(PATH_TO_PLUGINS);

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

		m_server.loadService("/resource1", "FileService");
	}
	~NewlyLoadedFileService_F() {
		boost::filesystem::remove_all("sandbox");
	}
	
	inline boost::asio::io_service& getIOService(void) { return m_scheduler.getIOService(); }
	
	PionScheduler	m_scheduler;
	WebServer		m_server;
};

BOOST_FIXTURE_TEST_SUITE(NewlyLoadedFileService_S, NewlyLoadedFileService_F)

BOOST_AUTO_TEST_CASE(checkSetServiceOptionDirectoryWithExistingDirectoryDoesntThrow) {
	BOOST_CHECK_NO_THROW(m_server.setServiceOption("/resource1", "directory", "sandbox"));
}

BOOST_AUTO_TEST_CASE(checkSetServiceOptionDirectoryWithNonexistentDirectoryThrows) {
	BOOST_CHECK_THROW(m_server.setServiceOption("/resource1", "directory", "NotADirectory"), WebServer::WebServiceException);
}

BOOST_AUTO_TEST_CASE(checkSetServiceOptionFileWithExistingFileDoesntThrow) {
	BOOST_CHECK_NO_THROW(m_server.setServiceOption("/resource1", "file", "sandbox/file1"));
}

BOOST_AUTO_TEST_CASE(checkSetServiceOptionFileWithNonexistentFileThrows) {
	BOOST_CHECK_THROW(m_server.setServiceOption("/resource1", "file", "NotAFile"), WebServer::WebServiceException);
}

// TODO: tests for options "cache" and "scan"

BOOST_AUTO_TEST_CASE(checkSetServiceOptionMaxChunkSizeWithSizeZeroThrows) {
	BOOST_CHECK_NO_THROW(m_server.setServiceOption("/resource1", "max_chunk_size", "0"));
}

BOOST_AUTO_TEST_CASE(checkSetServiceOptionMaxChunkSizeWithNonZeroSizeDoesntThrow) {
	BOOST_CHECK_NO_THROW(m_server.setServiceOption("/resource1", "max_chunk_size", "100"));
}

BOOST_AUTO_TEST_CASE(checkSetServiceOptionWritableToTrueDoesntThrow) {
	BOOST_CHECK_NO_THROW(m_server.setServiceOption("/resource1", "writable", "true"));
}

BOOST_AUTO_TEST_CASE(checkSetServiceOptionWritableToFalseDoesntThrow) {
	BOOST_CHECK_NO_THROW(m_server.setServiceOption("/resource1", "writable", "false"));
}

BOOST_AUTO_TEST_CASE(checkSetServiceOptionWritableToNonBooleanThrows) {
	BOOST_REQUIRE_THROW(m_server.setServiceOption("/resource1", "writable", "3"), WebServer::WebServiceException);
	try {
		m_server.setServiceOption("/resource1", "writable", "3");
	} catch (WebServer::WebServiceException& e) {
		BOOST_CHECK_EQUAL(e.what(), "WebService (/resource1): FileService invalid value for writable option: 3");
	}
	//Original exception is FileService::InvalidOptionValueException
}

BOOST_AUTO_TEST_CASE(checkSetServiceOptionWithInvalidOptionNameThrows) {
	BOOST_CHECK_THROW(m_server.setServiceOption("/resource1", "NotAnOption", "value1"), WebServer::WebServiceException);
}

BOOST_AUTO_TEST_SUITE_END()	

class RunningFileService_F : public NewlyLoadedFileService_F {
public:
	RunningFileService_F() {
		m_content_length = 0;
		m_server.setServiceOption("/resource1", "directory", "sandbox");
		m_server.setServiceOption("/resource1", "file", "sandbox/file1");
		m_server.start();

		// open a connection
		tcp::endpoint http_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 8080);
		m_http_stream.connect(http_endpoint);
	}
	~RunningFileService_F() {
	}
	
	/**
	 * sends a request to the local HTTP server
	 *
	 * @param request_method e.g., HEAD, GET, POST
	 * @param resource name of the HTTP resource to request
	 * @param expected_response_code expected status code of the response
	 */
	inline void sendRequestAndCheckResponseHead(const std::string& request_method,
												const std::string& resource,
												unsigned int expected_response_code = 200)
	{
		// send HTTP request to the server
		m_http_stream << request_method << " " << resource << " HTTP/1.1" << HTTPTypes::STRING_CRLF << HTTPTypes::STRING_CRLF;
		m_http_stream.flush();

		checkResponseHead(expected_response_code);
	}
	
	/**
	 * check status line and headers of response
	 *
	 * @param expected_response_code expected status code of the response
	 */
	inline void checkResponseHead(unsigned int expected_response_code = 200)
	{
		const boost::regex regex_get_response_code("^HTTP/1\\.1\\s(\\d+)\\s.*");
		const boost::regex regex_response_header("(^[A-Za-z0-9_-]+):\\s*(.*)\\r");
		const boost::regex regex_content_length_header("^Content-Length:\\s(\\d+).*", boost::regex::icase);
		const boost::regex regex_response_end("^\\s*$");

		// receive status line from the server
		std::string rsp_line;
		boost::smatch rx_matches;
		unsigned int response_code = 0;
		BOOST_REQUIRE(std::getline(m_http_stream, rsp_line));
		BOOST_REQUIRE(boost::regex_match(rsp_line, rx_matches, regex_get_response_code));
		BOOST_REQUIRE(rx_matches.size() == 2);

		// extract response status code
		response_code = boost::lexical_cast<unsigned int>(rx_matches[1]);
		BOOST_CHECK_EQUAL(response_code, expected_response_code);

		// read response headers
		while (true) {
			BOOST_REQUIRE(std::getline(m_http_stream, rsp_line));
			// check for end of response headers (empty line)
			if (boost::regex_match(rsp_line, rx_matches, regex_response_end))
				break;
			// check validity of response header
			BOOST_REQUIRE(boost::regex_match(rsp_line, rx_matches, regex_response_header));
			m_response_headers[rx_matches[1]] = rx_matches[2];
			// check for content-length response header
			if (boost::regex_match(rsp_line, rx_matches, regex_content_length_header)) {
				if (rx_matches.size() == 2)
					m_content_length = boost::lexical_cast<unsigned long>(rx_matches[1]);
			}
		}

		// Responses with status-code 201 (Created) must have a Location header.
		if (response_code == 201) {
			BOOST_CHECK(m_response_headers.find("Location") != m_response_headers.end());
		}
		// Responses with status-code 405 (Method Not Allowed) must have an Allow header.
		if (response_code == 405) {
			BOOST_CHECK(m_response_headers.find("Allow") != m_response_headers.end());
		}

		// TODO: Any headers that are always required for specific status-codes should be checked here.
	}
	
	inline void sendRequestWithContent(const std::string& request_method,
									   const std::string& resource,
									   const std::string& content)
	{
		// send HTTP request to the server
		m_http_stream << request_method << " " << resource << " HTTP/1.1" << HTTPTypes::STRING_CRLF;
		m_http_stream << "Content-Length: " << content.size() << HTTPTypes::STRING_CRLF << HTTPTypes::STRING_CRLF << content;
		m_http_stream.flush();
	}
	
	/**
	 * checks response content validity for the local HTTP server
	 *
	 * @param content_regex regex that the response content should match
	 */
	inline void checkWebServerResponseContent(const boost::regex& content_regex)
	{
		BOOST_CHECK(m_content_length > 0);

		// read in the response content
		boost::scoped_array<char> content_buf(new char[m_content_length + 1]);
		BOOST_CHECK(m_http_stream.read(content_buf.get(), m_content_length));
		content_buf[m_content_length] = '\0';
		
		// check the response content
		BOOST_CHECK(boost::regex_match(content_buf.get(), content_regex));
	}
	
	unsigned long m_content_length;
	tcp::iostream m_http_stream;
	std::map<std::string, std::string> m_response_headers;
};

BOOST_FIXTURE_TEST_SUITE(RunningFileService_S, RunningFileService_F)

BOOST_AUTO_TEST_CASE(checkResponseToGetRequestForDefaultFile) {
	sendRequestAndCheckResponseHead("GET", "/resource1");
	checkWebServerResponseContent(boost::regex("abc\\s*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToHeadRequestForDefaultFile) {
	sendRequestAndCheckResponseHead("HEAD", "/resource1");
	BOOST_CHECK(m_content_length == 0);
}

BOOST_AUTO_TEST_CASE(checkResponseToGetRequestForDefaultFileAfterDeletingIt) {
	boost::filesystem::remove("sandbox/file1");
	sendRequestAndCheckResponseHead("GET", "/resource1", 404);
	checkWebServerResponseContent(boost::regex(".*404\\sNot\\sFound.*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToHeadRequestForDefaultFileAfterDeletingIt) {
	boost::filesystem::remove("sandbox/file1");
	sendRequestAndCheckResponseHead("HEAD", "/resource1", 404);
	BOOST_CHECK(m_content_length == 0);
}

BOOST_AUTO_TEST_CASE(checkResponseToGetRequestForSpecifiedFile) {
	sendRequestAndCheckResponseHead("GET", "/resource1/file2");
	checkWebServerResponseContent(boost::regex("xyz\\s*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToGetRequestForEmptyFile) {
	sendRequestAndCheckResponseHead("GET", "/resource1/emptyFile");
	BOOST_CHECK(m_content_length == 0);
}

BOOST_AUTO_TEST_CASE(checkResponseToGetRequestForNonexistentFile) {
	sendRequestAndCheckResponseHead("GET", "/resource1/file3", 404);
	checkWebServerResponseContent(boost::regex(".*404\\sNot\\sFound.*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToHeadRequestForNonexistentFile) {
	sendRequestAndCheckResponseHead("HEAD", "/resource1/file3", 404);
	BOOST_CHECK(m_content_length == 0);
}

BOOST_AUTO_TEST_CASE(checkResponseToGetRequestForDirectory) {
	sendRequestAndCheckResponseHead("GET", "/resource1/dir1", 403);
	checkWebServerResponseContent(boost::regex(".*403\\sForbidden.*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToHeadRequestForDirectory) {
	sendRequestAndCheckResponseHead("HEAD", "/resource1/dir1", 403);
	BOOST_CHECK(m_content_length == 0);
}

BOOST_AUTO_TEST_CASE(checkResponseToGetRequestForFileOutsideDirectory) {
	sendRequestAndCheckResponseHead("GET", "/resource1/../someFile", 403);
	checkWebServerResponseContent(boost::regex(".*403\\sForbidden.*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToHeadRequestForFileOutsideDirectory) {
	sendRequestAndCheckResponseHead("HEAD", "/resource1/../someFile", 403);
	BOOST_CHECK(m_content_length == 0);
}

BOOST_AUTO_TEST_CASE(checkResponseToPostRequestForDefaultFile) {
	sendRequestAndCheckResponseHead("POST", "/resource1", 405);
	checkWebServerResponseContent(boost::regex(".*405\\sMethod\\sNot\\sAllowed.*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToPostRequestForNonexistentFile) {
	sendRequestAndCheckResponseHead("POST", "/resource1/file3", 405);
	checkWebServerResponseContent(boost::regex(".*405\\sMethod\\sNot\\sAllowed.*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToPutRequestForDefaultFile) {
	sendRequestAndCheckResponseHead("PUT", "/resource1", 405);
	checkWebServerResponseContent(boost::regex(".*405\\sMethod\\sNot\\sAllowed.*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToPutRequestForNonexistentFile) {
	sendRequestAndCheckResponseHead("PUT", "/resource1/file3", 405);
	checkWebServerResponseContent(boost::regex(".*405\\sMethod\\sNot\\sAllowed.*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToDeleteRequestForDefaultFile) {
	sendRequestAndCheckResponseHead("DELETE", "/resource1", 405);
	checkWebServerResponseContent(boost::regex(".*405\\sMethod\\sNot\\sAllowed.*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToDeleteRequestForNonexistentFile) {
	sendRequestAndCheckResponseHead("DELETE", "/resource1/file3", 405);
	checkWebServerResponseContent(boost::regex(".*405\\sMethod\\sNot\\sAllowed.*"));
}

BOOST_AUTO_TEST_CASE(checkAllowHeader) {
	sendRequestAndCheckResponseHead("POST", "/resource1", 405);
	checkWebServerResponseContent(boost::regex(".*405\\sMethod\\sNot\\sAllowed.*"));
	BOOST_CHECK(boost::algorithm::find_first(m_response_headers["Allow"], "GET"));
	BOOST_CHECK(boost::algorithm::find_first(m_response_headers["Allow"], "HEAD"));
	BOOST_CHECK(!boost::algorithm::find_first(m_response_headers["Allow"], "PUT"));
	BOOST_CHECK(!boost::algorithm::find_first(m_response_headers["Allow"], "POST"));
	BOOST_CHECK(!boost::algorithm::find_first(m_response_headers["Allow"], "DELETE"));
}

BOOST_AUTO_TEST_CASE(checkResponseToTraceRequestForDefaultFile) {
	sendRequestAndCheckResponseHead("TRACE", "/resource1", 501);
	checkWebServerResponseContent(boost::regex(".*501\\sNot\\sImplemented.*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToRequestWithBogusMethod) {
	sendRequestAndCheckResponseHead("BOGUS", "/resource1", 501);
	checkWebServerResponseContent(boost::regex(".*501\\sNot\\sImplemented.*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToHTTP_1_0_Request) {
	m_http_stream << "GET /resource1 HTTP/1.0" << HTTPTypes::STRING_CRLF << HTTPTypes::STRING_CRLF;
	m_http_stream.flush();

	checkResponseHead(200);
	checkWebServerResponseContent(boost::regex("abc\\s*"));
}

BOOST_AUTO_TEST_SUITE_END()

class RunningFileServiceWithWritingEnabled_F : public RunningFileService_F {
public:
	RunningFileServiceWithWritingEnabled_F() {
		m_server.setServiceOption("/resource1", "writable", "true");
	}
	~RunningFileServiceWithWritingEnabled_F() {
	}

	void checkFileContents(const std::string& filename,
						   const std::string& expected_contents)
	{
		boost::filesystem::ifstream in(filename);
		std::string actual_contents;
		char c;
		while (in.get(c)) actual_contents += c;
		BOOST_CHECK_EQUAL(actual_contents, expected_contents);
	}
};

BOOST_FIXTURE_TEST_SUITE(RunningFileServiceWithWritingEnabled_S, RunningFileServiceWithWritingEnabled_F)

BOOST_AUTO_TEST_CASE(checkResponseToPostRequestForDefaultFile) {
	sendRequestWithContent("POST", "/resource1", "1234");
	checkResponseHead(204);
	BOOST_CHECK(m_content_length == 0);
	checkFileContents("sandbox/file1", "abc\n1234");
}

BOOST_AUTO_TEST_CASE(checkResponseToPostRequestForSpecifiedFile) {
	sendRequestWithContent("POST", "/resource1/file2", "1234\n");
	checkResponseHead(204);
	BOOST_CHECK(m_content_length == 0);
	checkFileContents("sandbox/file2", "xyz\n1234\n");
}

BOOST_AUTO_TEST_CASE(checkResponseToPostRequestForNonexistentFile) {
	sendRequestWithContent("POST", "/resource1/file3", "1234\n");
	checkResponseHead(201);
	BOOST_CHECK_EQUAL(m_response_headers["Location"], "/resource1/file3");
	checkWebServerResponseContent(boost::regex(".*201\\sCreated.*"));
	checkFileContents("sandbox/file3", "1234\n");
}

BOOST_AUTO_TEST_CASE(checkResponseToPutRequestForDefaultFile) {
	sendRequestWithContent("PUT", "/resource1", "1234\n");
	checkResponseHead(204);
	BOOST_CHECK(m_content_length == 0);
	checkFileContents("sandbox/file1", "1234\n");
}

BOOST_AUTO_TEST_CASE(checkResponseToPutRequestForSpecifiedFile) {
	sendRequestWithContent("PUT", "/resource1/file2", "1234");
	checkResponseHead(204);
	BOOST_CHECK(m_content_length == 0);
	checkFileContents("sandbox/file2", "1234");
}

/* TODO: write a couple tests of PUT requests with header 'If-None-Match: *'.
   The response should be 412 (Precondition Failed) if the file exists, and
   204 if it doesn't.
*/

BOOST_AUTO_TEST_CASE(checkResponseToPutRequestForNonexistentFile) {
	sendRequestWithContent("PUT", "/resource1/file3", "1234\n");
	checkResponseHead(201);
	BOOST_CHECK_EQUAL(m_response_headers["Location"], "/resource1/file3");
	checkWebServerResponseContent(boost::regex(".*201\\sCreated.*"));
	checkFileContents("sandbox/file3", "1234\n");
}

BOOST_AUTO_TEST_CASE(checkResponseToPutRequestForFileInNonexistentDirectory) {
	sendRequestWithContent("PUT", "/resource1/dir2/file4", "1234");
	checkResponseHead(404);
	checkWebServerResponseContent(boost::regex(".*404\\sNot Found.*"));
}

/* TODO: write tests for POST and PUT with a file that's not in the configured directory.
*/

/* TODO: write a test with a PUT request with no content and check that it throws an 
   appropriate exception.  Currently it crashes.
*/

BOOST_AUTO_TEST_CASE(checkResponseToDeleteRequestForDefaultFile) {
	sendRequestAndCheckResponseHead("DELETE", "/resource1", 204);
	BOOST_CHECK(m_content_length == 0);
}

BOOST_AUTO_TEST_CASE(checkResponseToDeleteRequestForSpecifiedFile) {
	sendRequestAndCheckResponseHead("DELETE", "/resource1/file2", 204);
	BOOST_CHECK(m_content_length == 0);
	BOOST_CHECK(!boost::filesystem::exists("sandbox/file2"));
}

BOOST_AUTO_TEST_CASE(checkResponseToDeleteRequestForNonexistentFile) {
	sendRequestAndCheckResponseHead("DELETE", "/resource1/file3", 404);
	checkWebServerResponseContent(boost::regex(".*404\\sNot\\sFound.*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToDeleteRequestForDirectory) {
	sendRequestAndCheckResponseHead("DELETE", "/resource1/dir1", 403);
	checkWebServerResponseContent(boost::regex(".*403\\sForbidden.*"));
}

BOOST_AUTO_TEST_CASE(checkResponseToDeleteRequestForFileOutsideDirectory) {
	sendRequestAndCheckResponseHead("DELETE", "/resource1/../someFile", 403);
	checkWebServerResponseContent(boost::regex(".*403\\sForbidden.*"));
}

#if defined(PION_WIN32) && defined(_MSC_VER)
// this test only works on Windows with MSVC
BOOST_AUTO_TEST_CASE(checkResponseToDeleteRequestForOpenFile) {
	boost::filesystem::ofstream openFile("sandbox/file2");
	sendRequestAndCheckResponseHead("DELETE", "/resource1/file2", 500);
	checkWebServerResponseContent(boost::regex(".*500\\sServer\\sError.*"));
}
#endif

BOOST_AUTO_TEST_CASE(checkResponseToChunkedPutRequest) {
	m_http_stream << "PUT /resource1 HTTP/1.1" << HTTPTypes::STRING_CRLF;
	m_http_stream << HTTPTypes::HEADER_TRANSFER_ENCODING << ": chunked" << HTTPTypes::STRING_CRLF << HTTPTypes::STRING_CRLF;
	// write first chunk size
	m_http_stream << "A" << HTTPTypes::STRING_CRLF;
	// write first chunk 
	m_http_stream << "abcdefghij" << HTTPTypes::STRING_CRLF;
	// write second chunk size
	m_http_stream << "5" << HTTPTypes::STRING_CRLF;
	// write second chunk 
	m_http_stream << "klmno" << HTTPTypes::STRING_CRLF;
	// write final chunk size
	m_http_stream << "0" << HTTPTypes::STRING_CRLF;
	m_http_stream << HTTPTypes::STRING_CRLF;
	m_http_stream.flush();
	checkResponseHead(204);
	BOOST_CHECK(m_content_length == 0);
	checkFileContents("sandbox/file1", "abcdefghijklmno");
}

BOOST_AUTO_TEST_CASE(checkResponseToChunkedPostRequest) {
	m_http_stream << "POST /resource1 HTTP/1.1" << HTTPTypes::STRING_CRLF;
	m_http_stream << HTTPTypes::HEADER_TRANSFER_ENCODING << ": chunked" << HTTPTypes::STRING_CRLF << HTTPTypes::STRING_CRLF;
	// write first chunk size
	m_http_stream << "A" << HTTPTypes::STRING_CRLF;
	// write first chunk 
	m_http_stream << "abcdefghij" << HTTPTypes::STRING_CRLF;
	// write second chunk size
	m_http_stream << "5" << HTTPTypes::STRING_CRLF;
	// write second chunk 
	m_http_stream << "klmno" << HTTPTypes::STRING_CRLF;
	// write final chunk size
	m_http_stream << "0" << HTTPTypes::STRING_CRLF;
	m_http_stream << HTTPTypes::STRING_CRLF;
	m_http_stream.flush();
	checkResponseHead(204);
	BOOST_CHECK(m_content_length == 0);
	checkFileContents("sandbox/file1", "abc\nabcdefghijklmno");
}

BOOST_AUTO_TEST_SUITE_END()

const char g_file4_contents[] = "012345678901234";

class RunningFileServiceWithMaxChunkSizeSet_F : public RunningFileService_F {
public:
	enum _size_constants { MAX_CHUNK_SIZE = 10 };

	RunningFileServiceWithMaxChunkSizeSet_F() {
		m_server.setServiceOption("/resource1", "max_chunk_size", boost::lexical_cast<std::string>(MAX_CHUNK_SIZE));

		// make sure the length of the test data is in the range expected by the tests
		m_file4_len = strlen(g_file4_contents);
		BOOST_REQUIRE(m_file4_len > MAX_CHUNK_SIZE);
		BOOST_REQUIRE(m_file4_len < 2 * MAX_CHUNK_SIZE);

		FILE* fp = fopen("sandbox/file4", "wb");
		fwrite(g_file4_contents, 1, m_file4_len, fp);
		fclose(fp);
	}
	~RunningFileServiceWithMaxChunkSizeSet_F() {
	}

	char m_data_buf[2 * MAX_CHUNK_SIZE];
	int m_file4_len;
};

BOOST_FIXTURE_TEST_SUITE(RunningFileServiceWithMaxChunkSizeSet_S, RunningFileServiceWithMaxChunkSizeSet_F)

BOOST_AUTO_TEST_CASE(checkResponseToHTTP_1_1_Request) {
	m_http_stream << "GET /resource1/file4 HTTP/1.1" << HTTPTypes::STRING_CRLF << HTTPTypes::STRING_CRLF;
	m_http_stream.flush();

	checkResponseHead(200);

	//From RFC 2616, sec 4.4:
	//Messages MUST NOT include both a Content-Length header field and a non-identity transfer-coding.
	BOOST_CHECK(m_response_headers.find("Content-Length") == m_response_headers.end());

	// extract first chunk size
	unsigned int chunk_size_1 = 0;
	m_http_stream >> std::hex >> chunk_size_1;
	BOOST_CHECK_EQUAL(chunk_size_1, static_cast<unsigned int>(MAX_CHUNK_SIZE));

	// expect CRLF following chunk size
	char two_bytes[2];
	BOOST_CHECK(m_http_stream.read(two_bytes, 2));
	BOOST_CHECK(strncmp(two_bytes, HTTPTypes::STRING_CRLF.c_str(), 2) == 0);

	// read first chunk
	m_http_stream.read(m_data_buf, chunk_size_1);

	// expect CRLF following chunk
	BOOST_CHECK(m_http_stream.read(two_bytes, 2));
	BOOST_CHECK(strncmp(two_bytes, HTTPTypes::STRING_CRLF.c_str(), 2) == 0);

	// extract second chunk size
	unsigned int chunk_size_2 = 0;
	m_http_stream >> std::hex >> chunk_size_2;
	BOOST_CHECK_EQUAL(chunk_size_2, m_file4_len - static_cast<unsigned int>(MAX_CHUNK_SIZE));
	
	// expect CRLF following chunk size
	BOOST_CHECK(m_http_stream.read(two_bytes, 2));
	BOOST_CHECK(strncmp(two_bytes, HTTPTypes::STRING_CRLF.c_str(), 2) == 0);

	// read second chunk
	m_http_stream.read(m_data_buf + chunk_size_1, chunk_size_2);

	// verify reconstructed data
	BOOST_CHECK(strncmp(m_data_buf, g_file4_contents, chunk_size_1 + chunk_size_2) == 0);

	// expect CRLF following chunk
	memset(two_bytes, 0, 2);
	BOOST_CHECK(m_http_stream.read(two_bytes, 2));
	BOOST_CHECK(strncmp(two_bytes, HTTPTypes::STRING_CRLF.c_str(), 2) == 0);

	// extract final chunk size
	unsigned int chunk_size_3 = 99;
	m_http_stream >> std::hex >> chunk_size_3;
	BOOST_CHECK(chunk_size_3 == 0);

	// there could be a trailer here, but so far there isn't...

	// expect CRLF following final chunk (and optional trailer)
	memset(two_bytes, 0, 2);
	BOOST_CHECK(m_http_stream.read(two_bytes, 2));
	BOOST_CHECK(strncmp(two_bytes, HTTPTypes::STRING_CRLF.c_str(), 2) == 0);
}

BOOST_AUTO_TEST_CASE(checkHTTPMessageReceive) {
	// open (another) connection
	TCPConnection tcp_conn(getIOService());
	boost::system::error_code error_code;
	error_code = tcp_conn.connect(boost::asio::ip::address::from_string("127.0.0.1"), 8080);
	BOOST_REQUIRE(!error_code);

	// send request to the server
	HTTPRequest http_request("/resource1/file4");
	http_request.send(tcp_conn, error_code);
	BOOST_REQUIRE(!error_code);

	// receive the response from the server
	HTTPResponse http_response;
	http_response.receive(tcp_conn, error_code);
	BOOST_REQUIRE(!error_code);

	// verify that the headers are as expected for a chunked response 
	BOOST_CHECK_EQUAL(http_response.getHeader(HTTPTypes::HEADER_TRANSFER_ENCODING), "chunked");
	BOOST_CHECK_EQUAL(http_response.getHeader(HTTPTypes::HEADER_CONTENT_LENGTH), "");

	// verify reconstructed data
	BOOST_CHECK_EQUAL(http_response.getContentLength(), static_cast<size_t>(m_file4_len));
	BOOST_CHECK(strncmp(http_response.getContent(), g_file4_contents, m_file4_len) == 0);
}

BOOST_AUTO_TEST_CASE(checkResponseToHTTP_1_0_Request) {
	m_http_stream << "GET /resource1/file4 HTTP/1.0" << HTTPTypes::STRING_CRLF << HTTPTypes::STRING_CRLF;
	m_http_stream.flush();

	checkResponseHead(200);

	// We expect no Content-Length header...
	BOOST_CHECK(m_response_headers.find("Content-Length") == m_response_headers.end());
	// ... but connection should be closed after sending all the data, so we check for EOF.
	int i;
	for (i = 0; !m_http_stream.eof(); ++i) {
		m_http_stream.read(&m_data_buf[i], 1);
	}

	BOOST_CHECK_EQUAL(i, m_file4_len + 1);
	BOOST_CHECK(strncmp(m_data_buf, g_file4_contents, m_file4_len) == 0);
}

BOOST_AUTO_TEST_SUITE_END()
