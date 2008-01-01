// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
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

BOOST_AUTO_TEST_CASE(checkGetMethodReturnsGET) {
	BOOST_CHECK_EQUAL(getMethod(), "GET");
}

BOOST_AUTO_TEST_CASE(checkGetMethodReturnsWhatSetMethodAssigns) {
	BOOST_CHECK_NO_THROW(setMethod("POST"));
	BOOST_CHECK_EQUAL(getMethod(), "POST");
}

BOOST_AUTO_TEST_CASE(checkGetMethodReturnsWhateverSetMethodAssigns) {
	BOOST_CHECK_NO_THROW(setMethod("BLAH_BLAH_BLAH"));
	BOOST_CHECK_EQUAL(getMethod(), "BLAH_BLAH_BLAH");
}

BOOST_AUTO_TEST_SUITE_END()
