// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <sstream>
#include <vector>
#include <algorithm>
#include <pion/config.hpp>
#include <pion/http/message.hpp>
#include <pion/http/request.hpp>
#include <pion/http/response.hpp>
#include <pion/test/unit_test.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/foreach.hpp>

using namespace pion;


BOOST_AUTO_TEST_CASE(checkHTTPRequestCopyConstructor) {
    http::request req1;
    req1.add_header("Test", "HTTPMessage");
    req1.set_method("GET");
    http::request req2(req1);
    BOOST_CHECK_EQUAL(req1.get_method(), "GET");
    BOOST_CHECK_EQUAL(req1.get_method(), req2.get_method());
    BOOST_CHECK_EQUAL(req1.get_header("Test"), "HTTPMessage");
    BOOST_CHECK_EQUAL(req1.get_header("Test"), req2.get_header("Test"));
}

BOOST_AUTO_TEST_CASE(checkHTTPRequestAssignmentOperator) {
    http::request req1, req2;
    req1.set_method("GET");
    req1.add_header("Test", "HTTPMessage");
    req2 = req1;
    BOOST_CHECK_EQUAL(req1.get_method(), "GET");
    BOOST_CHECK_EQUAL(req1.get_method(), req2.get_method());
    BOOST_CHECK_EQUAL(req1.get_header("Test"), "HTTPMessage");
    BOOST_CHECK_EQUAL(req1.get_header("Test"), req2.get_header("Test"));
}

BOOST_AUTO_TEST_CASE(checkHTTPResponseCopyConstructor) {
    http::response rsp1;
    rsp1.add_header("Test", "HTTPMessage");
    rsp1.set_status_code(199);
    http::response rsp2(rsp1);
    BOOST_CHECK_EQUAL(rsp1.get_status_code(), 199U);
    BOOST_CHECK_EQUAL(rsp1.get_status_code(), rsp2.get_status_code());
    BOOST_CHECK_EQUAL(rsp1.get_header("Test"), "HTTPMessage");
    BOOST_CHECK_EQUAL(rsp1.get_header("Test"), rsp2.get_header("Test"));
}

BOOST_AUTO_TEST_CASE(checkHTTPResponseAssignmentOperator) {
    http::response rsp1, rsp2;
    rsp1.add_header("Test", "HTTPMessage");
    rsp1.set_status_code(199);
    rsp2 = rsp1;
    BOOST_CHECK_EQUAL(rsp1.get_status_code(), 199U);
    BOOST_CHECK_EQUAL(rsp1.get_status_code(), rsp2.get_status_code());
    BOOST_CHECK_EQUAL(rsp1.get_header("Test"), "HTTPMessage");
    BOOST_CHECK_EQUAL(rsp1.get_header("Test"), rsp2.get_header("Test"));
}

BOOST_AUTO_TEST_CASE(checkGetFirstLineForRequest) {
    http::request http_request;
    
    http_request.set_method("GET");
    http_request.set_resource("/");

    BOOST_CHECK_EQUAL(http_request.get_first_line(), "GET / HTTP/1.1");
    
    http_request.set_method("POST");

    BOOST_CHECK_EQUAL(http_request.get_first_line(), "POST / HTTP/1.1");

    http_request.set_resource("/index.html");

    BOOST_CHECK_EQUAL(http_request.get_first_line(), "POST /index.html HTTP/1.1");

    http_request.set_version_major(1);
    http_request.set_version_minor(0);

    BOOST_CHECK_EQUAL(http_request.get_first_line(), "POST /index.html HTTP/1.0");
}

BOOST_AUTO_TEST_CASE(checkGetFirstLineForResponse) {
    http::response http_response;
    
    http_response.set_status_code(http::types::RESPONSE_CODE_OK);
    http_response.set_status_message(http::types::RESPONSE_MESSAGE_OK);

    BOOST_CHECK_EQUAL(http_response.get_first_line(), "HTTP/1.1 200 OK");
    
    http_response.set_status_code(http::types::RESPONSE_CODE_NOT_FOUND);

    BOOST_CHECK_EQUAL(http_response.get_first_line(), "HTTP/1.1 404 OK");

    http_response.set_status_message(http::types::RESPONSE_MESSAGE_NOT_FOUND);

    BOOST_CHECK_EQUAL(http_response.get_first_line(), "HTTP/1.1 404 Not Found");
}


