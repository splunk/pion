// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/test/unit_test.hpp>
#include <pion/algorithm.hpp>
#include <pion/http/parser.hpp>
#include <pion/http/request.hpp>
#include <pion/http/response.hpp>

#include "http_parser_tests_data.inc"

using namespace pion;
using namespace pion::net;

BOOST_AUTO_TEST_CASE(testParseSimpleQueryString)
{
    const std::string QUERY_STRING("a=b");
    HTTPTypes::QueryParams params;
    BOOST_REQUIRE(HTTPParser::parseURLEncoded(params, QUERY_STRING.c_str(), QUERY_STRING.size()));
    BOOST_CHECK_EQUAL(params.size(), 1UL);

    HTTPTypes::QueryParams::const_iterator i = params.find("a");
    BOOST_REQUIRE(i != params.end());
    BOOST_CHECK_EQUAL(i->second, "b");
}

BOOST_AUTO_TEST_CASE(testParseQueryStringWithMultipleValues)
{
    const std::string QUERY_STRING("test=2&three=%20four%20with%20spaces&five=sixty+two");
    HTTPTypes::QueryParams params;
    BOOST_REQUIRE(HTTPParser::parseURLEncoded(params, QUERY_STRING));
    BOOST_CHECK_EQUAL(params.size(), 3UL);

    HTTPTypes::QueryParams::const_iterator i = params.find("test");
    BOOST_REQUIRE(i != params.end());
    BOOST_CHECK_EQUAL(i->second, "2");
    i = params.find("three");
    BOOST_REQUIRE(i != params.end());
    BOOST_CHECK_EQUAL(algo::url_decode(i->second), " four with spaces");
    i = params.find("five");
    BOOST_REQUIRE(i != params.end());
    BOOST_CHECK_EQUAL(algo::url_decode(i->second), "sixty two");
}

BOOST_AUTO_TEST_CASE(testParseQueryStringWithDoubleAmpersand)
{
    const std::string QUERY_STRING("a=b&&c=d&e");
    HTTPTypes::QueryParams params;
    BOOST_REQUIRE(HTTPParser::parseURLEncoded(params, QUERY_STRING));
    BOOST_CHECK_EQUAL(params.size(), 3UL);

    HTTPTypes::QueryParams::const_iterator i = params.find("a");
    BOOST_REQUIRE(i != params.end());
    BOOST_CHECK_EQUAL(i->second, "b");
    i = params.find("c");
    BOOST_REQUIRE(i != params.end());
    BOOST_CHECK_EQUAL(i->second, "d");
    i = params.find("e");
    BOOST_REQUIRE(i != params.end());
    BOOST_CHECK(i->second.empty());
}

BOOST_AUTO_TEST_CASE(testParseQueryStringWithEmptyName)
{
    const std::string QUERY_STRING("a=b&=bob&=&c=d&e");
    HTTPTypes::QueryParams params;
    BOOST_REQUIRE(HTTPParser::parseURLEncoded(params, QUERY_STRING));
    BOOST_CHECK_EQUAL(params.size(), 3UL);

    HTTPTypes::QueryParams::const_iterator i = params.find("a");
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
    HTTPTypes::QueryParams params;
    BOOST_REQUIRE(HTTPParser::parseURLEncoded(params, QUERY_STRING));
    BOOST_CHECK_EQUAL(params.size(), 3UL);

    HTTPTypes::QueryParams::const_iterator i = params.find("a");
    BOOST_REQUIRE(i != params.end());
    BOOST_CHECK(i->second.empty());
    i = params.find("b");
    BOOST_REQUIRE(i != params.end());
    BOOST_CHECK(i->second.empty());
    i = params.find("c");
    BOOST_REQUIRE(i != params.end());
    BOOST_CHECK(i->second.empty());
}

BOOST_AUTO_TEST_CASE(testParseQueryStringWithTabs)
{
    const std::string QUERY_STRING("promoCode=BOB	");
    HTTPTypes::QueryParams params;
    BOOST_REQUIRE(HTTPParser::parseURLEncoded(params, QUERY_STRING));
    BOOST_CHECK_EQUAL(params.size(), 1UL);

    HTTPTypes::QueryParams::const_iterator i = params.find("promoCode");
    BOOST_REQUIRE(i != params.end());
    BOOST_CHECK_EQUAL(i->second, "BOB");
}

