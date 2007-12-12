// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/PionConfig.hpp>
#include <pion/net/HTTPRequest.hpp>
#include <boost/test/unit_test.hpp>

using namespace pion;
using namespace pion::net;

class NewHTTPRequest_F : public HTTPRequest {
public:
	NewHTTPRequest_F() {
	}
	~NewHTTPRequest_F() {
	}
};

BOOST_FIXTURE_TEST_SUITE(NewHTTPRequest_S, NewHTTPRequest_F)

BOOST_AUTO_TEST_CASE(checkSetMethodWithValidMethodDoesntThrow) {
	BOOST_CHECK_NO_THROW(setMethod("GET"));
}

// Is this what we want?
BOOST_AUTO_TEST_CASE(checkSetMethodWithInvalidMethodDoesntThrow) {
	BOOST_CHECK_NO_THROW(setMethod("NOT_A_VALID_METHOD"));
}

BOOST_AUTO_TEST_CASE(checkGetMethodReturnsGET) {
	BOOST_CHECK_EQUAL(getMethod(), "GET");
}

BOOST_AUTO_TEST_CASE(checkGetMethodReturnsWhatSetMethodAssigns) {
	BOOST_CHECK_NO_THROW(setMethod("POST"));
	BOOST_CHECK_EQUAL(getMethod(), "POST");
}

BOOST_AUTO_TEST_CASE(checkGetMethodReturnsWhateverSetMethodAssigns) {
	BOOST_CHECK_NO_THROW(setMethod("BLAH_BLAH_BLAH"));
	BOOST_CHECK_EQUAL(getMethod(), "BLAH_BLAH_BLAH");
}

BOOST_AUTO_TEST_CASE(checkGetContentLengthReturnsZero) {
	BOOST_CHECK_EQUAL(getContentLength(), static_cast<size_t>(0));
}

BOOST_AUTO_TEST_CASE(checkSetContentLengthDoesntThrow) {
	BOOST_CHECK_NO_THROW(setContentLength(10));
	BOOST_CHECK_NO_THROW(setContentLength(0));
}

// Is this what we want?  Note that the length is zero here.
BOOST_AUTO_TEST_CASE(checkCreateContentBufferReturnsPointer) {
	BOOST_CHECK(createContentBuffer() != NULL);
}

BOOST_AUTO_TEST_CASE(checkGetContentReturnsNull) {
	BOOST_CHECK(getContent() == NULL);
}

// Is this what we want?  Note that the length is zero here.
BOOST_AUTO_TEST_CASE(checkGetContentReturnsPointerAfterCreatingContentBuffer) {
	createContentBuffer();
	BOOST_CHECK(getContent() != NULL);
}

BOOST_AUTO_TEST_CASE(checkIsValidReturnsFalse) {
	BOOST_CHECK(!isValid());
}

BOOST_AUTO_TEST_CASE(testChunksSupportedAccessors) {
	BOOST_CHECK(!getChunksSupported());
	setChunksSupported(true);
	BOOST_CHECK(getChunksSupported());
	setChunksSupported(false);
	BOOST_CHECK(!getChunksSupported());
}

BOOST_AUTO_TEST_SUITE_END()

class HTTPRequestWithContentLengthSet_F : public HTTPRequest {
public:
	HTTPRequestWithContentLengthSet_F() {
		setContentLength(20);
	}
	~HTTPRequestWithContentLengthSet_F() {
	}
};

BOOST_FIXTURE_TEST_SUITE(HTTPRequestWithContentLengthSet_S, HTTPRequestWithContentLengthSet_F)

BOOST_AUTO_TEST_CASE(checkGetContentLengthReturnsCorrectLength) {
	BOOST_CHECK_EQUAL(getContentLength(), static_cast<size_t>(20));
}

BOOST_AUTO_TEST_CASE(checkGetContentLengthAfterSettingLengthAgain) {
	setContentLength(30);
	BOOST_CHECK_EQUAL(getContentLength(), static_cast<size_t>(30));
	setContentLength(0);
	BOOST_CHECK_EQUAL(getContentLength(), static_cast<size_t>(0));
}

BOOST_AUTO_TEST_CASE(checkCreateContentBufferReturnsPointer) {
	BOOST_CHECK(createContentBuffer() != NULL);
}

BOOST_AUTO_TEST_CASE(checkGetContentReturnsNull) {
	BOOST_CHECK(getContent() == NULL);
}

BOOST_AUTO_TEST_CASE(checkIsValidReturnsFalse) {
	BOOST_CHECK(!isValid());
}

