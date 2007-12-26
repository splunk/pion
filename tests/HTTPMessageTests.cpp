#include <pion/PionConfig.hpp>
#include <pion/net/HTTPMessage.hpp>
#include <pion/net/HTTPRequest.hpp>
#include <pion/net/HTTPResponse.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>

using namespace pion::net;

// Based on BOOST_AUTO_TEST_CASE_TEMPLATE (in unit_test_suite.hpp)
#define BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(test_name, TL)	\
template<typename type_name>									\
struct test_name : public type_name								\
{ void test_method(); };										\
																\
struct BOOST_AUTO_TC_INVOKER( test_name ) {						\
	template<typename TestType>									\
	static void run( boost::type<TestType>* = 0 )				\
	{															\
		test_name<TestType> t;									\
		t.test_method();										\
	}															\
};																\
																\
BOOST_AUTO_TC_REGISTRAR( test_name )(							\
	boost::unit_test::ut_detail::template_test_case_gen<		\
	BOOST_AUTO_TC_INVOKER( test_name ),TL >(					\
		BOOST_STRINGIZE( test_name ) ) );						\
																\
template<typename type_name>									\
void test_name<type_name>::test_method()

#define FIXTURE_TYPE_LIST(F) boost::mpl::list<F<HTTPRequest>, F<HTTPResponse> >

template<typename ConcreteMessageType>
class NewHTTPMessage_F : public ConcreteMessageType {
public:
	NewHTTPMessage_F() {
	}
	~NewHTTPMessage_F() {
	}
};

BOOST_AUTO_TEST_SUITE(NewHTTPMessage_S)

typedef FIXTURE_TYPE_LIST(NewHTTPMessage_F) fixture_types;

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentLengthReturnsZero, fixture_types) {
	BOOST_CHECK_EQUAL(getContentLength(), static_cast<size_t>(0));
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkSetContentLengthDoesntThrow, fixture_types) {
	BOOST_CHECK_NO_THROW(setContentLength(10));
	BOOST_CHECK_NO_THROW(setContentLength(0));
}

// Is this what we want?  Note that the length is zero here.
BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkCreateContentBufferReturnsPointer, fixture_types) {
	BOOST_CHECK(createContentBuffer() != NULL);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentReturnsNull, fixture_types) {
	BOOST_CHECK(getContent() == NULL);
}

// Is this what we want?  Note that the length is zero here.
BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentReturnsPointerAfterCreatingContentBuffer, fixture_types) {
	createContentBuffer();
	BOOST_CHECK(getContent() != NULL);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkIsValidReturnsFalse, fixture_types) {
	BOOST_CHECK(!isValid());
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(testChunksSupportedAccessors, fixture_types) {
	BOOST_CHECK(!getChunksSupported());
	setChunksSupported(true);
	BOOST_CHECK(getChunksSupported());
	setChunksSupported(false);
	BOOST_CHECK(!getChunksSupported());
}

BOOST_AUTO_TEST_SUITE_END()

template<typename ConcreteMessageType>
class HTTPMessageWithContentLengthSet_F : public ConcreteMessageType {
public:
	HTTPMessageWithContentLengthSet_F() {
		setContentLength(20);
	}
	~HTTPMessageWithContentLengthSet_F() {
	}
};

BOOST_AUTO_TEST_SUITE(HTTPMessageWithContentLengthSet_S)

typedef FIXTURE_TYPE_LIST(HTTPMessageWithContentLengthSet_F) fixture_types;

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentLengthReturnsCorrectLength, fixture_types) {
	BOOST_CHECK_EQUAL(getContentLength(), static_cast<size_t>(20));
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentLengthAfterSettingLengthAgain, fixture_types) {
	setContentLength(30);
	BOOST_CHECK_EQUAL(getContentLength(), static_cast<size_t>(30));
	setContentLength(0);
	BOOST_CHECK_EQUAL(getContentLength(), static_cast<size_t>(0));
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkCreateContentBufferReturnsPointer, fixture_types) {
	BOOST_CHECK(createContentBuffer() != NULL);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentReturnsNull, fixture_types) {
	BOOST_CHECK(getContent() == NULL);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkIsValidReturnsFalse, fixture_types) {
	BOOST_CHECK(!isValid());
}

