// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
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

BOOST_AUTO_TEST_CASE(checkXmlEncodeWithAlphanumericString) {
    BOOST_CHECK_EQUAL("Freedom7", algorithm::xml_encode("Freedom7"));
}

BOOST_AUTO_TEST_CASE(checkXmlEncodeWithAmpersand) {
    BOOST_CHECK_EQUAL("A&amp;P", algorithm::xml_encode("A&P"));
}

BOOST_AUTO_TEST_CASE(checkXmlEncodeWithVariousSpecialXmlCharacters) {
    BOOST_CHECK_EQUAL("&quot;1&quot; &lt; &quot;2&quot; &amp;&amp; &apos;b&apos; &gt; &apos;a&apos;", algorithm::xml_encode("\"1\" < \"2\" && 'b' > 'a'"));
}

// UTF-8 replacement character
const std::string RC = "\xEF\xBF\xBD";

BOOST_AUTO_TEST_CASE(checkXmlEncodeWithControlCharacters) {
    char cc_array_1[] = "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F";
    char cc_array_2[] = "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F";
    std::string cc_str_1(cc_array_1, 16);
    std::string cc_str_2(cc_array_2, 16);
    std::string expected_output_1 = RC + RC + RC + RC + RC + RC + RC + RC + RC + "\x09\x0A" + RC + RC + "\x0D" + RC + RC;
    std::string expected_output_2 = RC + RC + RC + RC + RC + RC + RC + RC + RC + RC + RC + RC + RC + RC + RC + RC;

    BOOST_CHECK_EQUAL(expected_output_1, algorithm::xml_encode(cc_str_1));
    BOOST_CHECK_EQUAL(expected_output_2, algorithm::xml_encode(cc_str_2));
}

BOOST_AUTO_TEST_CASE(checkXmlEncodeWithValidUtf8TwoByteSequences) {
    const char UTF8_ENCODED_TEST_CHAR_ARRAY[] = {
        (char)0xCE, (char)0xB1,             // UTF-8 encoding of U+03B1 (GREEK SMALL LETTER ALPHA)
        0x3D,                               // '='
        0x31,                               // '1'
        0x20,                               // space
        (char)0xCE, (char)0xB2,             // UTF-8 encoding of U+03B2 (GREEK SMALL LETTER BETA)
        0x3D,                               // '='
        0x32};                              // '2'
    const std::string UTF8_ENCODED_TEST_STRING(UTF8_ENCODED_TEST_CHAR_ARRAY, sizeof(UTF8_ENCODED_TEST_CHAR_ARRAY));
    BOOST_CHECK_EQUAL(UTF8_ENCODED_TEST_STRING, algorithm::xml_encode(UTF8_ENCODED_TEST_STRING));
}

BOOST_AUTO_TEST_CASE(checkXmlEncodeWithValidUtf8ThreeByteSequences) {
    const char UTF8_ENCODED_TEST_CHAR_ARRAY[] = {
        (char)0xE2, (char)0x82, (char)0xA4,     // UTF-8 encoding of U+20A4 (LIRA SIGN)
        0x32,                                   // '2'
        0x3D,                                   // '='
        (char)0xE2, (char)0x82, (char)0xA8,     // UTF-8 encoding of U+20A8 (RUPEE SIGN)
        0x32};                                  // '3'
    const std::string UTF8_ENCODED_TEST_STRING(UTF8_ENCODED_TEST_CHAR_ARRAY, sizeof(UTF8_ENCODED_TEST_CHAR_ARRAY));
    BOOST_CHECK_EQUAL(UTF8_ENCODED_TEST_STRING, algorithm::xml_encode(UTF8_ENCODED_TEST_STRING));
}

BOOST_AUTO_TEST_CASE(checkXmlEncodeWithValidUtf8FourByteSequences) {
    char UTF8_ENCODED_TEST_CHAR_ARRAY[] = {
        (char)0xF0, (char)0x90, (char)0x82, (char)0x88,     // UTF-8 encoding of U+10088 (LINEAR B IDEOGRAM B107F SHE-GOAT)
        (char)0xE2, (char)0x82, (char)0xA8,                 // UTF-8 encoding of U+2260 (NOT EQUAL TO)
        (char)0xF0, (char)0x90, (char)0x82, (char)0x89};    // UTF-8 encoding of U+10089 (LINEAR B IDEOGRAM B107M HE-GOAT)
    const std::string UTF8_ENCODED_TEST_STRING(UTF8_ENCODED_TEST_CHAR_ARRAY, sizeof(UTF8_ENCODED_TEST_CHAR_ARRAY));
    BOOST_CHECK_EQUAL(UTF8_ENCODED_TEST_STRING, algorithm::xml_encode(UTF8_ENCODED_TEST_STRING));
}

