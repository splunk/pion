// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_SPDYDECOMPRESSOR_HEADER__
#define __PION_SPDYDECOMPRESSOR_HEADER__


#include <boost/system/error_code.hpp>
#include <boost/noncopyable.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/thread/once.hpp>
#include <pion/config.hpp>
#include <pion/logger.hpp>
#include <pion/user.hpp>
#include <pion/spdy/types.hpp>
#include <zlib.h>


namespace pion {    // begin namespace pion
namespace spdy {    // begin namespace spdy


///
/// SPDYDecompressor : Decompresses SPDY frames
///

class PION_API decompressor
{
public:
    
    /// class-specific error code values
    enum error_value_t {
        ERROR_DECOMPRESSION_FAILED =1,
        ERROR_MISSING_HEADER_DATA
    };
    
    /// class-specific error category
    class error_category_t
        : public boost::system::error_category
    {
    public:
        const char *name() const { return "SPDYDecompressor"; }
        std::string message(int ev) const {
            switch (ev) {
                case ERROR_DECOMPRESSION_FAILED:
                    return "error in decompression";
                case ERROR_MISSING_HEADER_DATA:
                    return "missing header data";
                    
            }
            return "SPDYDecompressor error";
        }
    };
    
    /// constructs a new decompressor object (default constructor)
    decompressor();
    
    /// destructor
    ~decompressor();
    
    /**
     * decompresses the http content
     *
     * @return the uncompressed string
     */
    char* decompress(boost::system::error_code& ec,
                     const char *compressed_data_ptr,
                     uint32_t stream_id,
                     spdy_control_frame_info frame,
                     int header_block_length);
    
    /**
     * decompresses the spdy header
     *
     * @return the uncompressed header
     */
    void spdy_decompress_header(boost::system::error_code& ec,
                                const char *compressed_data_ptr,
                                z_streamp decomp,
                                uint32_t length,
                                uint32_t& uncomp_length);
    
    /// creates the unique error category
    static void create_error_category(void);
    
    /// returns an instance of parser::error_category_t
    static inline error_category_t& get_error_category(void) {
        boost::call_once(decompressor::create_error_category, m_instance_flag);
        return *m_error_category_ptr;
    }

    
private:
    
    enum data_size
    {
        max_uncompressed_data_buf_size = 16384
    };
    
    /**
     * sets an error code
     *
     * @param ec error code variable to define
     * @param ev error value to raise
     */
    static inline void set_error(boost::system::error_code& ec, error_value_t ev) {
        ec = boost::system::error_code(static_cast<int>(ev), get_error_category());
    }
    
    /// primary logging interface used by this class
    mutable logger                      m_logger;
    
    /// zlib stream for decompression request packets
    z_streamp                           m_request_zstream;

    /// zlib stream for decompression response packets
    z_streamp                           m_response_zstream;
    
    /// dictionary identifier
    uint32_t                            m_dictionary_id;
    
    /// Used for decompressing spdy headers
    u_char                              m_uncompressed_header[max_uncompressed_data_buf_size];

    /// points to a single and unique instance of the HTTPParser ErrorCategory
    static error_category_t *           m_error_category_ptr;
    
    /// used to ensure thread safety of the HTTPParser ErrorCategory
    static boost::once_flag             m_instance_flag;
    
    // SPDY Dictionary used for zlib decompression
    static const char                   SPDY_ZLIB_DICTIONARY[];
};
    
/// data type for a spdy reader pointer
typedef boost::shared_ptr<decompressor>       decompressor_ptr;
    
}   // end namespace spdy
}   // end namespace pion

#endif

