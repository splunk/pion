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
    prepare_headers_for_send(true, false);
    BOOST_CHECK(!get_headers().empty());
    clear();
    BOOST_CHECK(get_headers().empty());
}

BOOST_AUTO_TEST_CASE(checkStatusCodeAccessors) {
    set_status_code(http::types::RESPONSE_CODE_NOT_FOUND);
    BOOST_CHECK_EQUAL(get_status_code(), http::types::RESPONSE_CODE_NOT_FOUND);
    set_status_code(http::types::RESPONSE_CODE_CREATED);
    BOOST_CHECK_EQUAL(get_status_code(), http::types::RESPONSE_CODE_CREATED);
}

BOOST_AUTO_TEST_CASE(checkStatusMessageAccessors) {
    set_status_message(http::types::RESPONSE_MESSAGE_NOT_FOUND);
    BOOST_CHECK_EQUAL(get_status_message(), http::types::RESPONSE_MESSAGE_NOT_FOUND);
    set_status_message(http::types::RESPONSE_MESSAGE_CREATED);
    BOOST_CHECK_EQUAL(get_status_message(), http::types::RESPONSE_MESSAGE_CREATED);
}

BOOST_AUTO_TEST_CASE(checkSetLastModified) {
    set_last_modified(0);
    BOOST_CHECK_EQUAL(get_header(HEADER_LAST_MODIFIED), get_date_string(0));
    set_last_modified(100000000);
    BOOST_CHECK_EQUAL(get_header(HEADER_LAST_MODIFIED), get_date_string(100000000));
    set_last_modified(1000000000);
    BOOST_CHECK_EQUAL(get_header(HEADER_LAST_MODIFIED), get_date_string(1000000000));
}

BOOST_AUTO_TEST_SUITE_END()