BOOST_AUTO_TEST_SUITE_END()

class HTTPRequestWithContentBufferCreated_F : public HTTPRequest {
public:
	HTTPRequestWithContentBufferCreated_F() {
		m_len = 10;
		setContentLength(m_len);
		m_content_buffer = createContentBuffer();
	}
	~HTTPRequestWithContentBufferCreated_F() {
	}

	int m_len;
	char* m_content_buffer;
};

BOOST_FIXTURE_TEST_SUITE(HTTPRequestWithContentBufferCreated_S, HTTPRequestWithContentBufferCreated_F)

// ???
BOOST_AUTO_TEST_CASE(checkCreateContentBufferAgainReturnsPointer) {
	BOOST_CHECK(createContentBuffer() != NULL);
}

BOOST_AUTO_TEST_CASE(checkGetContentReturnsPointer) {
	BOOST_CHECK(getContent() != NULL);
}

BOOST_AUTO_TEST_CASE(checkGetContentReturnsWhatWasWrittenToBuffer) {
	char buf[] = {0, 1, 2, 3, 127, 0, -1, -2, -3, -128};
	BOOST_CHECK_EQUAL(sizeof(buf), static_cast<unsigned int>(m_len));
	memcpy(m_content_buffer, buf, m_len);
	BOOST_CHECK(memcmp(buf, getContent(), m_len) == 0);
}

BOOST_AUTO_TEST_SUITE_END()

static const char TEXT_STRING_1[] = "0123456789";
static const char TEXT_STRING_2[] = "9876543210";
static const char TEXT_STRING_3[] = "0123456789abcde";

class HTTPRequestWithTextOnlyContent_F : public HTTPRequest {
public:
	HTTPRequestWithTextOnlyContent_F() {
		m_len = strlen(TEXT_STRING_1);
		setContentLength(m_len);
		m_content_buffer = createContentBuffer();
		memcpy(m_content_buffer, TEXT_STRING_1, m_len);
	}
	~HTTPRequestWithTextOnlyContent_F() {
	}

	size_t m_len;
	char* m_content_buffer;
};

BOOST_FIXTURE_TEST_SUITE(HTTPRequestWithTextOnlyContent_S, HTTPRequestWithTextOnlyContent_F)

BOOST_AUTO_TEST_CASE(checkGetContentReturnsPointer) {
	BOOST_CHECK(getContent() != NULL);
}

BOOST_AUTO_TEST_CASE(checkGetContentReturnsWhatWasWrittenToBuffer) {
	BOOST_CHECK(memcmp(TEXT_STRING_1, getContent(), m_len) == 0);
}

BOOST_AUTO_TEST_CASE(checkGetContentAfterChangingContent) {
	BOOST_CHECK_EQUAL(strlen(TEXT_STRING_2), static_cast<unsigned int>(m_len));
	memcpy(m_content_buffer, TEXT_STRING_2, m_len);
	BOOST_CHECK(memcmp(TEXT_STRING_2, getContent(), m_len) == 0);
}

BOOST_AUTO_TEST_CASE(checkGetContentAfterChangingSizeAndContent) {
	m_len = strlen(TEXT_STRING_3);
	setContentLength(m_len);
	m_content_buffer = createContentBuffer();
	memcpy(m_content_buffer, TEXT_STRING_3, m_len);
	BOOST_CHECK(memcmp(TEXT_STRING_3, getContent(), m_len) == 0);
}

// This is just for convenience for text-only post content.
// Strictly speaking, getContent() guarantees nothing beyond the buffer.
BOOST_AUTO_TEST_CASE(checkGetContentReturnsZeroTerminatedBuffer) {
	// This crashes due to bug in Boost.Test if m_content_buffer[m_len] is negative, so use workaround.
	//BOOST_CHECK_EQUAL(m_content_buffer[m_len], static_cast<char>(0));

	// Illustration of bug:
	//char c1 = -2;
	//char c2 = -2;
	//BOOST_CHECK_EQUAL(c1, c2);

	BOOST_CHECK_EQUAL(static_cast<int>(m_content_buffer[m_len]), static_cast<int>(0));
}

// See comments for checkGetContentReturnsZeroTerminatedBuffer.
BOOST_AUTO_TEST_CASE(checkContentPointerUsableAsString) {
	const std::string s1 = TEXT_STRING_1;
	std::string s2 = getContent();
	BOOST_CHECK_EQUAL(s1, s2);
}

BOOST_AUTO_TEST_SUITE_END()
