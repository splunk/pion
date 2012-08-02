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
    HTTPRequest req1;
    req1.addHeader("Test", "HTTPMessage");
    req1.setMethod("GET");
    HTTPRequest req2(req1);
    BOOST_CHECK_EQUAL(req1.getMethod(), "GET");
    BOOST_CHECK_EQUAL(req1.getMethod(), req2.getMethod());
    BOOST_CHECK_EQUAL(req1.getHeader("Test"), "HTTPMessage");
    BOOST_CHECK_EQUAL(req1.getHeader("Test"), req2.getHeader("Test"));
}

BOOST_AUTO_TEST_CASE(checkHTTPRequestAssignmentOperator) {
    HTTPRequest req1, req2;
    req1.setMethod("GET");
    req1.addHeader("Test", "HTTPMessage");
    req2 = req1;
    BOOST_CHECK_EQUAL(req1.getMethod(), "GET");
    BOOST_CHECK_EQUAL(req1.getMethod(), req2.getMethod());
    BOOST_CHECK_EQUAL(req1.getHeader("Test"), "HTTPMessage");
    BOOST_CHECK_EQUAL(req1.getHeader("Test"), req2.getHeader("Test"));
}

BOOST_AUTO_TEST_CASE(checkHTTPResponseCopyConstructor) {
    HTTPResponse rsp1;
    rsp1.addHeader("Test", "HTTPMessage");
    rsp1.setStatusCode(199);
    HTTPResponse rsp2(rsp1);
    BOOST_CHECK_EQUAL(rsp1.getStatusCode(), 199U);
    BOOST_CHECK_EQUAL(rsp1.getStatusCode(), rsp2.getStatusCode());
    BOOST_CHECK_EQUAL(rsp1.getHeader("Test"), "HTTPMessage");
    BOOST_CHECK_EQUAL(rsp1.getHeader("Test"), rsp2.getHeader("Test"));
}

BOOST_AUTO_TEST_CASE(checkHTTPResponseAssignmentOperator) {
    HTTPResponse rsp1, rsp2;
    rsp1.addHeader("Test", "HTTPMessage");
    rsp1.setStatusCode(199);
    rsp2 = rsp1;
    BOOST_CHECK_EQUAL(rsp1.getStatusCode(), 199U);
    BOOST_CHECK_EQUAL(rsp1.getStatusCode(), rsp2.getStatusCode());
    BOOST_CHECK_EQUAL(rsp1.getHeader("Test"), "HTTPMessage");
    BOOST_CHECK_EQUAL(rsp1.getHeader("Test"), rsp2.getHeader("Test"));
}

BOOST_AUTO_TEST_CASE(checkGetFirstLineForRequest) {
    HTTPRequest http_request;
    
    http_request.setMethod("GET");
    http_request.setResource("/");

    BOOST_CHECK_EQUAL(http_request.getFirstLine(), "GET / HTTP/1.1");
    
    http_request.setMethod("POST");

    BOOST_CHECK_EQUAL(http_request.getFirstLine(), "POST / HTTP/1.1");

    http_request.setResource("/index.html");

    BOOST_CHECK_EQUAL(http_request.getFirstLine(), "POST /index.html HTTP/1.1");

    http_request.setVersionMajor(1);
    http_request.setVersionMinor(0);

    BOOST_CHECK_EQUAL(http_request.getFirstLine(), "POST /index.html HTTP/1.0");
}

BOOST_AUTO_TEST_CASE(checkGetFirstLineForResponse) {
    HTTPResponse http_response;
    
    http_response.setStatusCode(HTTPTypes::RESPONSE_CODE_OK);
    http_response.setStatusMessage(HTTPTypes::RESPONSE_MESSAGE_OK);

    BOOST_CHECK_EQUAL(http_response.getFirstLine(), "HTTP/1.1 200 OK");
    
    http_response.setStatusCode(HTTPTypes::RESPONSE_CODE_NOT_FOUND);

    BOOST_CHECK_EQUAL(http_response.getFirstLine(), "HTTP/1.1 404 OK");

    http_response.setStatusMessage(HTTPTypes::RESPONSE_MESSAGE_NOT_FOUND);

    BOOST_CHECK_EQUAL(http_response.getFirstLine(), "HTTP/1.1 404 Not Found");
}


#define FIXTURE_TYPE_LIST(F) boost::mpl::list<F<HTTPRequest>, F<HTTPResponse> >

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
    BOOST_CHECK_EQUAL(F::getContentLength(), static_cast<size_t>(0));
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkSetContentLengthDoesntThrow) {
    BOOST_CHECK_NO_THROW(F::setContentLength(10));
    BOOST_CHECK_NO_THROW(F::setContentLength(0));
}

