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


class parser_F {
public:
    parser_F() {
    }
    ~parser_F() {
    }
};

BOOST_FIXTURE_TEST_SUITE(parser_S, parser_F)

BOOST_AUTO_TEST_CASE(test_is_spdy_frame)
{
    // Try with a invalid SPDY frame
    uint16_t sample_frame = 0xFF;
    boost::system::error_code ec;
    
    SPDYStreamCompressor    spdy_compressor;
    
    bool result = pion::spdy::parser::is_spdy_frame((const char *)&(sample_frame), spdy_compressor);
    
    BOOST_CHECK_EQUAL(result, false);
    
    // Try with valid SPDY Frames
    
    result = pion::spdy::parser::is_spdy_frame((const char*)spdy_syn_reply_frame, spdy_compressor);
    
    BOOST_CHECK_EQUAL(result, true);
}

BOOST_AUTO_TEST_CASE(test_spdy_parse_frame)
{
    // Parse a spdy response frame
    
    boost::system::error_code ec;
    http_protocol_info http_info;
    
    {
        // The length is known for this packet
        uint32_t length_packet = 1460;
        
        spdy_control_frame_info frame;
        uint32_t                stream_id = 0;
        
        parser spdy_parser((const char*)spdy_syn_reply_frame, ec);
        
        spdy_parser.populate_frame(ec, &frame, length_packet, stream_id, http_info);
        
        // Check the frame properties
        
        BOOST_CHECK_EQUAL(frame.control_bit, 1);
        BOOST_CHECK_EQUAL(frame.flags, 0);
        BOOST_CHECK_EQUAL(frame.length, 280);
        BOOST_CHECK_EQUAL(frame.type, 2);
        BOOST_CHECK_EQUAL(frame.version, 2);
        
        BOOST_CHECK_EQUAL(stream_id, 1);
    }
    
    // Parse a spdy stream frame
    {
        // The length is known for this packet
        uint32_t length_packet = 294;
        
        spdy_control_frame_info frame;
        uint32_t                stream_id = 0;
        
        parser spdy_parser((const char*)spdy_syn_stream_frame, ec);
        
        spdy_parser.populate_frame(ec,
                                   &frame,
                                   length_packet,
                                   stream_id,
                                   http_info);
        
        // Check the frame properties
        
        BOOST_CHECK_EQUAL(frame.control_bit, 1);
        BOOST_CHECK_EQUAL(frame.flags, 1);
        BOOST_CHECK_EQUAL(frame.length, 286);
        BOOST_CHECK_EQUAL(frame.type, 1);
        BOOST_CHECK_EQUAL(frame.version, 2);
        
        BOOST_CHECK_EQUAL(stream_id, 1);
        
        BOOST_CHECK_EQUAL(http_info.data_offset, 16);
        BOOST_CHECK_EQUAL(http_info.data_size, 286);
    }
}

BOOST_AUTO_TEST_CASE(test_spdy_parse_interleaved_frame)
{
    // Parse a spdy response frame
    
    http_protocol_info http_info;
    SPDYStreamCompressor    spdy_compressor;
    boost::system::error_code ec;
    
    // Check for interleaved spdy frames
    // The length is known for this packet
    uint32_t length_packet = 1460;
    
    parser spdy_parser((const char*)spdy_window_frame, ec);
    
    bool more_to_parse = false;
    
    more_to_parse = spdy_parser.parse(spdy_compressor,
                                      http_info,
                                      ec,
                                      length_packet,
                                      1);
    
    BOOST_CHECK_EQUAL(more_to_parse, true);
}

BOOST_AUTO_TEST_CASE(test_spdy_parse_header)
{
    // Parse a spdy response frame
    
    http_protocol_info          http_info;
    SPDYStreamCompressor        spdy_compressor;
    boost::system::error_code   ec;
    
    // Check for interleaved spdy frames
    // The length is known for this packet
    uint32_t length_packet = 1460;
    
    parser spdy_parser((const char*)spdy_syn_stream_frame, ec);
    
    bool more_to_parse = false;
    
    more_to_parse = spdy_parser.parse(spdy_compressor,
                                      http_info,
                                      ec,
                                      length_packet,
                                      0);
    
    BOOST_CHECK_EQUAL(more_to_parse, true);
    
    // Verify the HTTP Info
    BOOST_CHECK_EQUAL(http_info.data_offset, 8);
    BOOST_CHECK_EQUAL(http_info.data_size, 286);
    BOOST_CHECK_EQUAL(http_info.http_type, 1);
    BOOST_CHECK_EQUAL(http_info.last_chunk, false);
    BOOST_CHECK_EQUAL(http_info.stream_id, 1);
    
    BOOST_CHECK_EQUAL(http_info.http_headers.size(), 10);
    
    BOOST_CHECK_EQUAL(http_info.http_headers["host"], "www.cnn.com");
    BOOST_CHECK_EQUAL(http_info.http_headers["accept-encoding"], "gzip,deflate,sdch");
    BOOST_CHECK_EQUAL(http_info.http_headers["accept-language"], "en-US,en;q=0.8");
    BOOST_CHECK_EQUAL(http_info.http_headers["method"], "GET");
    BOOST_CHECK_EQUAL(http_info.http_headers["scheme"], "http");
    BOOST_CHECK_EQUAL(http_info.http_headers["url"], "/");
    BOOST_CHECK_EQUAL(http_info.http_headers["version"], "HTTP/1.1");
}

BOOST_AUTO_TEST_CASE(testSPDYParseData)
{
    // Parse a spdy response frame
    
    http_protocol_info   http_info;
    SPDYStreamCompressor    spdy_compressor;
    boost::system::error_code ec;
    
    // Check for interleaved spdy frames
    // The length is known for this packet
    uint32_t length_packet = 294;
    
    bool more_to_parse = false;
    
    {
        parser spdy_parser((const char*)spdy_syn_stream_frame, ec);
        
        
        more_to_parse = spdy_parser.parse(spdy_compressor,
                                          http_info,
                                          ec,
                                          length_packet,
                                          1);
    }
    
    BOOST_CHECK_EQUAL(http_info.http_headers.size(), 0);
    BOOST_CHECK_EQUAL(http_info.data_offset, 8);
    BOOST_CHECK_EQUAL(http_info.data_size, 286);
    BOOST_CHECK_EQUAL(http_info.http_type, 1);
    BOOST_CHECK_EQUAL(http_info.last_chunk, false);
    
    
    {
        uint32_t length_packet = 1460;
        
        parser spdy_parser((const char*)spdy_datastream_frame, ec);
        
        more_to_parse = spdy_parser.parse(spdy_compressor,
                                          http_info,
                                          ec,
                                          length_packet,
                                          1);
        
    }
    
    
    BOOST_CHECK_EQUAL(http_info.http_headers.size(), 0);
    
    BOOST_CHECK_EQUAL(http_info.data_offset, 16);
    BOOST_CHECK_EQUAL(http_info.data_size, 1427);
    BOOST_CHECK_EQUAL(http_info.http_type, 3);
    BOOST_CHECK_EQUAL(http_info.last_chunk, false);
}


BOOST_AUTO_TEST_SUITE_END()