// Any isolated high byte (i.e. > x7F) is invalid, but they are invalid for a variety of reasons.
BOOST_AUTO_TEST_CASE(checkXmlEncodeWithInvalidSingleHighByte) {
    // These are invalid because the second byte is > x7F and is not allowed as the first byte of a UTF-8 multi-byte sequence.
    BOOST_CHECK_EQUAL("=" + RC + "=", algorithm::xml_encode("=\x80="));
    BOOST_CHECK_EQUAL("=" + RC + "=", algorithm::xml_encode("=\xBF="));
    BOOST_CHECK_EQUAL("=" + RC + "=", algorithm::xml_encode("=\xC0="));
    BOOST_CHECK_EQUAL("=" + RC + "=", algorithm::xml_encode("=\xC1="));
    BOOST_CHECK_EQUAL("=" + RC + "=", algorithm::xml_encode("=\xF5="));
    BOOST_CHECK_EQUAL("=" + RC + "=", algorithm::xml_encode("=\xFF="));

    // These are invalid because the second byte is the first byte of a UTF-8 2-byte sequence, but isn't followed by a valid second byte.
    BOOST_CHECK_EQUAL("=" + RC + "=", algorithm::xml_encode("=\xC2="));
    BOOST_CHECK_EQUAL("=" + RC + "=", algorithm::xml_encode("=\xDF="));

    // These are invalid because the second byte is the first byte of a UTF-8 3-byte sequence, but isn't followed by a valid second byte.
    BOOST_CHECK_EQUAL("=" + RC + "=", algorithm::xml_encode("=\xE0="));
    BOOST_CHECK_EQUAL("=" + RC + "=", algorithm::xml_encode("=\xEF="));

    // These are invalid because the second byte is the first byte of a UTF-8 4-byte sequence, but isn't followed by a valid second byte.
    BOOST_CHECK_EQUAL("=" + RC + "=", algorithm::xml_encode("=\xF0="));
    BOOST_CHECK_EQUAL("=" + RC + "=", algorithm::xml_encode("=\xF4="));
}

BOOST_AUTO_TEST_CASE(checkXmlEncodeWithInvalidTwoHighByteSequence) {
    // These are invalid because the second byte is the first byte of a UTF-8 2-byte sequence, but isn't followed by a valid second byte.
    BOOST_CHECK_EQUAL("=" + RC + RC + "=", algorithm::xml_encode("=\xC2\xC0="));
    BOOST_CHECK_EQUAL("=" + RC + RC + "=", algorithm::xml_encode("=\xDF\xFF="));

    // These are invalid because bytes 2 & 3 are the first and second bytes of a UTF-8 3-byte sequence, but aren't followed by a valid third byte.
    BOOST_CHECK_EQUAL("=" + RC + RC + "=", algorithm::xml_encode("=\xE0\x80="));
    BOOST_CHECK_EQUAL("=" + RC + RC + "=", algorithm::xml_encode("=\xEF\xBF="));

    // These are invalid because bytes 2 & 3 are the first and second bytes of a UTF-8 4-byte sequence, but aren't followed by a valid third byte.
    BOOST_CHECK_EQUAL("=" + RC + RC + "=", algorithm::xml_encode("=\xF0\x80="));
    BOOST_CHECK_EQUAL("=" + RC + RC + "=", algorithm::xml_encode("=\xF4\xBF="));
}

BOOST_AUTO_TEST_CASE(checkXmlEncodeWithInvalidThreeHighByteSequence) {
    // These are invalid because bytes 2 & 3 are the first and second bytes of a UTF-8 3-byte sequence, but aren't followed by a valid third byte.
    BOOST_CHECK_EQUAL("=" + RC + RC + RC + "=", algorithm::xml_encode("=\xE0\x80\xC0="));
    BOOST_CHECK_EQUAL("=" + RC + RC + RC + "=", algorithm::xml_encode("=\xEF\xBF\xFF="));

    // These are invalid because bytes 2 & 3 are the first and second bytes of a UTF-8 4-byte sequence, but aren't followed by a valid third byte.
    BOOST_CHECK_EQUAL("=" + RC + RC + RC + "=", algorithm::xml_encode("=\xF0\x80\xC0="));
    BOOST_CHECK_EQUAL("=" + RC + RC + RC + "=", algorithm::xml_encode("=\xF4\xBF\xFF="));
}

BOOST_AUTO_TEST_CASE(checkXmlEncodeWithInvalidFourHighByteSequence) {
    // These are invalid because bytes 2-4  are the first, second and third bytes of a UTF-8 4-byte sequence, but aren't followed by a valid fourth byte.
    BOOST_CHECK_EQUAL("=" + RC + RC + RC + RC + "=", algorithm::xml_encode("=\xF0\x80\x80\xC0="));
    BOOST_CHECK_EQUAL("=" + RC + RC + RC + RC + "=", algorithm::xml_encode("=\xF4\xBF\xBF\xFF="));
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
    
    algorithm::from_double(buf, 1.0123456789012345e-300);
    BOOST_CHECK_EQUAL(algorithm::to_double(buf), 1.0123456789012345e-300);
    
    algorithm::from_double(buf, 1.0123456789012345e+300);
    BOOST_CHECK_EQUAL(algorithm::to_double(buf), 1.0123456789012345e+300);
    
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

    algorithm::from_long_double(buf, 1.0123456789012345e-300);
    BOOST_CHECK_EQUAL(algorithm::to_long_double(buf), 1.0123456789012345e-300);

    algorithm::from_long_double(buf, 1.0123456789012345e+300);
    BOOST_CHECK_EQUAL(algorithm::to_long_double(buf), 1.0123456789012345e+300);
}
