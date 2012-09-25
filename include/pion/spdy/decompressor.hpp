// ------------------------------------------------------------------
// pion: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2012 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_SPDYDECOMPRESSOR_HEADER__
#define __PION_SPDYDECOMPRESSOR_HEADER__


#include <boost/shared_ptr.hpp>
#include <boost/system/error_code.hpp>
#include <boost/noncopyable.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/thread/once.hpp>
#include <pion/PionConfig.hpp>
#include <pion/PionLogger.hpp>
#include <pion/net/PionUser.hpp>
#include <pion/spdy/types.hpp>
#include <pion/spdy/parser.hpp>

#include <zlib.h>

namespace pion {	// begin namespace pion
namespace spdy {	// begin namespace spdy
    
// SPDY Dictionary used for zlib decompression

static const char spdy_dictionary[] =
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


///
/// SPDYDecompressor : Decompresses SPDY frames
///

class PION_NET_API decompressor
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
    decompressor(const char *compressed_data_ptr,
                 boost::system::error_code& ec);
    
    /// initializes the decompressor object
    void init_decompressor(boost::system::error_code &ec,
                           spdy_compression*& compression_data);
    
    /// destructor
    ~decompressor(){}
    
    /**
     * decompresses the http content
     *
     * @return the uncompressed string
     */
    char* decompress(boost::system::error_code& ec,
                     uint32_t stream_id,
                     spdy_control_frame_info frame,
                     int header_block_length,
                     spdy_compression*& compression_data);
    
    /**
     * decompresses the spdy header
     *
     * @return the uncompressed header
     */
    char* spdy_decompress_header(boost::system::error_code& ec,
                                 z_streamp decomp,
                                 uint32_t dictionary_id,
                                 uint32_t length,
                                 uint32_t *uncomp_length);
    
    /// creates the unique error category
    static void create_error_category(void);
    
    /// returns an instance of parser::error_category_t
    static inline error_category_t& get_error_category(void) {
        boost::call_once(parser::create_error_category, m_instance_flag);
        return *m_error_category_ptr;
    }

    
private:
    
    /**
     * sets an error code
     *
     * @param ec error code variable to define
     * @param ev error value to raise
     */
    static inline void set_error(boost::system::error_code& ec, error_value_t ev) {
        ec = boost::system::error_code(static_cast<int>(ev), get_error_category());
    }
    
    /// points to the next character to be consumed in the read_buffer
    const char *						m_compressed_data_ptr;
    
    /// primary logging interface used by this class
    mutable PionLogger					m_logger;
    
    /// points to a single and unique instance of the HTTPParser ErrorCategory
    static error_category_t *				m_error_category_ptr;
    
    /// used to ensure thread safety of the HTTPParser ErrorCategory
    static boost::once_flag				m_instance_flag;
    
};
        
}	// end namespace spdy
}	// end namespace pion

#endif