// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/config.hpp>
#include <pion/algorithm.hpp>
#include <pion/test/unit_test.hpp>
#include <boost/test/unit_test.hpp>
#include <pion/spdy/parser.hpp>
#include <pion/spdy/decompressor.hpp>

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
    boost::uint16_t sample_frame = 0xFF;
    boost::system::error_code ec;
    
    BOOST_CHECK_EQUAL(parser::get_spdy_frame_type((const char *)&(sample_frame)), spdy_invalid_frame);
    BOOST_CHECK_EQUAL(parser::is_spdy_control_frame((const char *)&(sample_frame)), false);
    
    // Try with valid SPDY Frames
    
    BOOST_CHECK_EQUAL(parser::get_spdy_frame_type((const char*)spdy_syn_reply_frame),
                                                  spdy_control_frame);
    BOOST_CHECK_EQUAL(parser::is_spdy_control_frame((const char*)spdy_syn_reply_frame), true);
    
    // Try a packet with low version number
    BOOST_CHECK_EQUAL(parser::is_spdy_control_frame((const char*)spdy_low_version_frame), false);
    
    // Try a packet with high version number
    BOOST_CHECK_EQUAL(parser::is_spdy_control_frame((const char*)spdy_high_version_frame), false);
    
    // Try with an invalid type frame
    BOOST_CHECK_EQUAL(parser::is_spdy_control_frame((const char*)spdy_invalid_type_frame), false);
    
    // Try with an invalid control frame
    BOOST_CHECK_EQUAL(parser::is_spdy_control_frame((const char*)spdy_incorrect_control_frame), false);
}

BOOST_AUTO_TEST_CASE(test_spdy_control_frame_stream_id)
{
    // Get the SPDY control frame stream id
    
    BOOST_CHECK_EQUAL(parser::get_control_frame_stream_id((const char*)spdy_control_frame_1), 6U);
                      
    BOOST_CHECK_EQUAL(parser::get_control_frame_stream_id((const char*)spdy_control_frame_2), 1793U);
}

BOOST_AUTO_TEST_CASE(test_get_spdy_frame_type)
{
    // Get the SPDY control frame type
    
    BOOST_CHECK_EQUAL(parser::get_spdy_frame_type((const char*)spdy_control_frame_1),
                      spdy_control_frame);
    
    BOOST_CHECK_EQUAL(parser::get_spdy_frame_type((const char*)spdy_control_frame_2),
                      spdy_control_frame);
    
    BOOST_CHECK_EQUAL(parser::get_spdy_frame_type((const char*)spdy_incorrect_control_frame),
                      spdy_invalid_frame);
    
    BOOST_CHECK_EQUAL(parser::get_spdy_frame_type((const char*)spdy_datastream_frame),
                      spdy_data_frame);
}

BOOST_AUTO_TEST_CASE(test_spdy_parse_syn_reply_frame)
{
    // Parse a spdy response frame
    
    boost::system::error_code ec;
    http_protocol_info http_info;
    
    // The length is known for this packet
    boost::uint32_t length_packet = 1460;
    
    spdy_control_frame_info frame;
    boost::uint32_t                stream_id = 0;
    
    this->set_read_ptr((const char*)spdy_syn_reply_frame);
    
    this->populate_frame(ec, frame, length_packet, stream_id, http_info);
    
    // Check the frame properties
    
    BOOST_CHECK_EQUAL(frame.control_bit, 1U);
    BOOST_CHECK_EQUAL(frame.flags, 0U);
    BOOST_CHECK_EQUAL(frame.length, 280U);
    BOOST_CHECK_EQUAL(frame.type, 2U);
    BOOST_CHECK_EQUAL(frame.version, 2U);

    BOOST_CHECK_EQUAL(stream_id, 1U);
}

