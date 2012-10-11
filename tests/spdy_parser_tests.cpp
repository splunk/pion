// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/config.hpp>
#include <pion/test/unit_test.hpp>
#include <boost/test/unit_test.hpp>
#include <pion/spdy/parser.hpp>

#include "spdy_parser_tests_data.inc"


using namespace pion;
using namespace pion::spdy;


class parser_F
    : public parser
{
public:
    parser_F() {
    }
    ~parser_F() {
    }
};

BOOST_FIXTURE_TEST_SUITE(parser_S, parser_F)

BOOST_AUTO_TEST_CASE(test_is_spdy_frame_methods)
{
    // Try with a invalid SPDY frame
    uint16_t sample_frame = 0xFF;
    boost::system::error_code ec;
    
    BOOST_CHECK_EQUAL(parser::is_spdy_frame((const char *)&(sample_frame)), false);
    BOOST_CHECK_EQUAL(parser::is_spdy_control_frame((const char *)&(sample_frame)), false);
    
    // Try with valid SPDY Frames
    
    BOOST_CHECK_EQUAL(parser::is_spdy_frame((const char*)spdy_syn_reply_frame), true);
    BOOST_CHECK_EQUAL(parser::is_spdy_control_frame((const char*)spdy_syn_reply_frame), true);
}

BOOST_AUTO_TEST_CASE(test_spdy_parse_syn_reply_frame)
{
    // Parse a spdy response frame
    
    boost::system::error_code ec;
    http_protocol_info http_info;
    
    // The length is known for this packet
    uint32_t length_packet = 1460;
    
    spdy_control_frame_info frame;
    uint32_t                stream_id = 0;
    
    this->set_read_ptr((const char*)spdy_syn_reply_frame);
    
    this->populate_frame(ec, &frame, length_packet, stream_id, http_info);
    
    // Check the frame properties
    
    BOOST_CHECK_EQUAL(frame.control_bit, 1U);
    BOOST_CHECK_EQUAL(frame.flags, 0U);
    BOOST_CHECK_EQUAL(frame.length, 280U);
    BOOST_CHECK_EQUAL(frame.type, 2U);
    BOOST_CHECK_EQUAL(frame.version, 2U);

    BOOST_CHECK_EQUAL(stream_id, 1U);
}

BOOST_AUTO_TEST_CASE(test_spdy_parse_syn_stream_frame)
{
    // Parse a spdy response frame
    
    boost::system::error_code ec;
    http_protocol_info http_info;
    
    // The length is known for this packet
    uint32_t length_packet = 294;
    
    spdy_control_frame_info frame;
    uint32_t                stream_id = 0;
    
    this->set_read_ptr((const char*)spdy_syn_stream_frame);
    
    this->populate_frame(ec,
                               &frame,
                               length_packet,
                               stream_id,
                               http_info);
    
    // Check the frame properties
    
    BOOST_CHECK_EQUAL(frame.control_bit, 1U);
    BOOST_CHECK_EQUAL(frame.flags, 1U);
    BOOST_CHECK_EQUAL(frame.length, 286U);
    BOOST_CHECK_EQUAL(frame.type, 1U);
    BOOST_CHECK_EQUAL(frame.version, 2U);
    
    BOOST_CHECK_EQUAL(stream_id, 1U);
    
    BOOST_CHECK_EQUAL(http_info.data_offset, 8U);
    BOOST_CHECK_EQUAL(http_info.data_size, 286U);
}


BOOST_AUTO_TEST_CASE(test_spdy_parse_interleaved_frame)
{
    // Parse a spdy response frame
    
    http_protocol_info http_info;
    boost::system::error_code ec;
    
    // Check for interleaved spdy frames
    // The length is known for this packet
    uint32_t length_packet = 1460;
    
    bool more_to_parse = false;
    
    more_to_parse = this->parse(http_info, ec,
                                (const char*)spdy_window_frame,
                                length_packet, 1);
    
    BOOST_CHECK_EQUAL(more_to_parse, true);
}

BOOST_AUTO_TEST_CASE(test_spdy_parse_header)
{
    // Parse a spdy response frame
    
    http_protocol_info          http_info;
    boost::system::error_code   ec;
    
    // Check for interleaved spdy frames
    // The length is known for this packet
    uint32_t length_packet = 1460;
    
    bool more_to_parse = false;
    
    more_to_parse = this->parse(http_info,
                                ec,
                                (const char*)spdy_syn_stream_frame,
                                length_packet,
                                0);
    
    BOOST_CHECK_EQUAL(more_to_parse, true);
    
    // Verify the HTTP Info
    BOOST_CHECK_EQUAL(http_info.data_offset, 8U);
    BOOST_CHECK_EQUAL(http_info.data_size, 286U);
    BOOST_CHECK_EQUAL(http_info.http_type, 1U);
    BOOST_CHECK_EQUAL(http_info.last_chunk, false);
    BOOST_CHECK_EQUAL(http_info.stream_id, 1U);
    
    BOOST_CHECK_EQUAL(http_info.http_headers.size(), 10U);
    
    BOOST_CHECK_EQUAL(http_info.http_headers["host"], "www.cnn.com");
    BOOST_CHECK_EQUAL(http_info.http_headers["accept-encoding"], "gzip,deflate,sdch");
    BOOST_CHECK_EQUAL(http_info.http_headers["accept-language"], "en-US,en;q=0.8");
    BOOST_CHECK_EQUAL(http_info.http_headers["method"], "GET");
    BOOST_CHECK_EQUAL(http_info.http_headers["scheme"], "http");
    BOOST_CHECK_EQUAL(http_info.http_headers["url"], "/");
    BOOST_CHECK_EQUAL(http_info.http_headers["version"], "HTTP/1.1");
}

BOOST_AUTO_TEST_CASE(test_populate_http_info_syn_stream_frame)
{
    // Parse a spdy response frame
    
    boost::system::error_code ec;
    
    // Check for interleaved spdy frames
    // The length is known for this packet
    uint32_t length_packet = 294;
    
    bool more_to_parse = false;
    
    http_protocol_info   http_info;
    
    more_to_parse = this->parse(http_info, ec,
                                (const char*)spdy_syn_stream_frame,
                                length_packet, 1);
    
    BOOST_CHECK_EQUAL(http_info.http_headers.size(), 10U);
    BOOST_CHECK_EQUAL(http_info.data_offset, 8U);
    BOOST_CHECK_EQUAL(http_info.data_size, 286U);
    BOOST_CHECK_EQUAL(http_info.http_type, 1U);
    BOOST_CHECK_EQUAL(http_info.last_chunk, false);
}

BOOST_AUTO_TEST_CASE(test_populate_http_info_datastream_frame)
{
    // Parse a spdy response frame
    
    boost::system::error_code ec;
    
    // Check for interleaved spdy frames
    // The length is known for this packet
    
    bool more_to_parse = false;

    http_protocol_info   http_info;
    uint32_t length_packet = 1460;

    more_to_parse = this->parse(http_info, ec,
                                (const char*)spdy_datastream_frame,
                                length_packet, 1);

    BOOST_CHECK_EQUAL(http_info.http_headers.size(), 0U);
    BOOST_CHECK_EQUAL(http_info.data_offset, 8U);
    BOOST_CHECK_EQUAL(http_info.data_size, 1427U);
    BOOST_CHECK_EQUAL(http_info.http_type, 3U);
    BOOST_CHECK_EQUAL(http_info.last_chunk, false);
}

BOOST_AUTO_TEST_SUITE_END()