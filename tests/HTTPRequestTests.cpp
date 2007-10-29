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

BOOST_AUTO_TEST_CASE(checkGetMethodReturnsEmptyString) {
	BOOST_CHECK_EQUAL(getMethod(), "");
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
BOOST_AUTO_TEST_CASE(checkCreatePostContentBufferReturnsPointer) {
	BOOST_CHECK(createPostContentBuffer() != NULL);
}

BOOST_AUTO_TEST_CASE(checkGetPostContentReturnsNull) {
	BOOST_CHECK(getPostContent() == NULL);
}

// Is this what we want?  Note that the length is zero here.
BOOST_AUTO_TEST_CASE(checkGetPostContentReturnsPointerAfterCreatingPostContentBuffer) {
	createPostContentBuffer();
	BOOST_CHECK(getPostContent() != NULL);
}

BOOST_AUTO_TEST_CASE(checkIsValidReturnsFalse) {
	BOOST_CHECK(!isValid());
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

BOOST_AUTO_TEST_CASE(checkCreatePostContentBufferReturnsPointer) {
	BOOST_CHECK(createPostContentBuffer() != NULL);
}

BOOST_AUTO_TEST_CASE(checkGetPostContentReturnsNull) {
	BOOST_CHECK(getPostContent() == NULL);
}

BOOST_AUTO_TEST_CASE(checkIsValidReturnsFalse) {
	BOOST_CHECK(!isValid());
}

BOOST_AUTO_TEST_SUITE_END()

class HTTPRequestWithPostContentBufferCreated_F : public HTTPRequest {
public:
	HTTPRequestWithPostContentBufferCreated_F() {
		m_len = 10;
		setContentLength(m_len);
		m_post_buffer = createPostContentBuffer();
	}
	~HTTPRequestWithPostContentBufferCreated_F() {
	}

	int m_len;
	char* m_post_buffer;
};

BOOST_FIXTURE_TEST_SUITE(HTTPRequestWithPostContentBufferCreated_S, HTTPRequestWithPostContentBufferCreated_F)

// ???
BOOST_AUTO_TEST_CASE(checkCreatePostContentBufferAgainReturnsPointer) {
	BOOST_CHECK(createPostContentBuffer() != NULL);
}

BOOST_AUTO_TEST_CASE(checkGetPostContentReturnsPointer) {
	BOOST_CHECK(getPostContent() != NULL);
}

BOOST_AUTO_TEST_CASE(checkGetPostContentReturnsWhatWasWrittenToBuffer) {
	char buf[] = {0, 1, 2, 3, 127, 0, -1, -2, -3, -128};
	BOOST_CHECK_EQUAL(sizeof(buf), m_len);
	memcpy(m_post_buffer, buf, m_len);
	BOOST_CHECK(memcmp(buf, getPostContent(), m_len) == 0);
}

BOOST_AUTO_TEST_SUITE_END()

static const char TEXT_STRING_1[] = "0123456789";
static const char TEXT_STRING_2[] = "9876543210";
static const char TEXT_STRING_3[] = "0123456789abcde";

class HTTPRequestWithTextOnlyPostContent_F : public HTTPRequest {
public:
	HTTPRequestWithTextOnlyPostContent_F() {
		m_len = strlen(TEXT_STRING_1);
		setContentLength(m_len);
		m_post_buffer = createPostContentBuffer();
		memcpy(m_post_buffer, TEXT_STRING_1, m_len);
	}
	~HTTPRequestWithTextOnlyPostContent_F() {
	}

	int m_len;
	char* m_post_buffer;
};

BOOST_FIXTURE_TEST_SUITE(HTTPRequestWithTextOnlyPostContent_S, HTTPRequestWithTextOnlyPostContent_F)

BOOST_AUTO_TEST_CASE(checkGetPostContentReturnsPointer) {
	BOOST_CHECK(getPostContent() != NULL);
}

BOOST_AUTO_TEST_CASE(checkGetPostContentReturnsWhatWasWrittenToBuffer) {
	BOOST_CHECK(memcmp(TEXT_STRING_1, getPostContent(), m_len) == 0);
}

BOOST_AUTO_TEST_CASE(checkGetPostContentAfterChangingPostContent) {
	BOOST_CHECK_EQUAL(strlen(TEXT_STRING_2), m_len);
	memcpy(m_post_buffer, TEXT_STRING_2, m_len);
	BOOST_CHECK(memcmp(TEXT_STRING_2, getPostContent(), m_len) == 0);
}

BOOST_AUTO_TEST_CASE(checkGetPostContentAfterChangingSizeAndPostContent) {
	m_len = strlen(TEXT_STRING_3);
	setContentLength(m_len);
	m_post_buffer = createPostContentBuffer();
	memcpy(m_post_buffer, TEXT_STRING_3, m_len);
	BOOST_CHECK(memcmp(TEXT_STRING_3, getPostContent(), m_len) == 0);
}

// This is just for convenience for text-only post content.
// Strictly speaking, getPostContent() guarantees nothing beyond the buffer.
BOOST_AUTO_TEST_CASE(checkGetPostContentReturnsZeroTerminatedBuffer) {
	// This crashes due to bug in Boost.Test if m_post_buffer[m_len] is negative, so use workaround.
	//BOOST_CHECK_EQUAL(m_post_buffer[m_len], static_cast<char>(0));

	// Illustration of bug:
	//char c1 = -2;
	//char c2 = -2;
	//BOOST_CHECK_EQUAL(c1, c2);

	BOOST_CHECK_EQUAL(static_cast<int>(m_post_buffer[m_len]), static_cast<int>(0));
}

// See comments for checkGetPostContentReturnsZeroTerminatedBuffer.
BOOST_AUTO_TEST_CASE(checkPostContentPointerUsableAsString) {
	const std::string s1 = TEXT_STRING_1;
	std::string s2 = getPostContent();
	BOOST_CHECK_EQUAL(s1, s2);
}

BOOST_AUTO_TEST_SUITE_END()