BOOST_AUTO_TEST_CASE(test_spdy_parse_rst_frame)
{
    // Parse a spdy rst frame
    
    http_protocol_info http_info;
    boost::system::error_code ec;
    
    decompressor_ptr decompressor = (decompressor_ptr)new pion::spdy::decompressor();
    
    // Check for interleaved spdy frames
    // The length is known for this packet
    boost::uint32_t length_packet = 30;
    
    boost::tribool result = false;
    
    result = this->parse(http_info, ec,
                         decompressor,
                         (const char*)spdy_rst_frame,
                         length_packet, 1);

    // Check if the parser recognized it as an incorrect rst frame
    BOOST_CHECK_EQUAL(result, true);
    BOOST_CHECK_EQUAL(algorithm::to_uint32(this->get_spdy_data_content()), 1U);
    
    length_packet = 30;
    
    result = this->parse(http_info, ec,
                         decompressor,
                         (const char*)spdy_rst_frame_with_correct_length,
                         length_packet, 1);
    
    // Check if the parser recognized it as an incorrect rst frame
    BOOST_CHECK(boost::indeterminate(result));
    BOOST_CHECK_EQUAL(algorithm::to_uint32(this->get_spdy_data_content()), 6169U);
}

BOOST_AUTO_TEST_CASE(test_spdy_parse_goaway_frame)
{
    // Parse a spdy go away frame
    
    http_protocol_info http_info;
    boost::system::error_code ec;
    
    decompressor_ptr decompressor = (decompressor_ptr)new pion::spdy::decompressor();
    
    // Check for interleaved spdy frames
    // The length is known for this packet
    boost::uint32_t length_packet = 30;
    
    boost::tribool result = false;
    
    result = this->parse(http_info, ec,
                         decompressor,
                         (const char*)spdy_goaway_frame,
                         length_packet, 1);
    
    // Check if the parser recognized it as an incorrect rst frame
    BOOST_CHECK_EQUAL(result, true);
    BOOST_CHECK_EQUAL(algorithm::to_uint32(this->get_spdy_data_content()), 1U);
    
    length_packet = 30;
    
    result = this->parse(http_info, ec,
                         decompressor,
                         (const char*)spdy_goaway_frame_with_correct_length,
                         length_packet, 1);
    
    // Check if the parser recognized it as an incorrect rst frame
    BOOST_CHECK(boost::indeterminate(result));
    BOOST_CHECK_EQUAL(algorithm::to_uint32(this->get_spdy_data_content()), 6169U);
}

BOOST_AUTO_TEST_CASE(test_spdy_parse_frames)
{
    // Parse a possible spdy frames which we don't parse.
    // The frames are not parsed but parsing these frames should not cause
    // any unwanted conditions like seg faults etx
    
    http_protocol_info http_info;
    boost::system::error_code ec;
    
    decompressor_ptr decompressor = (decompressor_ptr)new pion::spdy::decompressor();
    
    // Check for interleaved spdy frames
    // The length is known for this packet
    boost::uint32_t length_packet = 30;
    
    boost::tribool result = false;
    
    BOOST_CHECK_NO_THROW(result = this->parse(http_info, ec,
                                              decompressor,
                                              (const char*)spdy_settings_frame,
                                              length_packet, 1));
    BOOST_CHECK_EQUAL(result, true);

    
    BOOST_CHECK_NO_THROW(result = this->parse(http_info, ec,
                                              decompressor,
                                              (const char*)spdy_noop_frame,
                                              length_packet, 1));
    BOOST_CHECK_EQUAL(result, true);

    
    BOOST_CHECK_NO_THROW(result = this->parse(http_info, ec,
                                              decompressor,
                                              (const char*)spdy_headers_frame,
                                              length_packet, 1));
    BOOST_CHECK_EQUAL(result, false);  // TODO: should this be failing?

    
    BOOST_CHECK_NO_THROW(result = this->parse(http_info, ec,
                                              decompressor,
                                              (const char*)spdy_window_update_frame,
                                              length_packet, 1));
    BOOST_CHECK(boost::indeterminate(result));
    
    BOOST_CHECK_NO_THROW(result = this->parse(http_info, ec,
                                              decompressor,
                                              (const char*)spdy_credential_frame,
                                              length_packet, 1));
    BOOST_CHECK(boost::indeterminate(result));
    
    BOOST_CHECK_NO_THROW(result = this->parse(http_info, ec,
                                              decompressor,
                                              (const char*)spdy_invalid_frame_type,
                                              length_packet, 1));
    BOOST_CHECK_EQUAL(result, false);  // TODO: should this be failing?
}

