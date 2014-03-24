// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
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

/// This structure will be tied to each SPDY frame
typedef struct spdy_control_frame_info{
    bool control_bit;
    boost::uint16_t  version;
    boost::uint16_t  type;
    boost::uint8_t   flags;
    boost::uint32_t  length;  // Actually only 24 bits.
} spdy_control_frame_info;

    
/// This structure will be tied to each SPDY header frame.
/// Only applies to frames containing headers: SYN_STREAM, SYN_REPLY, HEADERS
/// Note that there may be multiple SPDY frames in one packet.
typedef struct _spdy_header_info{
    boost::uint32_t stream_id;
    boost::uint8_t *header_block;
    boost::uint8_t  header_block_len;
    boost::uint16_t frame_type;
} spdy_header_info;


/// This structure contains the HTTP Protocol information
typedef struct _http_protocol_info_t{
    std::map<std::string, std::string> http_headers;
    boost::uint32_t     http_type;
    boost::uint32_t     stream_id;
    boost::uint32_t     data_offset;
    boost::uint32_t     data_size;
    bool                last_chunk;
    
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

