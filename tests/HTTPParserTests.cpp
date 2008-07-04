// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/test/unit_test.hpp>
#include <pion/net/HTTPParser.hpp>
#include <pion/net/HTTPRequest.hpp>
#include <pion/net/HTTPResponse.hpp>

#include "HTTPParserTestsData.inc"

using namespace pion::net;

BOOST_AUTO_TEST_CASE(testHTTPParserSimpleRequest)
{
	HTTPParser request_parser(true);
    request_parser.setReadBuffer((const char*)request_data_1, sizeof(request_data_1));

    HTTPRequest http_request;
	BOOST_CHECK(request_parser.parse(http_request));
}

BOOST_AUTO_TEST_CASE(testHTTPParserSimpleResponse)
{
    HTTPParser response_parser(false);
    response_parser.setReadBuffer((const char*)response_data_1, sizeof(response_data_1));

    HTTPResponse http_response;
    BOOST_CHECK(response_parser.parse(http_response));
}

BOOST_AUTO_TEST_CASE(testHTTPParser_MultipleResponseFrames)
{
	const unsigned char* frames[] = { resp2_frame0, resp2_frame1, resp2_frame2, 
			resp2_frame3, resp2_frame4, resp2_frame5, resp2_frame6 };

	size_t sizes[] = { sizeof(resp2_frame0), sizeof(resp2_frame1), sizeof(resp2_frame2), 
			sizeof(resp2_frame3), sizeof(resp2_frame4), sizeof(resp2_frame5), sizeof(resp2_frame6) };

	int frame_cnt = sizeof(frames)/sizeof(frames[0]);

    HTTPParser response_parser(false);
    HTTPResponse http_response;

    for (int i=0; i <  frame_cnt - 1; i++ ) {
        response_parser.setReadBuffer((const char*)frames[i], sizes[i]);
		BOOST_CHECK( boost::indeterminate(response_parser.parse(http_response)) );
	}

    response_parser.setReadBuffer((const char*)frames[frame_cnt - 1], sizes[frame_cnt - 1]);
    BOOST_CHECK( response_parser.parse(http_response) );
}
