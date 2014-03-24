// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <cstdlib>
#include <boost/regex.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/asio/detail/socket_ops.hpp>
#include <pion/algorithm.hpp>
#include <pion/spdy/parser.hpp>
#include <pion/spdy/decompressor.hpp>
#include <pion/spdy/types.hpp>


namespace pion {    // begin namespace pion
namespace spdy {    // begin namespace spdy

/// RST stream status string from code, return NULL for unknown status code
static char const* rst_stream_status(boost::uint32_t rst_stream_status_code)
{
    switch (rst_stream_status_code)
    {
    case  1: return "PROTOCOL_ERROR";
    case  2: return "INVALID_STREAM";
    case  3: return "REFUSED_STREAM";
    case  4: return "UNSUPPORTED_VERSION";
    case  5: return "CANCEL";
    case  6: return "INTERNAL_ERROR";
    case  7: return "FLOW_CONTROL_ERROR";
    case  8: return "STREAM_IN_USE";
    case  9: return "STREAM_ALREADY_CLOSED";
    case 10: return "INVALID_CREDENTIALS";
    case 11: return "FRAME_TOO_LARGE";
    case 12: return "INVALID";
    default: return NULL;
    }
}

parser::error_category_t *  parser::m_error_category_ptr = NULL;
boost::once_flag            parser::m_instance_flag = BOOST_ONCE_INIT;

// parser member functions

parser::parser()
    : m_read_ptr(NULL),
    m_uncompressed_ptr(NULL),
    m_current_data_chunk_ptr(NULL),
    m_last_data_chunk_ptr(NULL),
    m_logger(PION_GET_LOGGER("pion.spdy.parser"))
{}

boost::tribool parser::parse(http_protocol_info& http_info,
                             boost::system::error_code& ec,
                             decompressor_ptr& decompressor,
                             const char *packet_ptr,
                             boost::uint32_t& length_packet,
                             boost::uint32_t current_stream_count)
{
    // initialize read position
    set_read_ptr(packet_ptr);
    
    // Parse the frame
    return parse_spdy_frame(ec, decompressor, http_info, length_packet, current_stream_count);
}

bool parser::is_spdy_control_frame(const char *ptr)
{
    // Parse further for higher accuracy
    
    // Get the control bit
    boost::uint8_t control_bit;
    boost::uint16_t version, type;
    boost::uint16_t byte_value = algorithm::to_uint16(ptr);
    control_bit = byte_value >> (sizeof(short) * CHAR_BIT - 1);

    if (!control_bit) return false;
    
    // Control bit is set; This is a control frame
    
    // Get the version number
    boost::uint16_t two_bytes = algorithm::to_uint16(ptr);
    version = two_bytes & 0x7FFF;
    
    if(version < 1 || version > 3){
        // SPDY does not have a version higher than 3 and lower than 1 at the moment
        return false;
    }
    
    // Increment the read pointer
    ptr += 2;
    
    type = algorithm::to_uint16(ptr);
    
    if (type >= SPDY_INVALID) {
        // Not among the recognized SPDY types
        return false;
    }
    
    return true;
}

spdy_frame_type parser::get_spdy_frame_type(const char *ptr)
{
    // Determine if this a SPDY frame
    BOOST_ASSERT(ptr);

    /*
     * The first byte of a SPDY frame must be either 0 or
     * 0x80. If it's not, assume that this is not SPDY.
     * (In theory, a data frame could have a stream ID
     * >= 2^24, in which case it won't have 0 for a first
     * byte, but this is a pretty reliable heuristic for
     * now.)
     */
    
    spdy_frame_type spdy_frame;
    boost::uint8_t first_byte = *((unsigned char *)ptr);
    if(first_byte == 0x80){
        spdy_frame = spdy_control_frame;
    }else if(first_byte == 0x0){
        spdy_frame = spdy_data_frame;
    }else{
        spdy_frame = spdy_invalid_frame;
    }
    return spdy_frame;
}
    
boost::uint32_t parser::get_control_frame_stream_id(const char *ptr)
{
    // The stream ID for control frames is at a 8 bit offser from start
    ptr += 8;

    boost::uint32_t four_bytes = algorithm::to_uint32(ptr);
    return four_bytes & 0x7FFFFFFF;
}
    
boost::tribool parser::parse_spdy_frame(boost::system::error_code& ec,
                                        decompressor_ptr& decompressor,
                                        http_protocol_info& http_info,
                                        boost::uint32_t& length_packet,
                                        boost::uint32_t current_stream_count)
{
    boost::tribool rc = true;
    
    // Verify that this is a spdy frame
    
    BOOST_ASSERT(m_read_ptr);
    boost::uint8_t first_byte = (boost::uint8_t)*m_read_ptr;
    if (first_byte != 0x80 && first_byte != 0x0) {
        // This is not a SPDY frame, throw an error
        PION_LOG_ERROR(m_logger, "Invalid SPDY Frame");
        set_error(ec, ERROR_INVALID_SPDY_FRAME);
        return false;
    }
    
    boost::uint8_t              control_bit;
    spdy_control_frame_info     frame;
    boost::uint32_t             stream_id = 0;
    
    ec.clear();

    // Populate the frame
    bool populate_frame_result = populate_frame(ec, frame, length_packet, stream_id, http_info);
    
    if(!populate_frame_result){
        /// There was an error; No need to further parse.
        return false;
    }
    
    BOOST_ASSERT(stream_id != 0);
    
    control_bit = (boost::uint8_t)frame.control_bit;
    
    // There is a possibility that there are more than one SPDY frames in one TCP frame
    if(length_packet > frame.length){
        m_current_data_chunk_ptr = m_read_ptr + frame.length;
        length_packet -= frame.length;
        rc = boost::indeterminate;
    }
    
    if (!control_bit) {
        // Parse the data packet
        parse_spdy_data(ec, frame, stream_id, http_info);
    }
    
    /* Abort here if the version is too low. */
    
    if (frame.version > MIN_SPDY_VERSION) {
        // Version less that min SPDY version, throw an error
        PION_LOG_ERROR(m_logger, "Invalid SPDY Version Number");
        set_error(ec, ERROR_INVALID_SPDY_VERSION);
        return false;
    }
    
    if(frame.type ==  SPDY_SYN_STREAM){
        http_info.http_type = HTTP_REQUEST;
    }else if (frame.type == SPDY_SYN_REPLY){
        http_info.http_type = HTTP_RESPONSE;
    }else if (frame.type == SPDY_DATA){
        http_info.http_type = HTTP_DATA;
    }

    switch (frame.type) {
        case SPDY_SYN_STREAM:
        case SPDY_SYN_REPLY:
        case SPDY_HEADERS:
            parse_header_payload(ec, decompressor, frame, http_info, current_stream_count);
            break;
            
        case SPDY_RST_STREAM:
            parse_spdy_rst_stream(ec, frame);
            http_info.http_type = SPDY_CONTROL;
            break;
            
        case SPDY_SETTINGS:
            parse_spdy_settings_frame(ec, frame);
            http_info.http_type = SPDY_CONTROL;
            break;
            
        case SPDY_PING:
            parse_spdy_ping_frame(ec, frame);
            http_info.http_type = SPDY_CONTROL;
            break;
            
        case SPDY_GOAWAY:
            parse_spdy_goaway_frame(ec, frame);
            http_info.http_type = SPDY_CONTROL;
            break;
            
        case SPDY_WINDOW_UPDATE:
            parse_spdy_window_update_frame(ec, frame);
            http_info.http_type = SPDY_CONTROL;
            break;
            
        case SPDY_CREDENTIAL:
            // We dont need to parse this for now
            http_info.http_type = SPDY_CONTROL;
            break;
            
        default:
            break;
    }
    
    if (ec)
        return false;
    
    m_last_data_chunk_ptr = m_read_ptr;
    m_read_ptr = m_current_data_chunk_ptr;
    
    return rc;
}

void parser::create_error_category(void)
{
    static error_category_t UNIQUE_ERROR_CATEGORY;
    m_error_category_ptr = &UNIQUE_ERROR_CATEGORY;
}

bool parser::populate_frame(boost::system::error_code& ec,
                            spdy_control_frame_info& frame,
                            boost::uint32_t& length_packet,
                            boost::uint32_t& stream_id,
                            http_protocol_info& http_info)
{
    // Get the control bit
    boost::uint8_t control_bit;
    boost::uint16_t byte_value = algorithm::to_uint16(m_read_ptr);
    control_bit = byte_value >> (sizeof(short) * CHAR_BIT - 1);
    
    frame.control_bit = (control_bit != 0);
    
    if(control_bit){
        
        // Control bit is set; This is a control frame
        
        // Get the version number
        boost::uint16_t two_bytes = algorithm::to_uint16(m_read_ptr);
        frame.version = two_bytes & 0x7FFF;
        
        // Increment the read pointer
        m_read_ptr += 2;
        length_packet -= 2;
        http_info.data_offset +=2;
        
        // Get the type
        frame.type = algorithm::to_uint16(m_read_ptr);
        
        if (frame.type >= SPDY_INVALID) {
            // SPDY Frame is invalid
            
            // This is not a SPDY frame, throw an error
            PION_LOG_ERROR(m_logger, "Invalid SPDY Frame");
            set_error(ec, ERROR_INVALID_SPDY_FRAME);
            return false;
        }
    }else {
        
        // Control bit is not set; This is a data frame
        
        frame.type = SPDY_DATA;
        frame.version = 0; /* Version doesn't apply to DATA. */
        // Get the stream id
        boost::uint32_t four_bytes = algorithm::to_uint32(m_read_ptr);
        stream_id = four_bytes & 0x7FFFFFFF;
        
        http_info.stream_id = stream_id;
        
        m_read_ptr +=2;
        http_info.data_offset +=2;
        length_packet -= 2;
        
    }
    
    // Increment the read pointer
    m_read_ptr += 2;
    length_packet -= 2;
    http_info.data_offset +=2;
    
    // Get the flags
    frame.flags = (boost::uint8_t)*m_read_ptr;
    
    // Increment the read pointer
    
    // Get the length
    boost::uint32_t four_bytes = algorithm::to_uint32(m_read_ptr);
    frame.length = four_bytes & 0xFFFFFF;
    
    // Increment the read pointer
    m_read_ptr += 4;
    length_packet -= 4;
    http_info.data_offset +=4;
    
    http_info.data_size = frame.length;
    
    if(control_bit){
        four_bytes = algorithm::to_uint32(m_read_ptr);
        stream_id = four_bytes & 0x7FFFFFFF;
    }
    
    return true;
}

void parser::parse_header_payload(boost::system::error_code &ec,
                                  decompressor_ptr& decompressor,
                                  const spdy_control_frame_info& frame,
                                  http_protocol_info& http_info,
                                  boost::uint32_t current_stream_count)
{
    boost::uint32_t stream_id = 0;
    boost::uint32_t associated_stream_id;
    boost::uint32_t header_block_length = frame.length;
    
    // Get the 31 bit stream id
    
    boost::uint32_t four_bytes = algorithm::to_uint32(m_read_ptr);
    stream_id = four_bytes & 0x7FFFFFFF;
    
    m_read_ptr += 4;
    
    http_info.stream_id = stream_id;
    
    // Get SYN_STREAM-only fields.
    
    if (frame.type == SPDY_SYN_STREAM) {
        
        // Get associated stream ID.
        
        boost::uint32_t four_bytes = algorithm::to_uint32(m_read_ptr);
        associated_stream_id = four_bytes & 0x7FFFFFFF;
        
        m_read_ptr += 4;
        
        // The next bits are priority, unused, and slot.
        // Disregard these for now as we dont need them
        
        m_read_ptr +=2 ;
        
    } else if( frame.type == SPDY_SYN_REPLY || frame.type == SPDY_HEADERS ) {
        
        // Unused bits
        m_read_ptr +=2 ;
    }
    
    // Get our header block length.
    
    switch (frame.type) {
        case SPDY_SYN_STREAM:
            header_block_length -= 10;
            break;
        case SPDY_SYN_REPLY:
        case SPDY_HEADERS:
            // This is a very important distinction.
            // It should be 6 bytes for SPDYv2 and 4 bytes for SPDYv3.
            header_block_length -= 6;
            break;
        default:
            // Unhandled case. This should never happen.
            PION_LOG_ERROR(m_logger, "Invalid SPDY Frame Type");
            set_error(ec, ERROR_INVALID_SPDY_FRAME);
            return;
    }
    
    // Decompress header block as necessary.
    m_uncompressed_ptr = decompressor->decompress(m_read_ptr,
                                                  stream_id,
                                                  frame,
                                                  header_block_length);
    
    if (!m_uncompressed_ptr) {
        set_error(ec, ERROR_DECOMPRESSION);
        return;
    }
        
    // Now parse the name/value pairs
    
    // The number of name/value pairs is 16 bit SPDYv2
    // and it is 32 bit in SPDYv3
    
    // TBD : Add support for SPDYv3
    boost::uint16_t num_name_val_pairs = algorithm::to_uint16(m_uncompressed_ptr);
    
    m_uncompressed_ptr += 2;
    
    std::string content_type = "";
    std::string content_encoding = "";
    
    for(boost::uint16_t count = 0; count < num_name_val_pairs; ++count){
        
        
        // Get the length of the name
        boost::uint16_t length_name = algorithm::to_uint16(m_uncompressed_ptr);
        std::string name = "";
        
        m_uncompressed_ptr += 2;
        
        {
            for(boost::uint16_t count = 0; count < length_name; ++count){
                name.push_back(*(m_uncompressed_ptr+count));
            }
            m_uncompressed_ptr += length_name;
        }
        
        // Get the length of the value
        boost::uint16_t length_value = algorithm::to_uint16(m_uncompressed_ptr);
        std::string value = "";
        
        m_uncompressed_ptr += 2;
        
        {
            for(boost::uint16_t count = 0; count < length_value; ++count){
                value.push_back(*(m_uncompressed_ptr+count));
            }
            m_uncompressed_ptr += length_value;
        }
        
        // Save these headers
        http_info.http_headers.insert(std::make_pair(name, value));
    }
}

void parser::parse_spdy_data(boost::system::error_code &ec,
                             const spdy_control_frame_info& frame,
                             boost::uint32_t stream_id,
                             http_protocol_info& http_info)
{
    // This marks the finish flag
    if (frame.flags & SPDY_FLAG_FIN){
        http_info.last_chunk = true;
    }
}

void parser::parse_spdy_rst_stream(boost::system::error_code &ec,
                                   const spdy_control_frame_info& frame)
{
    boost::uint32_t stream_id = 0;
    boost::uint32_t status_code = 0;
    
    // First complete the check for size and flag
    // The flag for RST frame should be 0, The length should be 8
    if(frame.flags != 0 || frame.length != 8 ){
        return;
    }

    // Get the 31 bit stream id
    
    boost::uint32_t four_bytes = algorithm::to_uint32(m_read_ptr);
    stream_id = four_bytes & 0x7FFFFFFF;
    
    m_read_ptr += 4;
    
    // Get the status code
    
    status_code = algorithm::to_uint32(m_read_ptr);
    
    char const* const status_code_str = rst_stream_status(status_code);
    if(status_code_str){
        PION_LOG_INFO(m_logger, "SPDY Status Code is : " << status_code_str);
    }else{
        PION_LOG_INFO(m_logger, "SPDY RST Invalid status code : " << status_code);
    }
}

void parser::parse_spdy_ping_frame(boost::system::error_code &ec,
                                   const spdy_control_frame_info& frame)
{
    // First complete the check for size 
    // The length should be 4 always
    if(frame.length != 4){
        return;
    }
  
    boost::uint32_t ping_id = 0;
    
    // Get the 32 bit ping id
    
    ping_id = algorithm::to_uint32(m_read_ptr);
    
    m_read_ptr += 4;
    
    PION_LOG_INFO(m_logger, "SPDY " << "Ping ID is : " << ping_id);
}

void parser::parse_spdy_settings_frame(boost::system::error_code &ec,
                                       const spdy_control_frame_info& frame)
{
    // Can ignore this frame for our purposes
}

void parser::parse_spdy_goaway_frame(boost::system::error_code &ec,
                                     const spdy_control_frame_info& frame)
{
    // First complete the check for size
    // The length should be 4 always
    if(frame.length != 4){
        return;
    }
    
    boost::uint32_t last_good_stream_id = 0;
    boost::uint32_t status_code = 0;
    
    // Get the 31 bit stream id
    
    boost::uint32_t four_bytes = algorithm::to_uint32(m_read_ptr);
    last_good_stream_id = four_bytes & 0x7FFFFFFF;
    
    m_read_ptr += 4;
    
    // Get the status code
    
    status_code = algorithm::to_uint32(m_read_ptr);
    
    // Chek if there was an error
    if(status_code == 1){
        
        PION_LOG_ERROR(m_logger, "There was a Protocol Error");
        set_error(ec, ERROR_PROTOCOL_ERROR);
        return;
    }else if (status_code == 11) {
        
        PION_LOG_ERROR(m_logger, "There was an Internal Error");
        set_error(ec, ERROR_INTERNAL_SPDY_ERROR);
        return;
    }
    
    PION_LOG_INFO(m_logger, "SPDY " << "Status Code is : " << status_code);
    
}

void parser::parse_spdy_window_update_frame(boost::system::error_code &ec,
                                            const spdy_control_frame_info& frame)
{
    // TBD : Do we really need this for our purpose
}
    
}   // end namespace spdy
}   // end namespace pion