BOOST_AUTO_TEST_CASE(testParseSingleCookieHeader)
{
    std::string cookie_header;
    HTTPTypes::CookieParams cookies;
    HTTPTypes::CookieParams::const_iterator cookie_it;

    cookie_header = "a=b";
    BOOST_REQUIRE(HTTPParser::parseCookieHeader(cookies, cookie_header, false));
    BOOST_CHECK_EQUAL(cookies.size(), 1UL);

    cookie_it = cookies.find("a");
    BOOST_REQUIRE(cookie_it != cookies.end());
    BOOST_CHECK_EQUAL(cookie_it->second, "b");
}

BOOST_AUTO_TEST_CASE(testParseTwoCookieHeader)
{
    std::string cookie_header;
    HTTPTypes::CookieParams cookies;
    HTTPTypes::CookieParams::const_iterator cookie_it;

    cookie_header = "a=b; Part_Number=\"Rocket_Launcher_0001\";";
    BOOST_REQUIRE(HTTPParser::parseCookieHeader(cookies, cookie_header, false));
    BOOST_CHECK_EQUAL(cookies.size(), 2UL);

    cookie_it = cookies.find("a");
    BOOST_REQUIRE(cookie_it != cookies.end());
    BOOST_CHECK_EQUAL(cookie_it->second, "b");

    cookie_it = cookies.find("Part_Number");
    BOOST_REQUIRE(cookie_it != cookies.end());
    BOOST_CHECK_EQUAL(cookie_it->second, "Rocket_Launcher_0001");
}

BOOST_AUTO_TEST_CASE(testParseCookieHeaderWithEmptyName)
{
    std::string cookie_header;
    HTTPTypes::CookieParams cookies;
    HTTPTypes::CookieParams::const_iterator cookie_it;

    cookie_header = "a=b; =; =\"001\"; c=d";
    BOOST_REQUIRE(HTTPParser::parseCookieHeader(cookies, cookie_header, false));
    BOOST_CHECK_EQUAL(cookies.size(), 2UL);

    cookie_it = cookies.find("a");
    BOOST_REQUIRE(cookie_it != cookies.end());
    BOOST_CHECK_EQUAL(cookie_it->second, "b");

    cookie_it = cookies.find("c");
    BOOST_REQUIRE(cookie_it != cookies.end());
    BOOST_CHECK_EQUAL(cookie_it->second, "d");
}

BOOST_AUTO_TEST_CASE(testParseCookieHeaderWithUnquotedSpaces)
{
    std::string cookie_header;
    HTTPTypes::CookieParams cookies;
    HTTPTypes::CookieParams::const_iterator cookie_it;

    cookie_header = "a=a black cat; c = Dec 2, 2010 11:54:30 AM; d = \"dark \"";
    BOOST_REQUIRE(HTTPParser::parseCookieHeader(cookies, cookie_header, false));
    BOOST_CHECK_EQUAL(cookies.size(), 4UL);

    cookie_it = cookies.find("a");
    BOOST_REQUIRE(cookie_it != cookies.end());
    BOOST_CHECK_EQUAL(cookie_it->second, "a black cat");

    cookie_it = cookies.find("c");
    BOOST_REQUIRE(cookie_it != cookies.end());
    BOOST_CHECK_EQUAL(cookie_it->second, "Dec 2");

    // ideally, this would be parsed as part of c (as intended)
    // but the parser needs to accept , as a cookie separator to conform with v1 and later
    // so for now just not "breaking" is good enough
    cookie_it = cookies.find("201011:54:30AM");
    BOOST_REQUIRE(cookie_it != cookies.end());
    BOOST_CHECK_EQUAL(cookie_it->second, "");

    cookie_it = cookies.find("d");
    BOOST_REQUIRE(cookie_it != cookies.end());
    BOOST_CHECK_EQUAL(cookie_it->second, "dark ");
}

