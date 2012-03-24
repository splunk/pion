// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007-2011 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/PionConfig.hpp>
#include <pion/PionAlgorithms.hpp>
#include <boost/test/unit_test.hpp>

using namespace pion;


BOOST_AUTO_TEST_CASE(testURLEncoding) {
	BOOST_CHECK_EQUAL(algo::url_encode("hello world"), "hello%20world");
	BOOST_CHECK_EQUAL(algo::url_encode("He said, \"Hello, World!\""),
					  "He%20said%2C%20%22Hello%2C%20World!%22");
}

BOOST_AUTO_TEST_CASE(testURLEncodingOfStringWithNegativeCharacter) {
	std::string s = "abcde";
	s[0] = -30;
	BOOST_CHECK_EQUAL(algo::url_encode(s), "%E2bcde");
}

BOOST_AUTO_TEST_CASE(testBase64Routines) {
	std::string original;
	std::string original_base64;

	std::string encoded;
	std::string decoded;

	original = "mike:123456";
	original_base64 = "bWlrZToxMjM0NTY=";

	BOOST_CHECK(algo::base64_encode(original,encoded));
	BOOST_CHECK(encoded == original_base64);
	BOOST_CHECK(algo::base64_decode(encoded,decoded));
	BOOST_CHECK(decoded == original);

	original = "mike:12345";
	BOOST_CHECK(algo::base64_encode(original,encoded));
	BOOST_CHECK(algo::base64_decode(encoded,decoded));
	BOOST_CHECK(decoded == original);

	original = "mike:1234";
	BOOST_CHECK(algo::base64_encode(original,encoded));
	BOOST_CHECK(algo::base64_decode(encoded,decoded));
	BOOST_CHECK(decoded == original);

	original = "mike:123";
	BOOST_CHECK(algo::base64_encode(original,encoded));
	BOOST_CHECK(algo::base64_decode(encoded,decoded));
	BOOST_CHECK(decoded == original);

	char *ptr = "mike\0123\0\0";
	original.assign(ptr, 10);
	BOOST_CHECK(algo::base64_encode(original,encoded));
	BOOST_CHECK(algo::base64_decode(encoded,decoded));
	BOOST_CHECK_EQUAL(decoded.size(), 10U);
	BOOST_CHECK_EQUAL(memcmp(decoded.c_str(), ptr, 10), 0);
}
