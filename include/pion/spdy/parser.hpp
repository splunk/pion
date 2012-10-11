// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_SPDYPARSER_HEADER__
#define __PION_SPDYPARSER_HEADER__


#include <boost/shared_ptr.hpp>
#include <boost/system/error_code.hpp>
#include <boost/noncopyable.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/thread/once.hpp>
#include <pion/config.hpp>
#include <pion/logger.hpp>
#include <pion/user.hpp>
#include <pion/spdy/types.hpp>
#include <pion/spdy/decompressor.hpp>


namespace pion {    // begin namespace pion
namespace spdy {    // begin namespace spdy

    
///
/// parser : parsers and reads the SPDY frames
///

class PION_API parser
{
public:
    
    /// class-specific error code values
    enum error_value_t {
        ERROR_INVALID_SPDY_FRAME = 1,
        ERROR_INVALID_SPDY_VERSION,
        ERROR_DECOMPRESSION,
        ERROR_PROTOCOL_ERROR,
        ERROR_INTERNAL_ERROR,
        ERROR_MISSING_HEADER_DATA
    };
    
    /// class-specific error category
    class error_category_t
        : public boost::system::error_category
    {
    public:
        const char *name() const { return "SPDYParser"; }
        std::string message(int ev) const {
            switch (ev) {
                case ERROR_INVALID_SPDY_FRAME:
                    return "invalid spdy frame";
                case ERROR_INVALID_SPDY_VERSION:
                    return "invalid spdy version";
                case ERROR_DECOMPRESSION:
                    return "error in decompression";
                case ERROR_MISSING_HEADER_DATA:
                    return "missing header data";
                    
            }
            return "SPDYParser error";
        }
    };
    
    /// constructs a new parser object (default constructor)
    parser();
    
    /// destructor
    ~parser() {}
    
    /// parse the SPDY Frame
    bool parse(http_protocol_info& http_headers,
               boost::system::error_code& ec,
               const char *packet_ptr,
               uint32_t& length_packet,
               uint32_t current_stream_count);
    
    /**
     * checks if the frame is spdy frame or not
     *
     * @return true if it is a frame else returns false
     */
    static bool is_spdy_frame(const char *ptr);
    
    /**
     * checks if the frame is spdy control frame or not
     *
     * @return true if it is a control frame else returns false
     */
    static bool is_spdy_control_frame(const char *ptr);
    
    
protected:
    
    /// resets the read pointer
    inline void set_read_ptr(const char *ptr) { m_read_ptr = m_current_data_chunk_ptr = ptr; }
    
    /// populates the frame for every spdy packet
    void populate_frame(boost::system::error_code& ec,
                        spdy_control_frame_info* frame,
                        uint32_t& length_packet,
                        uint32_t& stream_id,
                        http_protocol_info& http_headers);
    
    /// creates the unique parser error_category_t
    static void create_error_category(void);
    
    /// returns an instance of parser::error_category_t
    static inline error_category_t& get_error_category(void) {
        boost::call_once(parser::create_error_category, m_instance_flag);
        return *m_error_category_ptr;
    }
    
    const char* get_uncompressed_http_data(){ return m_uncompressed_ptr; }
    
    /**
     * converts the char data to 16 bit int
     *
     * @param ptr of raw string
     */
    static uint16_t int16_from_char(const char* ptr){
        
        uint16_t i;
        uint16_t result;
        
        result = 0;
        for (i = 0; i < sizeof(uint16_t); ++i){
            uint8_t val = ptr[i];
            result = (result << CHAR_BIT) + val;
        }
        return result;
    }
    
    /**
     * converts the char data to 32 bit int
     *
     * @param ptr of raw string
     */
    static uint32_t int32_from_char(const char* ptr){
        
        uint32_t i = 0;
        uint32_t result;
        
        result = 0;
        for (i = 0; i < sizeof(uint32_t); ++i){
            uint8_t val = ptr[i];               // Only get the 8 bits
            result = (result << CHAR_BIT) + val;
        }
        return result;
    }
    
    /**
     * sets an error code
     *
     * @param ec error code variable to define
     * @param ev error value to raise
     */
    static inline void set_error(boost::system::error_code& ec, error_value_t ev) {
        ec = boost::system::error_code(static_cast<int>(ev), get_error_category());
    }
    
    /**
     * parses an the header payload for SPDY
     *
     */
    void parse_header_payload(boost::system::error_code& ec,
                              const spdy_control_frame_info* frame,
                              http_protocol_info& http_headers,
                              uint32_t current_stream_count);
    
    /**
     * parses the data for SPDY
     *
     */
    void parse_spdy_data(boost::system::error_code& ec,
                         const spdy_control_frame_info* frame,
                         uint32_t stream_id,
                         http_protocol_info& http_info);
    
    /**
     * parses an the Settings Frame for SPDY
     *
     */
    void parse_spdy_settings_frame(boost::system::error_code& ec,
                                   const spdy_control_frame_info* frame);
    
    /**
     * parses an the RST stream for SPDY
     *
     */
    void parse_spdy_rst_stream(boost::system::error_code& ec,
                               const spdy_control_frame_info* frame);
    
    /**
     * parses an the Ping Frame for SPDY
     *
     */
    void parse_spdy_ping_frame(boost::system::error_code& ec,
                               const spdy_control_frame_info* frame);
    
    /**
     * parses an the GoAway Frame for SPDY
     *
     */
    void parse_spdy_goaway_frame(boost::system::error_code& ec,
                                 const spdy_control_frame_info* frame);
    
    /**
     * parses an the WindowUpdate Frame for SPDY
     *
     */
    void parse_spdy_window_update_frame(boost::system::error_code& ec,
                                        const spdy_control_frame_info* frame);
    
    /**
     * parses an the entire SPDY frame
     *
     */
    bool parse_spdy_frame(boost::system::error_code& ec,
                          http_protocol_info& http_headers,
                          uint32_t& length_packet,
                          uint32_t current_stream_count);
    
    /// Get the pointer to the first character to the spdy data contect 
    const char * get_spdy_data_content( ) { return m_last_data_chunk_ptr; }
    
private:
    
    /// generic read pointer which parses the spdy data
    const char *                        m_read_ptr;
    
    /// points to the first character of the uncompressed http headers
    const char *                        m_uncompressed_ptr;
    
    /// SPDY has interleaved frames and this will point to start of the current chunk data
    const char *                        m_current_data_chunk_ptr;
    
    /// SPDY has interleaved frames and this will point to start of the the last chunk data
    const char *                        m_last_data_chunk_ptr;
    
    /// used to decompress the SPDY headers
    pion::spdy::decompressor            m_decompressor;
    
    /// primary logging interface used by this class
    mutable logger                      m_logger;
    
    /// points to a single and unique instance of the HTTPParser ErrorCategory
    static error_category_t *           m_error_category_ptr;
    
    /// used to ensure thread safety of the HTTPParser ErrorCategory
    static boost::once_flag             m_instance_flag;
};

/// data type for a spdy reader pointer
typedef boost::shared_ptr<parser>       parser_ptr;
        
        
}   // end namespace spdy
}   // end namespace pion

#endif