BOOST_AUTO_TEST_CASE(testParseNormalCookieHeader)
{
    std::string cookie_header;
    HTTPTypes::CookieParams cookies;
    HTTPTypes::CookieParams::const_iterator cookie_it;

    cookie_header = "$Version=\"1\"; Part_Number=\"Rocket_Launcher_0001\"; $Path=\"/acme\"";
    BOOST_REQUIRE(HTTPParser::parseCookieHeader(cookies, cookie_header, false));
    BOOST_CHECK_EQUAL(cookies.size(), 1UL);
    cookie_it = cookies.find("Part_Number");
    BOOST_REQUIRE(cookie_it != cookies.end());
    BOOST_CHECK_EQUAL(cookie_it->second, "Rocket_Launcher_0001");
}

BOOST_AUTO_TEST_CASE(testParseSetCookieHeader)
{
    std::string cookie_header;
    HTTPTypes::CookieParams cookies;
    HTTPTypes::CookieParams::const_iterator cookie_it;

    cookie_header = "Shipping=\"FedEx\"; Version=\"1\"; Path=\"/acme\"";
    BOOST_REQUIRE(HTTPParser::parseCookieHeader(cookies, cookie_header, true));
    BOOST_CHECK_EQUAL(cookies.size(), 1UL);
    cookie_it = cookies.find("Shipping");
    BOOST_REQUIRE(cookie_it != cookies.end());
    BOOST_CHECK_EQUAL(cookie_it->second, "FedEx");
}

BOOST_AUTO_TEST_CASE(testHTTPParserSimpleRequest)
{
    HTTPParser request_parser(true);
    request_parser.setReadBuffer((const char*)request_data_1, sizeof(request_data_1));

    HTTPRequest http_request;
    boost::system::error_code ec;
    BOOST_CHECK(request_parser.parse(http_request, ec));
    BOOST_CHECK(!ec);

    BOOST_CHECK_EQUAL(http_request.getContentLength(), 0UL);
    BOOST_CHECK_EQUAL(request_parser.getTotalBytesRead(), sizeof(request_data_1));
    BOOST_CHECK_EQUAL(request_parser.getContentBytesRead(), 0UL);
}

BOOST_AUTO_TEST_CASE(testHTTPParserSimpleResponse)
{
    HTTPParser response_parser(false);
    response_parser.setReadBuffer((const char*)response_data_1, sizeof(response_data_1));

    HTTPResponse http_response;
    boost::system::error_code ec;
    BOOST_CHECK(response_parser.parse(http_response, ec));
    BOOST_CHECK(!ec);

    BOOST_CHECK_EQUAL(http_response.getContentLength(), 117UL);
    BOOST_CHECK_EQUAL(response_parser.getTotalBytesRead(), sizeof(response_data_1));
    BOOST_CHECK_EQUAL(response_parser.getContentBytesRead(), 117UL);

    boost::regex content_regex("^GIF89a.*");
    BOOST_CHECK(boost::regex_match(http_response.getContent(), content_regex));
}

BOOST_AUTO_TEST_CASE(testHTTPParserBadRequest)
{
    HTTPParser request_parser(true);
    request_parser.setReadBuffer((const char*)request_data_bad, sizeof(request_data_bad));

    HTTPRequest http_request;
    boost::system::error_code ec;
    BOOST_CHECK(!request_parser.parse(http_request, ec));
    BOOST_CHECK_EQUAL(ec.value(), HTTPParser::ERROR_VERSION_CHAR);
    BOOST_CHECK_EQUAL(ec.message(), "invalid version character");
}

BOOST_AUTO_TEST_CASE(testHTTPParserSimpleResponseWithSmallerMaxSize)
{
    HTTPParser response_parser(false);
    response_parser.setReadBuffer((const char*)response_data_1, sizeof(response_data_1));
    response_parser.setMaxContentLength(4);

    HTTPResponse http_response;
    boost::system::error_code ec;
    BOOST_CHECK(response_parser.parse(http_response, ec));
    BOOST_CHECK(!ec);

    BOOST_CHECK_EQUAL(http_response.getContentLength(), 4UL);
    BOOST_CHECK_EQUAL(response_parser.getTotalBytesRead(), sizeof(response_data_1));
    BOOST_CHECK_EQUAL(response_parser.getContentBytesRead(), 117UL);

    std::string content_str("GIF8");
    BOOST_CHECK_EQUAL(content_str, http_response.getContent());
}

