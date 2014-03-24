// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/config.hpp>
#include <pion/http/request.hpp>
#include <boost/test/unit_test.hpp>

using namespace pion;

class NewHTTPRequest_F : public http::request {
public:
    NewHTTPRequest_F() {
    }
    ~NewHTTPRequest_F() {
    }
};

BOOST_FIXTURE_TEST_SUITE(NewHTTPRequest_S, NewHTTPRequest_F)

BOOST_AUTO_TEST_CASE(checkSetMethodWithValidMethodDoesntThrow) {
    BOOST_CHECK_NO_THROW(set_method("GET"));
}

// Is this what we want?
BOOST_AUTO_TEST_CASE(checkSetMethodWithInvalidMethodDoesntThrow) {
    BOOST_CHECK_NO_THROW(set_method("NOT_A_VALID_METHOD"));
}

BOOST_AUTO_TEST_CASE(checkGetMethodReturnsGET) {
    BOOST_CHECK_EQUAL(get_method(), "GET");
}

BOOST_AUTO_TEST_CASE(checkGetMethodReturnsWhatSetMethodAssigns) {
    BOOST_CHECK_NO_THROW(set_method("POST"));
    BOOST_CHECK_EQUAL(get_method(), "POST");
}

BOOST_AUTO_TEST_CASE(checkGetMethodReturnsWhateverSetMethodAssigns) {
    BOOST_CHECK_NO_THROW(set_method("BLAH_BLAH_BLAH"));
    BOOST_CHECK_EQUAL(get_method(), "BLAH_BLAH_BLAH");
}

BOOST_AUTO_TEST_SUITE_END()