BOOST_AUTO_TEST_CASE(test_spdy_parse_ping_frame)
{
    // Parse a spdy ping frame
    
    http_protocol_info http_info;
    boost::system::error_code ec;
    
    decompressor_ptr decompressor = (decompressor_ptr)new pion::spdy::decompressor();
    
    // Check for interleaved spdy frames
    // The length is known for this packet
    boost::uint32_t length_packet = 30;
    
    boost::tribool result = false;
    
    result = this->parse(http_info, ec,
                         decompressor,
                         (const char*)spdy_ping_frame,
                         length_packet, 1);
    
    // Check if the parser recognized it as an incorrect rst frame
    BOOST_CHECK_EQUAL(result, true);
    BOOST_CHECK_EQUAL(algorithm::to_uint32(this->get_spdy_data_content()), 1U);
    
    length_packet = 30;
    
    result = this->parse(http_info, ec,
                         decompressor,
                         (const char*)spdy_ping_frame_with_correct_length,
                         length_packet, 1);
    
    // Check if the parser recognized it as an incorrect rst frame
    BOOST_CHECK(boost::indeterminate(result));
    BOOST_CHECK_EQUAL(algorithm::to_uint32(this->get_spdy_data_content()), 6169U);
}

BOOST_AUTO_TEST_CASE(test_spdy_parse_syn_stream_frame)
{
    // Parse a spdy response frame
    
    boost::system::error_code ec;
    http_protocol_info http_info;
    
    // The length is known for this packet
    boost::uint32_t length_packet = 294;
    
    spdy_control_frame_info frame;
    boost::uint32_t                stream_id = 0;
    
    this->set_read_ptr((const char*)spdy_syn_stream_frame);
    
    this->populate_frame(ec, frame, length_packet, stream_id, http_info);
    
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
    
    decompressor_ptr decompressor = (decompressor_ptr)new pion::spdy::decompressor();
    
    // Check for interleaved spdy frames
    // The length is known for this packet
    boost::uint32_t length_packet = 1460;
    
    boost::tribool result = false;
    
    result = this->parse(http_info, ec,
                         decompressor,
                         (const char*)spdy_window_frame,
                         length_packet, 1);
    
    BOOST_CHECK(boost::indeterminate(result));
}

BOOST_AUTO_TEST_CASE(test_spdy_parse_header)
{
    // Parse a spdy response frame
    
    http_protocol_info          http_info;
    boost::system::error_code   ec;
    
    // Check for interleaved spdy frames
    // The length is known for this packet
    boost::uint32_t length_packet = 1460;
    
    decompressor_ptr decompressor = (decompressor_ptr)new pion::spdy::decompressor();
    
    boost::tribool result = false;
    
    result = this->parse(http_info,
                         ec,
                         decompressor,
                         (const char*)spdy_syn_stream_frame,
                         length_packet,
                         0);
    
    BOOST_CHECK(boost::indeterminate(result));
    
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
    boost::uint32_t length_packet = 294;
    
    decompressor_ptr decompressor = (decompressor_ptr)new pion::spdy::decompressor();
    
    http_protocol_info   http_info;
    
    boost::tribool result = false;
    
    result = this->parse(http_info, ec,
                         decompressor,
                         (const char*)spdy_syn_stream_frame,
                         length_packet, 1);

    BOOST_CHECK_EQUAL(result, true);
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
    
    decompressor_ptr decompressor = (decompressor_ptr)new pion::spdy::decompressor();

    http_protocol_info   http_info;
    boost::uint32_t length_packet = 1460;

    boost::tribool result = false;
    
    result = this->parse(http_info, ec,
                         decompressor,
                         (const char*)spdy_datastream_frame,
                         length_packet, 1);

    BOOST_CHECK(boost::indeterminate(result));
    BOOST_CHECK_EQUAL(http_info.http_headers.size(), 0U);
    BOOST_CHECK_EQUAL(http_info.data_offset, 8U);
    BOOST_CHECK_EQUAL(http_info.data_size, 1427U);
    BOOST_CHECK_EQUAL(http_info.http_type, 3U);
    BOOST_CHECK_EQUAL(http_info.last_chunk, false);
}

BOOST_AUTO_TEST_SUITE_END()