#define FIXTURE_TYPE_LIST(F) boost::mpl::list<F<http::request>, F<http::response> >

template<typename ConcreteMessageType>
class NewHTTPMessage_F : public ConcreteMessageType {
public:
    NewHTTPMessage_F() {
    }
    ~NewHTTPMessage_F() {
    }
};

BOOST_AUTO_TEST_SUITE_FIXTURE_TEMPLATE(NewHTTPMessage_S,
                                       FIXTURE_TYPE_LIST(NewHTTPMessage_F))

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentLengthReturnsZero) {
    BOOST_CHECK_EQUAL(F::get_content_length(), static_cast<size_t>(0));
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkSetContentLengthDoesntThrow) {
    BOOST_CHECK_NO_THROW(F::set_content_length(10));
    BOOST_CHECK_NO_THROW(F::set_content_length(0));
}

// Is this what we want?  Note that the length is zero here.
BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkCreateContentBufferReturnsPointer) {
    BOOST_CHECK(F::create_content_buffer() != NULL);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentReturnsEmptyString) {
    BOOST_CHECK(F::get_content() != NULL);
    BOOST_CHECK_EQUAL(strcmp(F::get_content(), ""), 0);
    BOOST_CHECK(!F::is_content_buffer_allocated());
}

// Is this what we want?  Note that the length is zero here.
BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentReturnsPointerAfterCreatingContentBuffer) {
    F::create_content_buffer();
    BOOST_CHECK(F::get_content() != NULL);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkIsValidReturnsFalse) {
    BOOST_CHECK(!F::is_valid());
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(testChunksSupportedAccessors) {
    BOOST_CHECK(!F::get_chunks_supported());
    F::set_chunks_supported(true);
    BOOST_CHECK(F::get_chunks_supported());
    F::set_chunks_supported(false);
    BOOST_CHECK(!F::get_chunks_supported());
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(testHeaderCaseIsIgnored) {
    const std::string xml_content_type("text/xml");

    F::add_header(http::types::HEADER_CONTENT_TYPE, xml_content_type);
    BOOST_CHECK_EQUAL(F::get_header("CoNTenT-TYPe"), xml_content_type);

    F::add_header("content-length", "10");
    BOOST_CHECK_EQUAL(F::get_header(http::types::HEADER_CONTENT_LENGTH), "10");
}

BOOST_AUTO_TEST_SUITE_END()

template<typename ConcreteMessageType>
class HTTPMessageWithContentLengthSet_F : public ConcreteMessageType {
public:
    HTTPMessageWithContentLengthSet_F() {
        this->set_content_length(20);
    }
    ~HTTPMessageWithContentLengthSet_F() {
    }
};

BOOST_AUTO_TEST_SUITE_FIXTURE_TEMPLATE(HTTPMessageWithContentLengthSet_S,
                                       FIXTURE_TYPE_LIST(HTTPMessageWithContentLengthSet_F))

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentLengthReturnsCorrectLength) {
    BOOST_CHECK_EQUAL(F::get_content_length(), static_cast<size_t>(20));
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentLengthAfterSettingLengthAgain) {
    F::set_content_length(30);
    BOOST_CHECK_EQUAL(F::get_content_length(), static_cast<size_t>(30));
    F::set_content_length(0);
    BOOST_CHECK_EQUAL(F::get_content_length(), static_cast<size_t>(0));
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkCreateContentBufferVarious) {
    char *ptr = F::create_content_buffer();
    BOOST_CHECK(ptr != NULL);
    BOOST_CHECK_EQUAL(ptr, F::get_content());
    BOOST_CHECK_EQUAL(F::get_content_buffer_size(), 20U);
    BOOST_CHECK(F::is_content_buffer_allocated());
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentReturnsEmptyString) {
    BOOST_CHECK(F::get_content() != NULL);
    BOOST_CHECK_EQUAL(strcmp(F::get_content(), ""), 0);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkIsValidReturnsFalse) {
    BOOST_CHECK(!F::is_valid());
}

BOOST_AUTO_TEST_SUITE_END()

template<typename ConcreteMessageType>
class HTTPMessageWithContentBufferCreated_F : public ConcreteMessageType {
public:
    HTTPMessageWithContentBufferCreated_F() {
        m_len = 10;
        this->set_content_length(m_len);
        m_content_buffer = this->create_content_buffer();
    }
    ~HTTPMessageWithContentBufferCreated_F() {
    }

    int m_len;
    char* m_content_buffer;
};

BOOST_AUTO_TEST_SUITE_FIXTURE_TEMPLATE(HTTPMessageWithContentBufferCreated_S,
                                       FIXTURE_TYPE_LIST(HTTPMessageWithContentBufferCreated_F))

// ???
BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkCreateContentBufferAgainReturnsPointer) {
    BOOST_CHECK(F::create_content_buffer() != NULL);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentReturnsPointer) {
    BOOST_CHECK(F::get_content() != NULL);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentReturnsWhatWasWrittenToBuffer) {
    char buf[] = {0, 1, 2, 3, 127, 0, -1, -2, -3, -128};
    BOOST_CHECK_EQUAL(sizeof(buf), static_cast<unsigned int>(F::m_len));
    memcpy(F::m_content_buffer, buf, F::m_len);
    BOOST_CHECK(memcmp(buf, F::get_content(), F::m_len) == 0);
}

