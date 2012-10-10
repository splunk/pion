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
#include <boost/logic/tribool.hpp>
#include <boost/asio/detail/socket_ops.hpp>
#include <pion/spdy/decompressor.hpp>
#include <pion/spdy/types.hpp>



namespace pion {    // begin namespace pion
namespace spdy {    // begin namespace spdy 

    
// decompressor static members
    
decompressor::error_category_t *    decompressor::m_error_category_ptr = NULL;
boost::once_flag                    decompressor::m_instance_flag = BOOST_ONCE_INIT;
const boost::uint16_t               decompressor::MAX_UNCOMPRESSED_DATA_BUF_SIZE = 16384;
    
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

decompressor::decompressor(const char *compressed_data_ptr,
                           boost::system::error_code& ec)
    : m_compressed_data_ptr(compressed_data_ptr),
      m_logger(PION_GET_LOGGER("pion.spdy.decompressor"))
{
    BOOST_ASSERT(m_compressed_data_ptr);
}

void decompressor::init_decompressor(boost::system::error_code &ec,
                                     spdy_compression_ptr& compression_data)
{
    compression_data = (spdy_compression_ptr)new spdy_compression_t;
    
    int retcode = 0;
    
    compression_data->rqst_decompressor = (z_streamp)malloc(sizeof(z_stream));
    compression_data->rply_decompressor = (z_streamp)malloc(sizeof(z_stream));
    
    compression_data->rqst_decompressor->zalloc = Z_NULL;
    compression_data->rqst_decompressor->zfree = Z_NULL;
    compression_data->rqst_decompressor->opaque = Z_NULL;
    compression_data->rqst_decompressor->next_in = Z_NULL;
    compression_data->rqst_decompressor->next_out = Z_NULL;
    compression_data->rqst_decompressor->avail_in = 0;
    compression_data->rqst_decompressor->avail_out = 0;
    
    compression_data->rply_decompressor->zalloc = Z_NULL;
    compression_data->rply_decompressor->zfree = Z_NULL;
    compression_data->rply_decompressor->opaque = Z_NULL;
    compression_data->rply_decompressor->next_in = Z_NULL;
    compression_data->rply_decompressor->next_out = Z_NULL;
    compression_data->rply_decompressor->avail_in = 0;
    compression_data->rply_decompressor->avail_out = 0;
    
    retcode = inflateInit2(compression_data->rqst_decompressor, MAX_WBITS);
    if (retcode == Z_OK) {
        retcode = inflateInit2(compression_data->rply_decompressor, MAX_WBITS);
    }
    if (retcode != Z_OK) {
        // Error condition
        return;
    }
    
    // Get the dictionary id
    compression_data->dictionary_id = adler32(0L, Z_NULL, 0);
    
    compression_data->dictionary_id = adler32(compression_data->dictionary_id,
                                              (const Bytef *)SPDY_ZLIB_DICTIONARY,
                                              sizeof(SPDY_ZLIB_DICTIONARY));
}

char* decompressor::decompress(boost::system::error_code& ec,
                               uint32_t stream_id,
                               spdy_control_frame_info frame,
                               int header_block_length,
                               spdy_compression_ptr& compression_data)
{
  
    uint32_t uncomp_length = 0;
    z_streamp decomp;
    char *uncomp_ptr;
    
    /// Get our decompressor.
    if (stream_id % 2 == 0) {
        // Even streams are server-initiated and should never get a
        // client-initiated header block. Use reply decompressor.
        
        decomp = compression_data->rply_decompressor;
    } else if (frame.type == SPDY_HEADERS) {
        // Odd streams are client-initiated, but may have HEADERS from either
        // side. Currently, no known clients send HEADERS so we assume they are
        // all from the server.
        
        decomp = compression_data->rply_decompressor;
    } else if (frame.type == SPDY_SYN_STREAM) {
        
        decomp = compression_data->rqst_decompressor;
    } else if (frame.type == SPDY_SYN_REPLY) {
        
        decomp = compression_data->rply_decompressor;
    } else {
        
        // Unhandled case. This should never happen.
        BOOST_ASSERT(false);
    }
    // Decompress the data
    uncomp_ptr = spdy_decompress_header(ec,
                                        decomp,
                                        compression_data->dictionary_id,
                                        (uint32_t)header_block_length,
                                        &uncomp_length);
    
    // Catch decompression failures.
    
    if (uncomp_ptr == NULL) {
        // Error in decompressing
        // This error is not catastrophic as many times we might get inconsistent
        // spdy header frames and we should just log error and continue.
        // No need to call SetError()
        PION_LOG_ERROR(m_logger, "Error in decompressing headers");
        
        return NULL;
    }
    return uncomp_ptr;
}

void decompressor::create_error_category(void)
{
    static error_category_t UNIQUE_ERROR_CATEGORY;
    m_error_category_ptr = &UNIQUE_ERROR_CATEGORY;
}

char* decompressor::spdy_decompress_header(boost::system::error_code& ec,
                                           z_streamp decomp,
                                           uint32_t dictionary_id,
                                           uint32_t length,
                                           uint32_t *uncomp_length) {
    int retcode;
    size_t bufsize = MAX_UNCOMPRESSED_DATA_BUF_SIZE;
    
    const uint8_t *hptr = (uint8_t *)m_compressed_data_ptr;
    uint8_t *uncomp_block = (uint8_t*)malloc(bufsize);
    
    decomp->next_in = (Bytef *)hptr;
    decomp->avail_in = length;
    decomp->next_out = uncomp_block;
    decomp->avail_out = bufsize;
    
    retcode = inflate(decomp, Z_SYNC_FLUSH);
    
    if (retcode == Z_NEED_DICT) {
        if (decomp->adler != dictionary_id) {
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
        return NULL;
    }
    
    // Handle successful inflation. 
    *uncomp_length = bufsize - decomp->avail_out;
    if (decomp->avail_in != 0) {
        
        // Error condition
        // This error is not catastrophic as many times we might get inconsistent
        // spdy header frames and we should just log error and continue.
        // No need to call SetError()
        PION_LOG_ERROR(m_logger, "Error in decompressing data");
        return NULL;
    }
    
    return  (char *)uncomp_block;
}
        
}   // end namespace spdy
}   // end namespace pion
