// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/config.hpp>
#include <pion/unit_test_defs.hpp>
#include <pion/http/types.hpp>
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

BOOST_AUTO_TEST_SUITE_END()
