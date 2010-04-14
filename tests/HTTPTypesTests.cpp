// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/PionConfig.hpp>
#include <pion/net/HTTPTypes.hpp>
#include <pion/PionUnitTestDefs.hpp>
#include <boost/test/unit_test.hpp>

using namespace pion;
using namespace pion::net;

BOOST_FIXTURE_TEST_SUITE(HTTPTypes_S, HTTPTypes)

BOOST_AUTO_TEST_CASE(testCaseInsensitiveLess) {
	BOOST_CHECK(!CaseInsensitiveLess()("a", "A"));
	BOOST_CHECK(!CaseInsensitiveLess()("A", "a"));
	BOOST_CHECK(!CaseInsensitiveLess()("aB", "Ab"));
	BOOST_CHECK(!CaseInsensitiveLess()("Ab", "aB"));
	BOOST_CHECK(CaseInsensitiveLess()("aA", "ab"));
	BOOST_CHECK(!CaseInsensitiveLess()("ab", "aA"));
	BOOST_CHECK(CaseInsensitiveLess()("AB", "abc"));
	BOOST_CHECK(!CaseInsensitiveLess()("abc", "AB"));
	BOOST_CHECK(CaseInsensitiveLess()("ac", "b"));
	BOOST_CHECK(!CaseInsensitiveLess()("b", "ac"));
}

BOOST_AUTO_TEST_CASE(testCaseInsensitiveEqual) {
	BOOST_CHECK(CaseInsensitiveEqual()("a", "A"));
	BOOST_CHECK(CaseInsensitiveEqual()("A", "a"));
	BOOST_CHECK(CaseInsensitiveEqual()("aB", "Ab"));
	BOOST_CHECK(CaseInsensitiveEqual()("Ab", "aB"));
	BOOST_CHECK(!CaseInsensitiveEqual()("AB", "ABC"));
	BOOST_CHECK(!CaseInsensitiveEqual()("abc", "ab"));
}

BOOST_AUTO_TEST_CASE(testURLEncoding) {
	BOOST_CHECK_EQUAL(url_encode("hello world"), "hello%20world");
	BOOST_CHECK_EQUAL(url_encode("He said, \"Hello, World!\""),
					  "He%20said%2C%20%22Hello%2C%20World!%22");
}

BOOST_AUTO_TEST_CASE(testURLEncodingOfStringWithNegativeCharacter) {
	std::string s = "abcde";
	s[0] = -30;
	BOOST_CHECK_EQUAL(url_encode(s), "%E2bcde");
}

BOOST_AUTO_TEST_CASE(testBase64Routines) {
	std::string original;
	std::string original_base64;

	std::string encoded;
	std::string decoded;

	original = "mike:123456";
	original_base64 = "bWlrZToxMjM0NTY=";

	BOOST_CHECK(base64_encode(original,encoded));
	BOOST_CHECK(encoded == original_base64);
	BOOST_CHECK(base64_decode(encoded,decoded));
	BOOST_CHECK(decoded == original);

	original = "mike:12345";
	BOOST_CHECK(base64_encode(original,encoded));
	BOOST_CHECK(base64_decode(encoded,decoded));
	BOOST_CHECK(decoded == original);

	original = "mike:1234";
	BOOST_CHECK(base64_encode(original,encoded));
	BOOST_CHECK(base64_decode(encoded,decoded));
	BOOST_CHECK(decoded == original);

	original = "mike:123";
	BOOST_CHECK(base64_encode(original,encoded));
	BOOST_CHECK(base64_decode(encoded,decoded));
	BOOST_CHECK(decoded == original);

	char *ptr = "mike\0123\0\0";
	original.assign(ptr, ptr+10);
	BOOST_CHECK(base64_encode(original,encoded));
	BOOST_CHECK(base64_decode(encoded,decoded));
	BOOST_CHECK_EQUAL(decoded.size(), 10U);
	BOOST_CHECK_EQUAL(memcmp(decoded.c_str(), ptr, 10), 0);
}

BOOST_AUTO_TEST_SUITE_END()