BOOST_AUTO_TEST_CASE(testHTTPParserSimpleResponseWithZeroMaxSize)
{
    HTTPParser response_parser(false);
    response_parser.setReadBuffer((const char*)response_data_1, sizeof(response_data_1));
    response_parser.setMaxContentLength(0);

    HTTPResponse http_response;
    boost::system::error_code ec;
    BOOST_CHECK(response_parser.parse(http_response, ec));
    BOOST_CHECK(!ec);

    BOOST_CHECK_EQUAL(http_response.getContentLength(), 0UL);
    BOOST_CHECK_EQUAL(response_parser.getTotalBytesRead(), sizeof(response_data_1));
    BOOST_CHECK_EQUAL(response_parser.getContentBytesRead(), 117UL);

    BOOST_CHECK_EQUAL(http_response.getContent()[0], '\0');
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
    boost::system::error_code ec;

    boost::uint64_t total_bytes = 0;
    for (int i=0; i <  frame_cnt - 1; i++ ) {
        response_parser.setReadBuffer((const char*)frames[i], sizes[i]);
        BOOST_CHECK( boost::indeterminate(response_parser.parse(http_response, ec)) );
        BOOST_CHECK(!ec);
        total_bytes += sizes[i];
    }

    response_parser.setReadBuffer((const char*)frames[frame_cnt - 1], sizes[frame_cnt - 1]);
    BOOST_CHECK( response_parser.parse(http_response, ec) );
        BOOST_CHECK(!ec);
    total_bytes += sizes[frame_cnt - 1];

    BOOST_CHECK_EQUAL(http_response.getContentLength(), 4712UL);
    BOOST_CHECK_EQUAL(response_parser.getTotalBytesRead(), total_bytes);
    BOOST_CHECK_EQUAL(response_parser.getContentBytesRead(), 4712UL);

    boost::regex content_regex(".*<title>Atomic\\sLabs:.*");
    BOOST_CHECK(boost::regex_match(http_response.getContent(), content_regex));
}


/// fixture used for testing HTTPParser's X-Fowarded-For header parsing
class HTTPParserForwardedForTests_F
{
public:
    HTTPParserForwardedForTests_F(void) {}
    ~HTTPParserForwardedForTests_F(void) {}

    inline void checkParsingTrue(const std::string& header, const std::string& result) {
        std::string public_ip;
        BOOST_CHECK(HTTPParser::parseForwardedFor(header, public_ip));
        BOOST_CHECK_EQUAL(public_ip, result);
    }

    inline void checkParsingFalse(const std::string& header) {
        std::string public_ip;
        BOOST_CHECK(! HTTPParser::parseForwardedFor(header, public_ip));
    }
};

BOOST_FIXTURE_TEST_SUITE(HTTPParserForwardedForTests_S, HTTPParserForwardedForTests_F)

BOOST_AUTO_TEST_CASE(checkParseForwardedForHeaderNoIP) {
    checkParsingFalse("myserver");
    checkParsingFalse("128.2.02f.12");
}

BOOST_AUTO_TEST_CASE(checkParseForwardedForHeaderNotPublic) {
    checkParsingFalse("127.0.0.1");
    checkParsingFalse("10.0.2.1");
    checkParsingFalse("192.168.2.12");
    checkParsingFalse("172.16.2.1");
    checkParsingFalse("172.21.2.1");
    checkParsingFalse("172.30.2.1");
}

BOOST_AUTO_TEST_CASE(checkParseForwardedForHeaderWithSpaces) {
    checkParsingTrue("   129.12.12.204   ", "129.12.12.204");
}

BOOST_AUTO_TEST_CASE(checkParseForwardedForHeaderFirstNotIP) {
    checkParsingTrue(" phono , 129.2.31.24, 62.31.21.2", "129.2.31.24");
    checkParsingTrue("not_ipv4, 127.2.31.24, 62.31.21.2", "62.31.21.2");
}

BOOST_AUTO_TEST_CASE(checkParseForwardedForHeaderFirstNotPublic) {
    checkParsingTrue("127.0.0.1, 62.31.21.2", "62.31.21.2");
    checkParsingTrue("10.21.31.2, 172.15.31.2", "172.15.31.2");
    checkParsingTrue("192.168.2.12, 172.32.31.2", "172.32.31.2");
}

BOOST_AUTO_TEST_SUITE_END()
