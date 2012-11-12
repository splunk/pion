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


#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <pion/config.hpp>
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

    /// data size constants
    enum data_size_t {
        /// maximum size of an uncompressed spdy header
        MAX_UNCOMPRESSED_DATA_BUF_SIZE = 16384
    };
    
    /// constructs a new decompressor object (default constructor)
    decompressor();
    
    /// destructor
    ~decompressor();
    
    /**
     * decompresses the http content
     *
     * @return the uncompressed string, or null on failure
     */
    char* decompress(const char *compressed_data_ptr,
                     boost::uint32_t stream_id,
                     const spdy_control_frame_info& frame,
                     boost::uint32_t header_block_length);

    
protected:

    /**
     * decompresses the spdy header
     *
     * @return true if successful
     */
    bool spdy_decompress_header(const char *compressed_data_ptr,
                                z_streamp decomp,
                                boost::uint32_t length,
                                boost::uint32_t& uncomp_length);


private:
    
    /// zlib stream for decompression request packets
    z_streamp                           m_request_zstream;

    /// zlib stream for decompression response packets
    z_streamp                           m_response_zstream;
    
    /// dictionary identifier
    boost::uint32_t                     m_dictionary_id;
    
    /// Used for decompressing spdy headers
    u_char                              m_uncompressed_header[MAX_UNCOMPRESSED_DATA_BUF_SIZE];

    // SPDY Dictionary used for zlib decompression
    static const char                   SPDY_ZLIB_DICTIONARY[];
};
    
/// data type for a spdy reader pointer
typedef boost::shared_ptr<decompressor>       decompressor_ptr;
    
}   // end namespace spdy
}   // end namespace pion

#endif

