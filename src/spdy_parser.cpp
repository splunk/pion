// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <cstdlib>
#include <boost/regex.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/asio/detail/socket_ops.hpp>
#include <pion/spdy/parser.hpp>
#include <pion/spdy/decompressor.hpp>
#include <pion/spdy/types.hpp>


namespace pion {    // begin namespace pion
namespace spdy {    // begin namespace spdy


parser::error_category_t *  parser::m_error_category_ptr = NULL;
boost::once_flag            parser::m_instance_flag = BOOST_ONCE_INIT;

// parser member functions

parser::parser(const char *ptr, boost::system::error_code& ec)
    : m_read_ptr(ptr),
    m_uncompressed_ptr(NULL),
    m_current_data_chunk_ptr(ptr),
    m_last_data_chunk_ptr(NULL),
    m_logger(PION_GET_LOGGER("pion.spdy.parser"))
{
    BOOST_ASSERT(ptr);
}

bool parser::parse(SPDYStreamCompressor& compression_data,
                   http_protocol_info& http_info,
                   boost::system::error_code& ec,
                   uint32_t& length_packet,
                   uint32_t current_stream_count)
{
    // Parse the frame
    return parse_spdy_frame(ec, compression_data, http_info, length_packet, current_stream_count);
}

bool parser::is_spdy_frame(const char *ptr)
{
    // Determine if this a SPDY frame
    
    /*
     * The first byte of a SPDY frame must be either 0 or
     * 0x80. If it's not, assume that this is not SPDY.
     * (In theory, a data frame could have a stream ID
     * >= 2^24, in which case it won't have 0 for a first
     * byte, but this is a pretty reliable heuristic for
     * now.)
     */
    BOOST_ASSERT(ptr);
    
    const char *read_ptr = ptr;
    
    u_int8_t first_byte = (u_int8_t)*read_ptr;
    if (first_byte != 0x80 && first_byte != 0x0) {
        return false;
    }
    
    // Parse further for higher accuracy
    
    // Get the control bit
    uint8_t control_bit;
    uint version, type;
    uint16_t byte_value = int16_from_char(ptr);
    control_bit = byte_value >> (sizeof(short) * CHAR_BIT - 1);

    if(control_bit){
        
        // Control bit is set; This is a control frame
        
        // Get the version number
        uint16_t two_bytes = int16_from_char(ptr);
        version = two_bytes & 0x7FFF;
        
        if(version > 3){
            // SPDY does not have a version higher than 3 at the moment
            return false;
        }
        
        // Increment the read pointer
        ptr += 2;
        
        type = int16_from_char(ptr);
        
        if (type >= SPDY_INVALID) {
            // Not among the recognized SPDY types
            return false;
        }
    }else{
        // This is supposedly a data frame
        ptr +=4;
        
        // Get the flags
        uint16_t flags = (uint8_t)*ptr;
        // The valid falg values are till 1 or less
        if(flags > 1){
            return false;
        }
    }
    
    return true;
}


bool parser::parse_spdy_frame(boost::system::error_code& ec,
                              SPDYStreamCompressor& compression_data,
                              http_protocol_info& http_info,
                              uint32_t& length_packet,
                              uint32_t current_stream_count)
{
    
    BOOST_ASSERT(m_read_ptr);
    
    bool more_to_parse = false;
    boost::tribool rc = boost::indeterminate;
    
    // Verify that this is a spdy frame
    
    uint8_t first_byte = (uint8_t)*m_read_ptr;
    if (first_byte != 0x80 && first_byte != 0x0) {
        // This is not a SPDY frame, throw an error
        PION_LOG_ERROR(m_logger, "Invalid SPDY Frame");
        set_error(ec, ERROR_INVALID_SPDY_FRAME);
        return false;
    }
    
    uint8_t                 control_bit;
    spdy_control_frame_info frame;
    uint32_t                stream_id = 0;
    spdy_compression_ptr    compression_data_ptr;
    
    // Populate the frame
    populate_frame(ec, &frame, length_packet, stream_id, http_info);
    
    BOOST_ASSERT(stream_id != 0);
    
    control_bit = (uint8_t)frame.control_bit;
    
    // There is a possibility that there are more than one SPDY frames in one TCP frame
    if(length_packet > frame.length){
        m_current_data_chunk_ptr = m_read_ptr + frame.length;
        length_packet -= frame.length;
        more_to_parse = true;
    }
    
    if (!control_bit) {
        // Parse the data packet
        parse_spdy_data(ec, &frame, stream_id, http_info);
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
    
    // Get the compression data corresponding to the stream id
    for(SPDYStreamCompressor::iterator iter = compression_data.begin();
        iter != compression_data.end();
        ++iter){
        
        if(iter->first ==  stream_id){
            // Found the stream
            compression_data_ptr = iter->second;
            BOOST_ASSERT(compression_data_ptr);
        }
            
    }
    
    
    switch (frame.type) {
        case SPDY_SYN_STREAM:
        case SPDY_SYN_REPLY:
        case SPDY_HEADERS:
            parse_header_payload(ec, &frame, compression_data_ptr, http_info, current_stream_count);
            break;
            
        case SPDY_RST_STREAM:
            parse_spdy_rst_stream(ec, &frame);
            http_info.http_type = SPDY_CONTROL;
            break;
            
        case SPDY_SETTINGS:
            parse_spdy_settings_frame(ec, &frame);
            http_info.http_type = SPDY_CONTROL;
            break;
            
        case SPDY_PING:
            parse_spdy_ping_frame(ec, &frame);
            http_info.http_type = SPDY_CONTROL;
            break;
            
        case SPDY_GOAWAY:
            parse_spdy_goaway_frame(ec, &frame);
            http_info.http_type = SPDY_CONTROL;
            break;
            
        case SPDY_WINDOW_UPDATE:
            parse_spdy_window_update_frame(ec, &frame);
            http_info.http_type = SPDY_CONTROL;
            break;
            
        case SPDY_CREDENTIAL:
            // We dont need to parse this for now
            http_info.http_type = SPDY_CONTROL;
            break;
            
        default:
            break;
    }
    
    m_last_data_chunk_ptr = m_read_ptr;
    m_read_ptr = m_current_data_chunk_ptr;
    
    return more_to_parse;
}

void parser::create_error_category(void)
{
    static error_category_t UNIQUE_ERROR_CATEGORY;
    m_error_category_ptr = &UNIQUE_ERROR_CATEGORY;
}

void parser::populate_frame(boost::system::error_code& ec,
                            spdy_control_frame_info* frame,
                            uint32_t& length_packet,
                            uint32_t& stream_id,
                            http_protocol_info& http_info)
{
    // Get the control bit
    uint8_t control_bit;
    uint16_t byte_value = int16_from_char(m_read_ptr);
    control_bit = byte_value >> (sizeof(short) * CHAR_BIT - 1);
    
    frame->control_bit = (bool)control_bit;
    
    if(control_bit){
        
        // Control bit is set; This is a control frame
        
        // Get the version number
        uint16_t two_bytes = int16_from_char(m_read_ptr);
        frame->version = two_bytes & 0x7FFF;
        
        // Increment the read pointer
        m_read_ptr += 2;
        length_packet -= 2;
        http_info.data_offset +=2;
        
        // Get the type
        frame->type = int16_from_char(m_read_ptr);
        
        if (frame->type >= SPDY_INVALID) {
            // SPDY Frame is invalid
            
            // This is not a SPDY frame, throw an error
            PION_LOG_ERROR(m_logger, "Invalid SPDY Frame");
            set_error(ec, ERROR_INVALID_SPDY_FRAME);
            return;
        }
    }else {
        
        // Control bit is not set; This is a data frame
        
        frame->type = SPDY_DATA;
        frame->version = 0; /* Version doesn't apply to DATA. */
        // Get the stream id
        uint16_t four_bytes = int32_from_char(m_read_ptr);
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
    frame->flags = (uint8_t)*m_read_ptr;
    
    // Increment the read pointer
    
    // Get the length
    uint32_t four_bytes = int32_from_char(m_read_ptr);
    frame->length = four_bytes & 0xFFFFFF;
    
    // Increment the read pointer
    m_read_ptr += 4;
    length_packet -= 4;
    http_info.data_offset +=4;
    
    http_info.data_size = frame->length;
    
    if(control_bit){
        four_bytes = int32_from_char(m_read_ptr);
        stream_id = four_bytes & 0x7FFFFFFF;
    }
}

void parser::parse_header_payload(boost::system::error_code &ec,
                                  const spdy_control_frame_info* frame,
                                  spdy_compression_ptr& compression_data,
                                  http_protocol_info& http_info,
                                  uint32_t current_stream_count)
{
    uint32_t stream_id = 0;
    uint32_t associated_stream_id;
    int header_block_length = frame->length;
    
    // Get the 31 bit stream id
    
    uint16_t four_bytes = int32_from_char(m_read_ptr);
    stream_id = four_bytes & 0x7FFFFFFF;
    
    m_read_ptr += 4;
    
    http_info.stream_id = stream_id;
    
    // Get SYN_STREAM-only fields.
    
    if (frame->type == SPDY_SYN_STREAM) {
        
        // Get associated stream ID.
        
        uint16_t four_bytes = int32_from_char(m_read_ptr);
        associated_stream_id = four_bytes & 0x7FFFFFFF;
        
        m_read_ptr += 4;
        
        // The next bits are priority, unused, and slot.
        // Disregard these for now as we dont need them
        
        m_read_ptr +=2 ;
        
    } else if( frame->type == SPDY_SYN_REPLY || frame->type == SPDY_HEADERS ) {
        
        // Unused bits
        m_read_ptr +=2 ;
    }
    
    // Get our header block length.
    
    switch (frame->type) {
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
    
    decompressor spdy_decom(m_read_ptr,
                            ec);
    
    if(frame->type == SPDY_SYN_STREAM && compression_data != NULL){
        
        if(current_stream_count == 0)
            spdy_decom.init_decompressor(ec, compression_data);
        
    }
    
    m_uncompressed_ptr = spdy_decom.decompress(ec,
                                               stream_id,
                                               *frame,
                                               header_block_length,
                                               compression_data);
    
    
    if(m_uncompressed_ptr){
        
        // Now parse the name/value pairs
        
        // The number of name/value pairs is 16 bit SPDYv2
        // and it is 32 bit in SPDYv3
        
        // TBD : Add support for SPDYv3
        uint16_t num_name_val_pairs = int16_from_char(m_uncompressed_ptr);
        
        m_uncompressed_ptr += 2;
        
        std::string content_type = "";
        std::string content_encoding = "";
        
        for(uint16_t count = 0; count < num_name_val_pairs; ++count){
            
            
            // Get the length of the name
            uint16_t length_name = int16_from_char(m_uncompressed_ptr);
            std::string name = "";
            
            m_uncompressed_ptr += 2;
            
            {
                for(uint16_t count = 0; count < length_name; ++count){
                    name.push_back(*(m_uncompressed_ptr+count));
                }
                m_uncompressed_ptr += length_name;
            }
            
            // Get the length of the value
            uint16_t length_value = int16_from_char(m_uncompressed_ptr);
            std::string value = "";
            
            m_uncompressed_ptr += 2;
            
            {
                for(uint16_t count = 0; count < length_value; ++count){
                    value.push_back(*(m_uncompressed_ptr+count));
                }
                m_uncompressed_ptr += length_value;
            }
            
            // Save these headers
            http_info.http_headers.insert(std::make_pair(name, value));
        }
    }
}

void parser::parse_spdy_data(boost::system::error_code &ec,
                             const spdy_control_frame_info* frame,
                             uint32_t stream_id,
                             http_protocol_info& http_info)
{
    // This marks the finish flag
    if (frame->flags & SPDY_FLAG_FIN){
        http_info.last_chunk = true;
    }
}

void parser::parse_spdy_rst_stream(boost::system::error_code &ec,
                                   const spdy_control_frame_info* frame)
{
    uint32_t stream_id = 0;
    uint32_t status_code = 0;
    
    // Get the 31 bit stream id
    
    uint16_t four_bytes = int32_from_char(m_read_ptr);
    stream_id = four_bytes & 0x7FFFFFFF;
    
    m_read_ptr += 4;
    
    // Get the status code
    
    status_code = int32_from_char(m_read_ptr);
    
    PION_LOG_INFO(m_logger,
                  "SPDY " << "Status Code is : "
                  << rst_stream_status_names[status_code].str);
}

void parser::parse_spdy_ping_frame(boost::system::error_code &ec,
                                   const spdy_control_frame_info* frame)
{
    uint32_t ping_id = 0;
    
    // Get the 32 bit ping id
    
    ping_id = int32_from_char(m_read_ptr);
    
    m_read_ptr += 4;
    
    PION_LOG_INFO(m_logger, "SPDY " << "Ping ID is : " << ping_id);
}

void parser::parse_spdy_settings_frame(boost::system::error_code &ec,
                                       const spdy_control_frame_info* frame)
{
    // Can ignore this frame for our purposes
}

void parser::parse_spdy_goaway_frame(boost::system::error_code &ec,
                                     const spdy_control_frame_info* frame)
{
    uint32_t last_good_stream_id = 0;
    uint32_t status_code = 0;
    
    // Get the 31 bit stream id
    
    uint16_t four_bytes = int32_from_char(m_read_ptr);
    last_good_stream_id = four_bytes & 0x7FFFFFFF;
    
    m_read_ptr += 4;
    
    // Get the status code
    
    status_code = int32_from_char(m_read_ptr);
    
    // Chek if there was an error
    if(status_code == 1){
        
        PION_LOG_ERROR(m_logger, "There was a Protocol Error");
        set_error(ec, ERROR_PROTOCOL_ERROR);
        return;
    }else if (status_code == 11) {
        
        PION_LOG_ERROR(m_logger, "There was an Internal Error");
        set_error(ec, ERROR_INTERNAL_ERROR);
        return;
    }
    
    PION_LOG_INFO(m_logger, "SPDY " << "Status Code is : " << status_code);
    
}

void parser::parse_spdy_window_update_frame(boost::system::error_code &ec,
                                            const spdy_control_frame_info* frame)
{
    // TBD : Do we really need this for our purpose
}
    
}   // end namespace spdy
}   // end namespace pion