// Is this what we want?  Note that the length is zero here.
BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkCreateContentBufferReturnsPointer) {
    BOOST_CHECK(F::createContentBuffer() != NULL);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentReturnsEmptyString) {
    BOOST_CHECK(F::getContent() != NULL);
    BOOST_CHECK_EQUAL(strcmp(F::getContent(), ""), 0);
    BOOST_CHECK(!F::isContentBufferAllocated());
}

// Is this what we want?  Note that the length is zero here.
BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentReturnsPointerAfterCreatingContentBuffer) {
    F::createContentBuffer();
    BOOST_CHECK(F::getContent() != NULL);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkIsValidReturnsFalse) {
    BOOST_CHECK(!F::isValid());
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(testChunksSupportedAccessors) {
    BOOST_CHECK(!F::getChunksSupported());
    F::setChunksSupported(true);
    BOOST_CHECK(F::getChunksSupported());
    F::setChunksSupported(false);
    BOOST_CHECK(!F::getChunksSupported());
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(testHeaderCaseIsIgnored) {
    const std::string xml_content_type("text/xml");

    F::addHeader(HTTPTypes::HEADER_CONTENT_TYPE, xml_content_type);
    BOOST_CHECK_EQUAL(F::getHeader("CoNTenT-TYPe"), xml_content_type);

    F::addHeader("content-length", "10");
    BOOST_CHECK_EQUAL(F::getHeader(HTTPTypes::HEADER_CONTENT_LENGTH), "10");
}

BOOST_AUTO_TEST_SUITE_END()

template<typename ConcreteMessageType>
class HTTPMessageWithContentLengthSet_F : public ConcreteMessageType {
public:
    HTTPMessageWithContentLengthSet_F() {
        this->setContentLength(20);
    }
    ~HTTPMessageWithContentLengthSet_F() {
    }
};

BOOST_AUTO_TEST_SUITE_FIXTURE_TEMPLATE(HTTPMessageWithContentLengthSet_S,
                                       FIXTURE_TYPE_LIST(HTTPMessageWithContentLengthSet_F))

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentLengthReturnsCorrectLength) {
    BOOST_CHECK_EQUAL(F::getContentLength(), static_cast<size_t>(20));
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentLengthAfterSettingLengthAgain) {
    F::setContentLength(30);
    BOOST_CHECK_EQUAL(F::getContentLength(), static_cast<size_t>(30));
    F::setContentLength(0);
    BOOST_CHECK_EQUAL(F::getContentLength(), static_cast<size_t>(0));
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkCreateContentBufferVarious) {
    char *ptr = F::createContentBuffer();
    BOOST_CHECK(ptr != NULL);
    BOOST_CHECK_EQUAL(ptr, F::getContent());
    BOOST_CHECK_EQUAL(F::getContentBufferSize(), 20);
    BOOST_CHECK(F::isContentBufferAllocated());
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentReturnsEmptyString) {
    BOOST_CHECK(F::getContent() != NULL);
    BOOST_CHECK_EQUAL(strcmp(F::getContent(), ""), 0);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkIsValidReturnsFalse) {
    BOOST_CHECK(!F::isValid());
}

BOOST_AUTO_TEST_SUITE_END()

template<typename ConcreteMessageType>
class HTTPMessageWithContentBufferCreated_F : public ConcreteMessageType {
public:
    HTTPMessageWithContentBufferCreated_F() {
        m_len = 10;
        this->setContentLength(m_len);
        m_content_buffer = this->createContentBuffer();
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
    BOOST_CHECK(F::createContentBuffer() != NULL);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentReturnsPointer) {
    BOOST_CHECK(F::getContent() != NULL);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentReturnsWhatWasWrittenToBuffer) {
    char buf[] = {0, 1, 2, 3, 127, 0, -1, -2, -3, -128};
    BOOST_CHECK_EQUAL(sizeof(buf), static_cast<unsigned int>(F::m_len));
    memcpy(F::m_content_buffer, buf, F::m_len);
    BOOST_CHECK(memcmp(buf, F::getContent(), F::m_len) == 0);
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
        this->setContentLength(m_len);
        m_content_buffer = this->createContentBuffer();
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
    BOOST_CHECK(F::getContent() != NULL);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentReturnsWhatWasWrittenToBuffer) {
    BOOST_CHECK(memcmp(TEXT_STRING_1, F::getContent(), F::m_len) == 0);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentAfterChangingContent) {
    BOOST_CHECK_EQUAL(strlen(TEXT_STRING_2), static_cast<unsigned int>(F::m_len));
    memcpy(F::m_content_buffer, TEXT_STRING_2, F::m_len);
    BOOST_CHECK(memcmp(TEXT_STRING_2, F::getContent(), F::m_len) == 0);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentAfterChangingSizeAndContent) {
    F::m_len = strlen(TEXT_STRING_3);
    F::setContentLength(F::m_len);
    F::m_content_buffer = F::createContentBuffer();
    memcpy(F::m_content_buffer, TEXT_STRING_3, F::m_len);
    BOOST_CHECK(memcmp(TEXT_STRING_3, F::getContent(), F::m_len) == 0);
}

// This is just for convenience for text-only post content.
// Strictly speaking, getContent() guarantees nothing beyond the buffer.
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
    std::string s2 = F::getContent();
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
    HTTPRequest req;
    req.setResource("/test.html");
    req.addHeader("Test", "Something");
    
    // write to file
    boost::system::error_code ec;
    req.write(m_file, ec);
    BOOST_REQUIRE(! ec);
    m_file.flush();
    
    // read from file
    HTTPRequest req2;
    m_file.clear();
    m_file.seekg(0);
    req2.read(m_file, ec);
    BOOST_REQUIRE(! ec);

    // make sure we're now at EOF
    HTTPRequest req3;
    req3.read(m_file, ec);
    BOOST_CHECK_EQUAL(ec, boost::system::errc::io_error);
    
    // check request read from file
    BOOST_CHECK_EQUAL(req2.getResource(), "/test.html");
    BOOST_CHECK_EQUAL(req2.getHeader("Test"), "Something");
    BOOST_CHECK_EQUAL(req2.getContentLength(), 0U);

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
    HTTPResponse rsp;
    rsp.setStatusCode(202);
    rsp.setStatusMessage("Hi There");
    rsp.addHeader("HeaderA", "a value");
    
    // write to file
    boost::system::error_code ec;
    rsp.write(m_file, ec);
    BOOST_REQUIRE(! ec);
    m_file.flush();
    
    // read from file
    HTTPResponse rsp2;
    m_file.clear();
    m_file.seekg(0);
    rsp2.read(m_file, ec);
    BOOST_REQUIRE(! ec);
    
    // make sure we're now at EOF
    HTTPResponse rsp3;
    rsp3.read(m_file, ec);
    BOOST_CHECK_EQUAL(ec, boost::system::errc::io_error);
    
    // check response read from file
    BOOST_CHECK_EQUAL(rsp2.getStatusCode(), 202U);
    BOOST_CHECK_EQUAL(rsp2.getStatusMessage(), "Hi There");
    BOOST_CHECK_EQUAL(rsp2.getHeader("HeaderA"), "a value");
    BOOST_CHECK_EQUAL(rsp2.getContentLength(), 0U);

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
    HTTPRequest req;
    HTTPResponse rsp;

    // build a request & write to file
    req.setResource("/test.html");
    req.addHeader("Test", "Something");
    req.write(m_file, ec);
    BOOST_REQUIRE(! ec);

    // build a response & write to file
    rsp.setStatusCode(202);
    rsp.setStatusMessage("Hi There");
    rsp.addHeader("HeaderA", "a value");
    rsp.setContent("My message content");
    rsp.write(m_file, ec);
    BOOST_REQUIRE(! ec);

    // another request
    req.setResource("/blah.html");
    req.addHeader("HeaderA", "a value");
    req.setContent("My request content");
    req.write(m_file, ec);
    BOOST_REQUIRE(! ec);
    
    // another response
    rsp.setStatusCode(302);
    rsp.setStatusMessage("Hello There");
    rsp.addHeader("HeaderB", "another value");
    rsp.clearContent();
    rsp.write(m_file, ec);
    BOOST_REQUIRE(! ec);

    // one last request
    req.setResource("/last.html");
    req.addHeader("HeaderB", "Bvalue");
    req.clearContent();
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
    HTTPRequest req1;
    req1.read(m_file, ec);
    BOOST_REQUIRE(! ec);
    
    // read first response
    HTTPResponse rsp1;
    rsp1.read(m_file, ec);
    BOOST_REQUIRE(! ec);

    // read second request
    HTTPRequest req2;
    req2.read(m_file, ec);
    BOOST_REQUIRE(! ec);
    
    // read second response
    HTTPResponse rsp2;
    rsp2.read(m_file, ec);
    BOOST_REQUIRE(! ec);
    BOOST_CHECK_EQUAL(rsp2.getStatusCode(), 302U);
    BOOST_CHECK_EQUAL(rsp2.getStatusMessage(), "Hello There");

    // read third request
    HTTPRequest req3;
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
