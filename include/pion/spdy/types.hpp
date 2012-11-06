// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_SPDYTYPES_HEADER__
#define __PION_SPDYTYPES_HEADER__

#include <map>
#include <pion/config.hpp>


namespace pion {    // begin namespace pion
namespace spdy {    // begin namespace spdy 

    
#define MIN_SPDY_VERSION            3
    
// The types of SPDY frames
#define SPDY_DATA                   0
#define SPDY_SYN_STREAM             1
#define SPDY_SYN_REPLY              2
#define SPDY_RST_STREAM             3
#define SPDY_SETTINGS               4
#define SPDY_PING                   6
#define SPDY_GOAWAY                 7
#define SPDY_HEADERS                8
#define SPDY_WINDOW_UPDATE          9
#define SPDY_CREDENTIAL             10
#define SPDY_INVALID                11
    
#define SPDY_FLAG_FIN               0x01
#define SPDY_FLAG_UNIDIRECTIONAL    0x02
    
#define SIZE_OF_BYTE                8
    
#define NON_SPDY                    0
#define HTTP_REQUEST                1
#define HTTP_RESPONSE               2
#define HTTP_DATA                   3
#define SPDY_CONTROL                4

    
/// SPDY value string data type
typedef struct _value_string {
    uint32_t  value;
    std::string   str;
} value_string;

    
/// Int-String value pairs of the status code for the RST Stream
static const value_string rst_stream_status_names[] = {
    { 1,  "PROTOCOL_ERROR" },
    { 2,  "INVALID_STREAM" },
    { 3,  "REFUSED_STREAM" },
    { 4,  "UNSUPPORTED_VERSION" },
    { 5,  "CANCEL" },
    { 6,  "INTERNAL_ERROR" },
    { 7,  "FLOW_CONTROL_ERROR" },
    { 8,  "STREAM_IN_USE" },
    { 9,  "STREAM_ALREADY_CLOSED" },
    { 10, "INVALID_CREDENTIALS" },
    { 11, "FRAME_TOO_LARGE" },
    { 12, "INVALID" },
};
    

/// This structure will be tied to each SPDY frame
typedef struct spdy_control_frame_info{
    bool control_bit;
    uint16_t  version;
    uint16_t  type;
    uint8_t   flags;
    uint32_t  length;  // Actually only 24 bits.
} spdy_control_frame_info;

    
/// This structure will be tied to each SPDY header frame.
/// Only applies to frames containing headers: SYN_STREAM, SYN_REPLY, HEADERS
/// Note that there may be multiple SPDY frames in one packet.
typedef struct _spdy_header_info{
    uint32_t stream_id;
    uint8_t *header_block;
    uint8_t  header_block_len;
    uint16_t frame_type;
} spdy_header_info;


/// This structure contains the HTTP Protocol information
typedef struct _http_protocol_info_t{
    std::map<std::string, std::string> http_headers;
    uint        http_type;
    uint        stream_id;
    uint        data_offset;
    uint        data_size;
    bool        last_chunk;
    
    _http_protocol_info_t()
    : http_type(NON_SPDY),
    stream_id(0),
    data_offset(0),
    data_size(0),
    last_chunk(false){}
    
} http_protocol_info;
    
enum spdy_frame_type{
    spdy_data_frame = 1,
    spdy_control_frame = 2,
    spdy_invalid_frame = 3
};


}   // end namespace spdy
}   // end namespace pion

#endif

