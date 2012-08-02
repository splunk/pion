// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/config.hpp>
#include <pion/test/unit_test.hpp>
#include <pion/http/types.hpp>
#include <boost/test/unit_test.hpp>

using namespace pion;

BOOST_FIXTURE_TEST_SUITE(types_S, http::types)

BOOST_AUTO_TEST_CASE(ihashTests) {
    std::string val1 = "Content-Type";
    std::string val2 = "Content-type";
    ihash hasher;
    BOOST_CHECK_EQUAL(hasher(val1), hasher(val2));
}

BOOST_AUTO_TEST_CASE(testCaseInsensitiveHeaders) {
    ihash_multimap h;
    std::string key1("Content-Length");
    std::string key2("Content-length");
    std::string value("123");
    
    h.insert(std::make_pair(key1, value));
    ihash_multimap::const_iterator it = h.find(key2);
    
    BOOST_REQUIRE(it != h.end());
    BOOST_CHECK_EQUAL(it->second, value);
}

BOOST_AUTO_TEST_CASE(testMultipleHeaderValues) {
    ihash_multimap h;
    std::string key1("Content-Length");
    std::string key2("Content-length");
    std::string key3("content-length");
    std::string value1("123");
    std::string value2("456");

    h.insert(std::make_pair(key1, value1));
    h.insert(std::make_pair(key2, value2));
    
    std::pair<ihash_multimap::const_iterator,ihash_multimap::const_iterator> hp = h.equal_range(key3);
    
    ihash_multimap::const_iterator it = hp.first;
    BOOST_REQUIRE(it != h.end());
    
    BOOST_CHECK(it->second == value1 || it->second == value2);
    
    if (it->second == value1) {
        ++it;
        BOOST_REQUIRE(it != h.end());
        BOOST_CHECK_EQUAL(it->second, value2);
    } else {
        ++it;
        BOOST_REQUIRE(it != h.end());
        BOOST_CHECK_EQUAL(it->second, value1);
    }
    
    ++it;
    BOOST_CHECK(it == h.end());
}

BOOST_AUTO_TEST_SUITE_END()
