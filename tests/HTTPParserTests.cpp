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

BOOST_AUTO_TEST_CASE(testParseSimpleQueryString)
{
	const std::string QUERY_STRING("a=b");
	HTTPTypes::StringDictionary params;
	BOOST_REQUIRE(HTTPParser::parseURLEncoded(params, QUERY_STRING.c_str(), QUERY_STRING.size()));
	BOOST_CHECK_EQUAL(params.size(), 1UL);

	HTTPTypes::StringDictionary::const_iterator i = params.find("a");
	BOOST_REQUIRE(i != params.end());
	BOOST_CHECK_EQUAL(i->second, "b");
}

BOOST_AUTO_TEST_CASE(testParseQueryStringWithMultipleValues)
{
	const std::string QUERY_STRING("test=2&three=%20four%20with%20spaces&five=sixty+two");
	HTTPTypes::StringDictionary params;
	BOOST_REQUIRE(HTTPParser::parseURLEncoded(params, QUERY_STRING));
	BOOST_CHECK_EQUAL(params.size(), 3UL);

	HTTPTypes::StringDictionary::const_iterator i = params.find("test");
	BOOST_REQUIRE(i != params.end());
	BOOST_CHECK_EQUAL(i->second, "2");
	i = params.find("three");
	BOOST_REQUIRE(i != params.end());
	BOOST_CHECK_EQUAL(HTTPTypes::url_decode(i->second), " four with spaces");
	i = params.find("five");
	BOOST_REQUIRE(i != params.end());
	BOOST_CHECK_EQUAL(HTTPTypes::url_decode(i->second), "sixty two");
}

BOOST_AUTO_TEST_CASE(testParseQueryStringWithDoubleAmpersand)
{
	const std::string QUERY_STRING("a=b&&c=d&e");
	HTTPTypes::StringDictionary params;
	BOOST_REQUIRE(HTTPParser::parseURLEncoded(params, QUERY_STRING));
	BOOST_CHECK_EQUAL(params.size(), 3UL);

	HTTPTypes::StringDictionary::const_iterator i = params.find("a");
	BOOST_REQUIRE(i != params.end());
	BOOST_CHECK_EQUAL(i->second, "b");
	i = params.find("c");
	BOOST_REQUIRE(i != params.end());
	BOOST_CHECK_EQUAL(i->second, "d");
	i = params.find("e");
	BOOST_REQUIRE(i != params.end());
	BOOST_CHECK(i->second.empty());
}

BOOST_AUTO_TEST_CASE(testParseQueryStringWithEmptyValues)
{
	const std::string QUERY_STRING("a=&b&c=");
	HTTPTypes::StringDictionary params;
	BOOST_REQUIRE(HTTPParser::parseURLEncoded(params, QUERY_STRING));
	BOOST_CHECK_EQUAL(params.size(), 3UL);

	HTTPTypes::StringDictionary::const_iterator i = params.find("a");
	BOOST_REQUIRE(i != params.end());
	BOOST_CHECK(i->second.empty());
	i = params.find("b");
	BOOST_REQUIRE(i != params.end());
	BOOST_CHECK(i->second.empty());
	i = params.find("c");
	BOOST_REQUIRE(i != params.end());
	BOOST_CHECK(i->second.empty());
}

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
