// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/config.hpp>
#include <pion/algorithm.hpp>
#include <boost/test/unit_test.hpp>

using namespace pion;


BOOST_AUTO_TEST_CASE(testURLEncoding) {
    BOOST_CHECK_EQUAL(algorithm::url_encode("hello world"), "hello%20world");
    BOOST_CHECK_EQUAL(algorithm::url_encode("He said, \"Hello, World!\""),
                      "He%20said%2C%20%22Hello%2C%20World!%22");
}

BOOST_AUTO_TEST_CASE(testURLEncodingOfStringWithNegativeCharacter) {
    std::string s = "abcde";
    s[0] = -30;
    BOOST_CHECK_EQUAL(algorithm::url_encode(s), "%E2bcde");
}

BOOST_AUTO_TEST_CASE(testBase64Routines) {
    std::string original;
    std::string original_base64;

    std::string encoded;
    std::string decoded;

    original = "mike:123456";
    original_base64 = "bWlrZToxMjM0NTY=";

    BOOST_CHECK(algorithm::base64_encode(original,encoded));
    BOOST_CHECK(encoded == original_base64);
    BOOST_CHECK(algorithm::base64_decode(encoded,decoded));
    BOOST_CHECK(decoded == original);

    original = "mike:12345";
    BOOST_CHECK(algorithm::base64_encode(original,encoded));
    BOOST_CHECK(algorithm::base64_decode(encoded,decoded));
    BOOST_CHECK(decoded == original);

    original = "mike:1234";
    BOOST_CHECK(algorithm::base64_encode(original,encoded));
    BOOST_CHECK(algorithm::base64_decode(encoded,decoded));
    BOOST_CHECK(decoded == original);

    original = "mike:123";
    BOOST_CHECK(algorithm::base64_encode(original,encoded));
    BOOST_CHECK(algorithm::base64_decode(encoded,decoded));
    BOOST_CHECK(decoded == original);

    const char *ptr = "mike\0123\0\0";
    original.assign(ptr, 10);
    BOOST_CHECK(algorithm::base64_encode(original,encoded));
    BOOST_CHECK(algorithm::base64_decode(encoded,decoded));
    BOOST_CHECK_EQUAL(decoded.size(), 10U);
    BOOST_CHECK_EQUAL(memcmp(decoded.c_str(), ptr, 10), 0);
}

BOOST_AUTO_TEST_CASE(testCharFromToIntRoutines) {
    char buf[8];
    
    algorithm::from_uint8(buf, 129U);
    BOOST_CHECK_EQUAL(buf[0], (char)0x81);
    BOOST_CHECK_EQUAL(algorithm::to_int8(buf), boost::int8_t(0x81));
    BOOST_CHECK_EQUAL(algorithm::to_uint8(buf), 129U);

    algorithm::from_uint16(buf, 32769U);
    BOOST_CHECK_EQUAL(buf[0], (char)0x80);
    BOOST_CHECK_EQUAL(buf[1], (char)0x01);
    BOOST_CHECK_EQUAL(algorithm::to_int16(buf), boost::int16_t(0x8001));
    BOOST_CHECK_EQUAL(algorithm::to_uint16(buf), 32769U);

    algorithm::from_uint24(buf, 9642497U);
    BOOST_CHECK_EQUAL(buf[0], (char)0x93);
    BOOST_CHECK_EQUAL(buf[1], (char)0x22);
    BOOST_CHECK_EQUAL(buf[2], (char)0x01);
    BOOST_CHECK_EQUAL(algorithm::to_int24(buf), boost::int32_t(0x932201));
    BOOST_CHECK_EQUAL(algorithm::to_uint24(buf), 9642497U);

    algorithm::from_uint32(buf, 2147680769UL);
    BOOST_CHECK_EQUAL(buf[0], (char)0x80);
    BOOST_CHECK_EQUAL(buf[1], (char)0x03);
    BOOST_CHECK_EQUAL(buf[2], (char)0x02);
    BOOST_CHECK_EQUAL(buf[3], (char)0x01);
    BOOST_CHECK_EQUAL(algorithm::to_int32(buf), boost::int32_t(0x80030201));
    BOOST_CHECK_EQUAL(algorithm::to_uint32(buf), 2147680769UL);

    algorithm::from_uint32(buf, 1427U);
    BOOST_CHECK_EQUAL(buf[0], (char)0x00);
    BOOST_CHECK_EQUAL(buf[1], (char)0x00);
    BOOST_CHECK_EQUAL(buf[2], (char)0x05);
    BOOST_CHECK_EQUAL(buf[3], (char)0x93);
    BOOST_CHECK_EQUAL(algorithm::to_int32(buf), boost::int32_t(0x00000593));
    BOOST_CHECK_EQUAL(algorithm::to_uint32(buf), 1427U);

    algorithm::from_uint64(buf, 9223378168241586176ULL);
    BOOST_CHECK_EQUAL(buf[0], (char)0x80);
    BOOST_CHECK_EQUAL(buf[1], (char)0x00);
    BOOST_CHECK_EQUAL(buf[2], (char)0x05);
    BOOST_CHECK_EQUAL(buf[3], (char)0x93);
    BOOST_CHECK_EQUAL(buf[4], (char)0x93);
    BOOST_CHECK_EQUAL(buf[5], (char)0x22);
    BOOST_CHECK_EQUAL(buf[6], (char)0x00);
    BOOST_CHECK_EQUAL(buf[7], (char)0x00);
    BOOST_CHECK_EQUAL(algorithm::to_int64(buf), boost::int64_t(0x8000059393220000ULL));
    BOOST_CHECK_EQUAL(algorithm::to_uint64(buf), 9223378168241586176ULL);
}

