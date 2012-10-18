// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <cstdlib>
#include <zlib.h>
#include <boost/regex.hpp>
#include <boost/scoped_array.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/asio/detail/socket_ops.hpp>
#include <pion/spdy/decompressor.hpp>
#include <pion/spdy/types.hpp>

#include <iostream>
#include <fstream>

namespace pion {    // begin namespace pion
namespace spdy {    // begin namespace spdy 


// decompressor static members
    
decompressor::error_category_t *    decompressor::m_error_category_ptr = NULL;
boost::once_flag                    decompressor::m_instance_flag = BOOST_ONCE_INIT;
 
 const char decompressor::SPDY_ZLIB_DICTIONARY[] =
    "optionsgetheadpostputdeletetraceacceptaccept-charsetaccept-encodingaccept-"
    "languageauthorizationexpectfromhostif-modified-sinceif-matchif-none-matchi"
    "f-rangeif-unmodifiedsincemax-forwardsproxy-authorizationrangerefererteuser"
    "-agent10010120020120220320420520630030130230330430530630740040140240340440"
    "5406407408409410411412413414415416417500501502503504505accept-rangesageeta"
    "glocationproxy-authenticatepublicretry-afterservervarywarningwww-authentic"
    "ateallowcontent-basecontent-encodingcache-controlconnectiondatetrailertran"
    "sfer-encodingupgradeviawarningcontent-languagecontent-lengthcontent-locati"
    "oncontent-md5content-rangecontent-typeetagexpireslast-modifiedset-cookieMo"
    "ndayTuesdayWednesdayThursdayFridaySaturdaySundayJanFebMarAprMayJunJulAugSe"
    "pOctNovDecchunkedtext/htmlimage/pngimage/jpgimage/gifapplication/xmlapplic"
    "ation/xhtmltext/plainpublicmax-agecharset=iso-8859-1utf-8gzipdeflateHTTP/1"
    ".1statusversionurl";
    

// decompressor member functions

decompressor::decompressor()
    : m_logger(PION_GET_LOGGER("pion.spdy.decompressor")),
    m_request_zstream(NULL), m_response_zstream(NULL)
    
{
    m_request_zstream = (z_streamp)malloc(sizeof(z_stream));
    BOOST_ASSERT(m_request_zstream);

    m_request_zstream->zalloc = Z_NULL;
    m_request_zstream->zfree = Z_NULL;
    m_request_zstream->opaque = Z_NULL;
    m_request_zstream->next_in = Z_NULL;
    m_request_zstream->next_out = Z_NULL;
    m_request_zstream->avail_in = 0;
    m_request_zstream->avail_out = 0;
    
    m_response_zstream = (z_streamp)malloc(sizeof(z_stream));
    BOOST_ASSERT(m_response_zstream);
    
    m_response_zstream->zalloc = Z_NULL;
    m_response_zstream->zfree = Z_NULL;
    m_response_zstream->opaque = Z_NULL;
    m_response_zstream->next_in = Z_NULL;
    m_response_zstream->next_out = Z_NULL;
    m_response_zstream->avail_in = 0;
    m_response_zstream->avail_out = 0;
    
    int retcode = inflateInit2(m_request_zstream, MAX_WBITS);
    if (retcode == Z_OK) {
        retcode = inflateInit2(m_response_zstream, MAX_WBITS);
        if (retcode == Z_OK) {
            // Get the dictionary id
            m_dictionary_id = adler32(0L, Z_NULL, 0);
            
            m_dictionary_id = adler32(m_dictionary_id,
                                      (const Bytef *)SPDY_ZLIB_DICTIONARY,
                                      sizeof(SPDY_ZLIB_DICTIONARY));
        }
    }
}

decompressor::~decompressor()
{
    inflateEnd(m_request_zstream);
    inflateEnd(m_response_zstream);
    free(m_request_zstream);
    free(m_response_zstream);
}

char* decompressor::decompress(boost::system::error_code& ec,
                               const char *compressed_data_ptr,
                               uint32_t stream_id,
                               spdy_control_frame_info frame,
                               int header_block_length)
{
    /// Get our decompressor.
    z_streamp decomp = NULL;
    if (stream_id % 2 == 0) {
        // Even streams are server-initiated and should never get a
        // client-initiated header block. Use reply decompressor.
        decomp = m_response_zstream;
    } else if (frame.type == SPDY_HEADERS) {
        // Odd streams are client-initiated, but may have HEADERS from either
        // side. Currently, no known clients send HEADERS so we assume they are
        // all from the server.
        decomp = m_response_zstream;
    } else if (frame.type == SPDY_SYN_STREAM) {
        decomp = m_request_zstream;
    } else if (frame.type == SPDY_SYN_REPLY) {
        decomp = m_response_zstream;
    } else {
        // Unhandled case. This should never happen.
        BOOST_ASSERT(false);
    }
    BOOST_ASSERT(decomp);
    
    // Decompress the data
    uint32_t uncomp_length = 0;
    
    spdy_decompress_header(ec,
                           compressed_data_ptr,
                           decomp,
                           (uint32_t)header_block_length,
                           uncomp_length);
    
    // Catch decompression failures.

    if (m_uncompressed_header == NULL) {
        // Error in decompressing
        // This error is not catastrophic as many times we might get inconsistent
        // spdy header frames and we should just log error and continue.
        // No need to call SetError()
        PION_LOG_ERROR(m_logger, "Error in decompressing headers");
        
        return NULL;
    }
    return reinterpret_cast<char*>(m_uncompressed_header);
}

void decompressor::create_error_category(void)
{
    static error_category_t UNIQUE_ERROR_CATEGORY;
    m_error_category_ptr = &UNIQUE_ERROR_CATEGORY;
}

void decompressor::spdy_decompress_header(boost::system::error_code& ec,
                                          const char *compressed_data_ptr,
                                          z_streamp decomp,
                                          uint32_t length,
                                          uint32_t& uncomp_length) {
    int retcode;
    uint32_t bufsize = max_uncompressed_data_buf_size;
    const uint8_t *hptr = (uint8_t *)compressed_data_ptr;
    
    decomp->next_in = (Bytef *)hptr;
    decomp->avail_in = length;
    decomp->next_out = m_uncompressed_header;
    decomp->avail_out = bufsize;
    
    retcode = inflate(decomp, Z_SYNC_FLUSH);
    
    if (retcode == Z_NEED_DICT) {
        if (decomp->adler != m_dictionary_id) {
            // Decompressor wants a different dictionary id
        } else {
            retcode = inflateSetDictionary(decomp,
                                           (const Bytef *)SPDY_ZLIB_DICTIONARY,
                                           sizeof(SPDY_ZLIB_DICTIONARY));
            if (retcode == Z_OK) {
                retcode = inflate(decomp, Z_SYNC_FLUSH);
            }
        }
    }
    
    // Handle Errors. 
    if (retcode != Z_OK) {
        // This error is not catastrophic as many times we might get inconsistent
        // spdy header frames and we should just log error and continue.
        // No need to call SetError()
        PION_LOG_ERROR(m_logger, "Error in decompressing data");
        return;
    }
    
    // Handle successful inflation. 
    uncomp_length = bufsize - decomp->avail_out;
    if (decomp->avail_in != 0) {
        
        // Error condition
        // This error is not catastrophic as many times we might get inconsistent
        // spdy header frames and we should just log error and continue.
        // No need to call SetError()
        PION_LOG_ERROR(m_logger, "Error in decompressing data");
        return;
    }
}
        
}   // end namespace spdy
}   // end namespace pion
