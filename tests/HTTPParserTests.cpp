// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/test/unit_test.hpp>
#include <pion/net/HTTPMessageParser.hpp>

#include "HTTPParserTestsData.inc"

using namespace pion::net;

BOOST_AUTO_TEST_CASE(testHTTPMessageParser)
{
	HTTPMessageParser request_parser(true);
	BOOST_CHECK(request_parser.readNext((const char*)request_data_1, sizeof(request_data_1)));

	HTTPMessageParser response_parser(false);
	BOOST_CHECK(response_parser.readNext((const char*)response_data_1, sizeof(response_data_1)));
}

BOOST_AUTO_TEST_CASE(testHTTPMessageParser_MultiFrame)
{
	HTTPMessageParser request_parser(true);
	BOOST_CHECK(request_parser.readNext((const char*)request_data_1, sizeof(request_data_1)));

	HTTPMessageParser response_parser(false);
	const unsigned char* frames[] = { resp2_frame0, resp2_frame1, resp2_frame2, 
			resp2_frame3, resp2_frame4, resp2_frame5, resp2_frame6 };

	size_t sizes[] = { sizeof(resp2_frame0), sizeof(resp2_frame1), sizeof(resp2_frame2), 
			sizeof(resp2_frame3), sizeof(resp2_frame4), sizeof(resp2_frame5), sizeof(resp2_frame6) };
	int frame_cnt = sizeof(frames)/sizeof(frames[0]);

	for(int i=0; i <  frame_cnt - 1; i++ )
	{
		BOOST_CHECK( boost::indeterminate(response_parser.readNext((const char*)frames[i], sizes[i])) );
	}

	BOOST_CHECK( response_parser.readNext((const char*)frames[frame_cnt - 1], sizes[frame_cnt - 1]) );
}
