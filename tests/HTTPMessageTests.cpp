#include <pion/PionConfig.hpp>
#include <pion/net/HTTPMessage.hpp>
#include <pion/net/HTTPRequest.hpp>
#include <pion/net/HTTPResponse.hpp>
#include <pion/PionUnitTestDefs.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>

using namespace pion::net;

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

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentReturnsNull) {
	BOOST_CHECK(F::getContent() == NULL);
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

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkCreateContentBufferReturnsPointer) {
	BOOST_CHECK(F::createContentBuffer() != NULL);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentReturnsNull) {
	BOOST_CHECK(F::getContent() == NULL);
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
