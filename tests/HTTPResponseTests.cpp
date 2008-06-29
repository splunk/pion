// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/PionConfig.hpp>
#include <pion/net/HTTPResponse.hpp>
#include <boost/test/unit_test.hpp>

using namespace pion;
using namespace pion::net;

class NewHTTPResponse_F : public HTTPResponse {
public:
	NewHTTPResponse_F() {
	}
	~NewHTTPResponse_F() {
	}
};

BOOST_FIXTURE_TEST_SUITE(NewHTTPResponse_S, NewHTTPResponse_F)

BOOST_AUTO_TEST_CASE(checkClear) {
	prepareHeadersForSend(true, false);
	BOOST_CHECK(!getHeaders().empty());
	clear();
	BOOST_CHECK(getHeaders().empty());
}

BOOST_AUTO_TEST_CASE(checkStatusCodeAccessors) {
	setStatusCode(HTTPTypes::RESPONSE_CODE_NOT_FOUND);
	BOOST_CHECK_EQUAL(getStatusCode(), HTTPTypes::RESPONSE_CODE_NOT_FOUND);
	setStatusCode(HTTPTypes::RESPONSE_CODE_CREATED);
	BOOST_CHECK_EQUAL(getStatusCode(), HTTPTypes::RESPONSE_CODE_CREATED);
}

BOOST_AUTO_TEST_CASE(checkStatusMessageAccessors) {
	setStatusMessage(HTTPTypes::RESPONSE_MESSAGE_NOT_FOUND);
	BOOST_CHECK_EQUAL(getStatusMessage(), HTTPTypes::RESPONSE_MESSAGE_NOT_FOUND);
	setStatusMessage(HTTPTypes::RESPONSE_MESSAGE_CREATED);
	BOOST_CHECK_EQUAL(getStatusMessage(), HTTPTypes::RESPONSE_MESSAGE_CREATED);
}

BOOST_AUTO_TEST_CASE(checkSetLastModified) {
	setLastModified(0);
	BOOST_CHECK_EQUAL(getHeader(HEADER_LAST_MODIFIED), get_date_string(0));
	setLastModified(100000000);
	BOOST_CHECK_EQUAL(getHeader(HEADER_LAST_MODIFIED), get_date_string(100000000));
	setLastModified(1000000000);
	BOOST_CHECK_EQUAL(getHeader(HEADER_LAST_MODIFIED), get_date_string(1000000000));
}

BOOST_AUTO_TEST_SUITE_END()