BOOST_AUTO_TEST_SUITE_END()

static const char TEXT_STRING_1[] = "0123456789";
static const char TEXT_STRING_2[] = "9876543210";
static const char TEXT_STRING_3[] = "0123456789abcde";

template<typename ConcreteMessageType>
class HTTPMessageWithTextOnlyContent_F : public ConcreteMessageType {
public:
    HTTPMessageWithTextOnlyContent_F() {
        m_len = strlen(TEXT_STRING_1);
        this->set_content_length(m_len);
        m_content_buffer = this->create_content_buffer();
        memcpy(m_content_buffer, TEXT_STRING_1, m_len);
    }
    ~HTTPMessageWithTextOnlyContent_F() {
    }

    size_t m_len;
    char* m_content_buffer;
};

BOOST_AUTO_TEST_SUITE_FIXTURE_TEMPLATE(HTTPMessageWithTextOnlyContent_S, 
                                       FIXTURE_TYPE_LIST(HTTPMessageWithTextOnlyContent_F))

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentReturnsPointer) {
    BOOST_CHECK(F::get_content() != NULL);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentReturnsWhatWasWrittenToBuffer) {
    BOOST_CHECK(memcmp(TEXT_STRING_1, F::get_content(), F::m_len) == 0);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentAfterChangingContent) {
    BOOST_CHECK_EQUAL(strlen(TEXT_STRING_2), static_cast<unsigned int>(F::m_len));
    memcpy(F::m_content_buffer, TEXT_STRING_2, F::m_len);
    BOOST_CHECK(memcmp(TEXT_STRING_2, F::get_content(), F::m_len) == 0);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentAfterChangingSizeAndContent) {
    F::m_len = strlen(TEXT_STRING_3);
    F::set_content_length(F::m_len);
    F::m_content_buffer = F::create_content_buffer();
    memcpy(F::m_content_buffer, TEXT_STRING_3, F::m_len);
    BOOST_CHECK(memcmp(TEXT_STRING_3, F::get_content(), F::m_len) == 0);
}

// This is just for convenience for text-only post content.
// Strictly speaking, get_content() guarantees nothing beyond the buffer.
BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentReturnsZeroTerminatedBuffer) {
    // This crashes due to bug in Boost.Test if m_content_buffer[m_len] is negative, so use workaround.
    //BOOST_CHECK_EQUAL(m_content_buffer[m_len], static_cast<char>(0));

    // Illustration of bug:
    //char c1 = -2;
    //char c2 = -2;
    //BOOST_CHECK_EQUAL(c1, c2);

    BOOST_CHECK_EQUAL(static_cast<int>(F::m_content_buffer[F::m_len]), static_cast<int>(0));
}

// See comments for checkGetContentReturnsZeroTerminatedBuffer.
BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkContentPointerUsableAsString) {
    const std::string s1 = TEXT_STRING_1;
    std::string s2 = F::get_content();
    BOOST_CHECK_EQUAL(s1, s2);
}

BOOST_AUTO_TEST_SUITE_END()


/// simple fixture for testing read() and write() methods
class HTTPMessageReadWrite_F {
public:
    HTTPMessageReadWrite_F()
        : m_filename("output.tmp")
    {
        openNewFile();
    }
    
    ~HTTPMessageReadWrite_F() {
        m_file.close();
        boost::filesystem::remove(m_filename);
    }
    
    void openNewFile(void) {
        if (m_file.is_open()) {
            m_file.close();
            m_file.clear();
        }
        m_file.open(m_filename.c_str(), std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary);
        BOOST_REQUIRE(m_file.is_open());
    }
    