BOOST_AUTO_TEST_SUITE_END()

template<typename ConcreteMessageType>
class HTTPMessageWithContentBufferCreated_F : public ConcreteMessageType {
public:
	HTTPMessageWithContentBufferCreated_F() {
		m_len = 10;
		setContentLength(m_len);
		m_content_buffer = createContentBuffer();
	}
	~HTTPMessageWithContentBufferCreated_F() {
	}

	int m_len;
	char* m_content_buffer;
};

BOOST_AUTO_TEST_SUITE(HTTPMessageWithContentBufferCreated_S)

typedef FIXTURE_TYPE_LIST(HTTPMessageWithContentBufferCreated_F) fixture_types;

// ???
BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkCreateContentBufferAgainReturnsPointer, fixture_types) {
	BOOST_CHECK(createContentBuffer() != NULL);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentReturnsPointer, fixture_types) {
	BOOST_CHECK(getContent() != NULL);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentReturnsWhatWasWrittenToBuffer, fixture_types) {
	char buf[] = {0, 1, 2, 3, 127, 0, -1, -2, -3, -128};
	BOOST_CHECK_EQUAL(sizeof(buf), static_cast<unsigned int>(m_len));
	memcpy(m_content_buffer, buf, m_len);
	BOOST_CHECK(memcmp(buf, getContent(), m_len) == 0);
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
		setContentLength(m_len);
		m_content_buffer = createContentBuffer();
		memcpy(m_content_buffer, TEXT_STRING_1, m_len);
	}
	~HTTPMessageWithTextOnlyContent_F() {
	}

	size_t m_len;
	char* m_content_buffer;
};

BOOST_AUTO_TEST_SUITE(HTTPMessageWithTextOnlyContent_S)

typedef FIXTURE_TYPE_LIST(HTTPMessageWithTextOnlyContent_F) fixture_types;

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentReturnsPointer, fixture_types) {
	BOOST_CHECK(getContent() != NULL);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentReturnsWhatWasWrittenToBuffer, fixture_types) {
	BOOST_CHECK(memcmp(TEXT_STRING_1, getContent(), m_len) == 0);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentAfterChangingContent, fixture_types) {
	BOOST_CHECK_EQUAL(strlen(TEXT_STRING_2), static_cast<unsigned int>(m_len));
	memcpy(m_content_buffer, TEXT_STRING_2, m_len);
	BOOST_CHECK(memcmp(TEXT_STRING_2, getContent(), m_len) == 0);
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentAfterChangingSizeAndContent, fixture_types) {
	m_len = strlen(TEXT_STRING_3);
	setContentLength(m_len);
	m_content_buffer = createContentBuffer();
	memcpy(m_content_buffer, TEXT_STRING_3, m_len);
	BOOST_CHECK(memcmp(TEXT_STRING_3, getContent(), m_len) == 0);
}

// This is just for convenience for text-only post content.
// Strictly speaking, getContent() guarantees nothing beyond the buffer.
BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetContentReturnsZeroTerminatedBuffer, fixture_types) {
	// This crashes due to bug in Boost.Test if m_content_buffer[m_len] is negative, so use workaround.
	//BOOST_CHECK_EQUAL(m_content_buffer[m_len], static_cast<char>(0));

	// Illustration of bug:
	//char c1 = -2;
	//char c2 = -2;
	//BOOST_CHECK_EQUAL(c1, c2);

	BOOST_CHECK_EQUAL(static_cast<int>(m_content_buffer[m_len]), static_cast<int>(0));
}

// See comments for checkGetContentReturnsZeroTerminatedBuffer.
BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkContentPointerUsableAsString, fixture_types) {
	const std::string s1 = TEXT_STRING_1;
	std::string s2 = getContent();
	BOOST_CHECK_EQUAL(s1, s2);
}

BOOST_AUTO_TEST_SUITE_END()
