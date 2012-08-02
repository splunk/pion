// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/config.hpp>
#include <pion/http/response.hpp>
#include <boost/test/unit_test.hpp>

using namespace pion;

class NewHTTPResponse_F : public http::response {
public:
    NewHTTPResponse_F() {
    }
    ~NewHTTPResponse_F() {
    }
};

BOOST_FIXTURE_TEST_SUITE(NewHTTPResponse_S, NewHTTPResponse_F)

BOOST_AUTO_TEST_CASE(checkClear) {
    prepareHeadersForSend(true, false);
    BOOST_CHECK(!get_headers().empty());
    clear();
    BOOST_CHECK(get_headers().empty());
}

BOOST_AUTO_TEST_CASE(checkStatusCodeAccessors) {
    setStatusCode(http::types::RESPONSE_CODE_NOT_FOUND);
    BOOST_CHECK_EQUAL(getStatusCode(), http::types::RESPONSE_CODE_NOT_FOUND);
    setStatusCode(http::types::RESPONSE_CODE_CREATED);
    BOOST_CHECK_EQUAL(getStatusCode(), http::types::RESPONSE_CODE_CREATED);
}

BOOST_AUTO_TEST_CASE(checkStatusMessageAccessors) {
    setStatusMessage(http::types::RESPONSE_MESSAGE_NOT_FOUND);
    BOOST_CHECK_EQUAL(getStatusMessage(), http::types::RESPONSE_MESSAGE_NOT_FOUND);
    setStatusMessage(http::types::RESPONSE_MESSAGE_CREATED);
    BOOST_CHECK_EQUAL(getStatusMessage(), http::types::RESPONSE_MESSAGE_CREATED);
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