    std::string getFileContents(void) {
        // if only it were this easy... unfortunately order of headers can vary
        //std::stringstream ss;
        //m_file.seekg(0);
        //ss << m_file.rdbuf();
        //return ss.str();

        char line[256];
        std::stringstream ss;
        std::vector<std::string> lines;
        bool is_first_line = true;

        m_file.clear();
        m_file.seekg(0);
        
        while (m_file.getline(line, 255)) {
            std::size_t len = strlen(line);
            if (len > 0 && line[len-1]=='\r')
                line[len-1] = '\0';
            if (is_first_line) {
                ss << line << "\r\n";
                lines.clear();
                is_first_line = false;
            } else if (line[0] == '\0') {
                std::sort(lines.begin(), lines.end());
                BOOST_FOREACH(const std::string& l, lines) {
                    ss << l << "\r\n";
                }
                ss << "\r\n";
                lines.clear();
                is_first_line = true;
            } else {
                lines.push_back(line);
            }
        }
        
        std::sort(lines.begin(), lines.end());
        BOOST_FOREACH(const std::string& l, lines) {
            ss << l << "\r\n";
        }

        return ss.str();
    }

    std::string     m_filename;
    std::fstream    m_file;
};

BOOST_FIXTURE_TEST_SUITE(HTTPMessageReadWrite_S, HTTPMessageReadWrite_F)

BOOST_AUTO_TEST_CASE(checkWriteReadHTTPRequestNoContent) {
    // build a request
    http::request req;
    req.set_resource("/test.html");
    req.add_header("Test", "Something");
    
    // write to file
    boost::system::error_code ec;
    req.write(m_file, ec);
    BOOST_REQUIRE(! ec);
    m_file.flush();
    
    // read from file
    http::request req2;
    m_file.clear();
    m_file.seekg(0);
    req2.read(m_file, ec);
    BOOST_REQUIRE(! ec);

    // make sure we're now at EOF
    http::request req3;
    req3.read(m_file, ec);
    BOOST_CHECK_EQUAL(ec, boost::system::errc::io_error);
    
    // check request read from file
    BOOST_CHECK_EQUAL(req2.get_resource(), "/test.html");
    BOOST_CHECK_EQUAL(req2.get_header("Test"), "Something");
    BOOST_CHECK_EQUAL(req2.get_content_length(), 0U);

    // validate file contents
    std::string req_contents = getFileContents();
    BOOST_CHECK_EQUAL(req_contents, "GET /test.html HTTP/1.1\r\nConnection: Keep-Alive\r\nContent-Length: 0\r\nTest: Something\r\n\r\n");

    // create a new file for req2
    openNewFile();
    req2.write(m_file, ec);
    BOOST_REQUIRE(! ec);
    m_file.flush();

    // make sure file matches original (no loss/change from read/write cycle)
    std::string req2_contents = getFileContents();
    BOOST_CHECK_EQUAL(req_contents, req2_contents);
}

BOOST_AUTO_TEST_CASE(checkWriteReadHTTPResponseNoContent) {
    // build a response
    http::response rsp;
    rsp.set_status_code(202);
    rsp.set_status_message("Hi There");
    rsp.add_header("HeaderA", "a value");
    
    // write to file
    boost::system::error_code ec;
    rsp.write(m_file, ec);
    BOOST_REQUIRE(! ec);
    m_file.flush();
    
    // read from file
    http::response rsp2;
    m_file.clear();
    m_file.seekg(0);
    rsp2.read(m_file, ec);
    BOOST_REQUIRE(! ec);
    
    // make sure we're now at EOF
    http::response rsp3;
    rsp3.read(m_file, ec);
    BOOST_CHECK_EQUAL(ec, boost::system::errc::io_error);
    
    // check response read from file
    BOOST_CHECK_EQUAL(rsp2.get_status_code(), 202U);
    BOOST_CHECK_EQUAL(rsp2.get_status_message(), "Hi There");
    BOOST_CHECK_EQUAL(rsp2.get_header("HeaderA"), "a value");
    BOOST_CHECK_EQUAL(rsp2.get_content_length(), 0U);

    // validate file contents
    std::string rsp_contents = getFileContents();
    BOOST_CHECK_EQUAL(rsp_contents, "HTTP/1.1 202 Hi There\r\nConnection: Keep-Alive\r\nContent-Length: 0\r\nHeaderA: a value\r\n\r\n");

    // create a new file for rsp2
    openNewFile();
    rsp2.write(m_file, ec);
    BOOST_REQUIRE(! ec);
    m_file.flush();

    // make sure file matches original (no loss/change from read/write cycle)
    std::string rsp2_contents = getFileContents();
    BOOST_CHECK_EQUAL(rsp_contents, rsp2_contents);
}