BOOST_AUTO_TEST_CASE(testCharFromToFloatRoutines) {
    char buf[16];
    
    algorithm::from_float(buf, 0.0);
    BOOST_CHECK_EQUAL(buf[0], (char)0x00);
    BOOST_CHECK_EQUAL(buf[1], (char)0x00);
    BOOST_CHECK_EQUAL(buf[2], (char)0x00);
    BOOST_CHECK_EQUAL(buf[3], (char)0x00);
    BOOST_CHECK_EQUAL(algorithm::to_float(buf), 0.0);
    
    algorithm::from_float(buf, -13021.0);
    BOOST_CHECK_EQUAL(buf[0], (char)0xc6);
    BOOST_CHECK_EQUAL(buf[1], (char)0x4b);
    BOOST_CHECK_EQUAL(buf[2], (char)0x74);
    BOOST_CHECK_EQUAL(buf[3], (char)0x00);
    BOOST_CHECK_EQUAL(algorithm::to_float(buf), -13021.0);
    
    algorithm::from_float(buf, 12.375);
    BOOST_CHECK_EQUAL(buf[0], (char)0x41);
    BOOST_CHECK_EQUAL(buf[1], (char)0x46);
    BOOST_CHECK_EQUAL(buf[2], (char)0x00);
    BOOST_CHECK_EQUAL(buf[3], (char)0x00);
    BOOST_CHECK_EQUAL(algorithm::to_float(buf), 12.375);

    algorithm::from_float(buf, 1);
    BOOST_CHECK_EQUAL(buf[0], (char)0x3f);
    BOOST_CHECK_EQUAL(buf[1], (char)0x80);
    BOOST_CHECK_EQUAL(buf[2], (char)0x00);
    BOOST_CHECK_EQUAL(buf[3], (char)0x00);
    BOOST_CHECK_EQUAL(algorithm::to_float(buf), 1.0);
    
    algorithm::from_float(buf, 0.25);
    BOOST_CHECK_EQUAL(buf[0], (char)0x3e);
    BOOST_CHECK_EQUAL(buf[1], (char)0x80);
    BOOST_CHECK_EQUAL(buf[2], (char)0x00);
    BOOST_CHECK_EQUAL(buf[3], (char)0x00);
    BOOST_CHECK_EQUAL(algorithm::to_float(buf), 0.25);
    
    algorithm::from_float(buf, 0.375);
    BOOST_CHECK_EQUAL(buf[0], (char)0x3e);
    BOOST_CHECK_EQUAL(buf[1], (char)0xc0);
    BOOST_CHECK_EQUAL(buf[2], (char)0x00);
    BOOST_CHECK_EQUAL(buf[3], (char)0x00);
    BOOST_CHECK_EQUAL(algorithm::to_float(buf), 0.375);
    
    algorithm::from_double(buf, 0);
    BOOST_CHECK_EQUAL(algorithm::to_double(buf), 0);

    algorithm::from_double(buf, -13021.0);
    BOOST_CHECK_EQUAL(algorithm::to_double(buf), -13021.0);
    
    algorithm::from_double(buf, 12.375);
    BOOST_CHECK_EQUAL(algorithm::to_double(buf), 12.375);
    
    algorithm::from_double(buf, 1);
    BOOST_CHECK_EQUAL(algorithm::to_double(buf), 1.0);
    
    algorithm::from_double(buf, 0.25);
    BOOST_CHECK_EQUAL(algorithm::to_double(buf), 0.25);
    
    algorithm::from_double(buf, 0.375);
    BOOST_CHECK_EQUAL(algorithm::to_double(buf), 0.375);
    
    algorithm::from_long_double(buf, 0);
    BOOST_CHECK_EQUAL(algorithm::to_long_double(buf), 0);
    
    algorithm::from_long_double(buf, -13021.0);
    BOOST_CHECK_EQUAL(algorithm::to_long_double(buf), -13021.0);
    
    algorithm::from_long_double(buf, 12.375);
    BOOST_CHECK_EQUAL(algorithm::to_long_double(buf), 12.375);
    
    algorithm::from_long_double(buf, 1);
    BOOST_CHECK_EQUAL(algorithm::to_long_double(buf), 1.0);
    
    algorithm::from_long_double(buf, 0.25);
    BOOST_CHECK_EQUAL(algorithm::to_long_double(buf), 0.25);

    algorithm::from_long_double(buf, 0.375);
    BOOST_CHECK_EQUAL(algorithm::to_long_double(buf), 0.375);
}