BOOST_AUTO_TEST_CASE(checkWriteReadMixedMessages) {
    boost::system::error_code ec;
    http::request req;
    http::response rsp;

    // build a request & write to file
    req.set_resource("/test.html");
    req.add_header("Test", "Something");
    req.write(m_file, ec);
    BOOST_REQUIRE(! ec);

    // build a response & write to file
    rsp.set_status_code(202);
    rsp.set_status_message("Hi There");
    rsp.add_header("HeaderA", "a value");
    rsp.set_content("My message content");
    rsp.write(m_file, ec);
    BOOST_REQUIRE(! ec);

    // another request
    req.set_resource("/blah.html");
    req.add_header("HeaderA", "a value");
    req.set_content("My request content");
    req.write(m_file, ec);
    BOOST_REQUIRE(! ec);
    
    // another response
    rsp.set_status_code(302);
    rsp.set_status_message("Hello There");
    rsp.add_header("HeaderB", "another value");
    rsp.clear_content();
    rsp.write(m_file, ec);
    BOOST_REQUIRE(! ec);

    // one last request
    req.set_resource("/last.html");
    req.add_header("HeaderB", "Bvalue");
    req.clear_content();
    req.write(m_file, ec);
    BOOST_REQUIRE(! ec);

    // flush file output
    m_file.flush();
    
    // validate file contents
    std::string contents = getFileContents();
    BOOST_CHECK_EQUAL(contents, "GET /test.html HTTP/1.1\r\nConnection: Keep-Alive\r\nContent-Length: 0\r\nTest: Something\r\n\r\n"
        "HTTP/1.1 202 Hi There\r\nConnection: Keep-Alive\r\nContent-Length: 18\r\nHeaderA: a value\r\n\r\nMy message content"
        "GET /blah.html HTTP/1.1\r\nConnection: Keep-Alive\r\nContent-Length: 18\r\nHeaderA: a value\r\nTest: Something\r\n\r\nMy request content"
        "HTTP/1.1 302 Hello There\r\nConnection: Keep-Alive\r\nContent-Length: 0\r\nHeaderA: a value\r\nHeaderB: another value\r\n\r\n"
        "GET /last.html HTTP/1.1\r\nConnection: Keep-Alive\r\nContent-Length: 0\r\nHeaderA: a value\r\nHeaderB: Bvalue\r\nTest: Something\r\n\r\n");

    m_file.clear();
    m_file.seekg(0);

    // read first request
    http::request req1;
    req1.read(m_file, ec);
    BOOST_REQUIRE(! ec);
    
    // read first response
    http::response rsp1;
    rsp1.read(m_file, ec);
    BOOST_REQUIRE(! ec);

    // read second request
    http::request req2;
    req2.read(m_file, ec);
    BOOST_REQUIRE(! ec);
    
    // read second response
    http::response rsp2;
    rsp2.read(m_file, ec);
    BOOST_REQUIRE(! ec);
    BOOST_CHECK_EQUAL(rsp2.get_status_code(), 302U);
    BOOST_CHECK_EQUAL(rsp2.get_status_message(), "Hello There");

    // read third request
    http::request req3;
    req3.read(m_file, ec);
    BOOST_REQUIRE(! ec);
    
    // write everything back to new file
    openNewFile();
    req1.write(m_file, ec);
    BOOST_REQUIRE(! ec);
    rsp1.write(m_file, ec);
    BOOST_REQUIRE(! ec);
    req2.write(m_file, ec);
    BOOST_REQUIRE(! ec);
    rsp2.write(m_file, ec);
    BOOST_REQUIRE(! ec);
    req3.write(m_file, ec);
    BOOST_REQUIRE(! ec);

    // flush file output
    m_file.flush();

    // make sure file matches original (no loss/change from read/write cycle)
    std::string new_contents = getFileContents();
    BOOST_CHECK_EQUAL(contents, new_contents);
}

BOOST_AUTO_TEST_SUITE_END()
