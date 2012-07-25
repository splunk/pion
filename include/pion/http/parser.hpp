// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTPPARSER_HEADER__
#define __PION_HTTPPARSER_HEADER__

#include <string>
#include <boost/noncopyable.hpp>
#include <boost/function/function2.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/system/error_code.hpp>
#include <boost/thread/once.hpp>
#include <pion/config.hpp>
#include <pion/logger.hpp>
#include <pion/http/message.hpp>


namespace pion {    // begin namespace pion
namespace net {     // begin namespace net (Pion Network Library)

// forward declarations used for finishing HTTP messages
class HTTPRequest;
class HTTPResponse;

///
/// HTTPParser: parses HTTP messages
///
class PION_NET_API HTTPParser :
    private boost::noncopyable
{

public:

    /// maximum length for HTTP payload content
    static const std::size_t        DEFAULT_CONTENT_MAX;

    /// callback type used to consume payload content
    typedef boost::function2<void, const char *, std::size_t>   PayloadHandler;
    
    /// class-specific error code values
    enum ErrorValue {
        ERROR_METHOD_CHAR = 1,
        ERROR_METHOD_SIZE,
        ERROR_URI_CHAR,
        ERROR_URI_SIZE,
        ERROR_QUERY_CHAR,
        ERROR_QUERY_SIZE,
        ERROR_VERSION_EMPTY,
        ERROR_VERSION_CHAR,
        ERROR_STATUS_EMPTY,
        ERROR_STATUS_CHAR,
        ERROR_HEADER_CHAR,
        ERROR_HEADER_NAME_SIZE,
        ERROR_HEADER_VALUE_SIZE,
        ERROR_INVALID_CONTENT_LENGTH,
        ERROR_CHUNK_CHAR,
        ERROR_MISSING_CHUNK_DATA,
        ERROR_MISSING_HEADER_DATA,
        ERROR_MISSING_TOO_MUCH_CONTENT,
    };
    
    /// class-specific error category
    class ErrorCategory
        : public boost::system::error_category
    {
    public:
        const char *name() const { return "HTTPParser"; }
        std::string message(int ev) const {
            switch (ev) {
            case ERROR_METHOD_CHAR:
                return "invalid method character";
            case ERROR_METHOD_SIZE:
                return "method exceeds maximum size";
            case ERROR_URI_CHAR:
                return "invalid URI character";
            case ERROR_URI_SIZE:
                return "method exceeds maximum size";
            case ERROR_QUERY_CHAR:
                return "invalid query string character";
            case ERROR_QUERY_SIZE:
                return "query string exceeds maximum size";
            case ERROR_VERSION_EMPTY:
                return "HTTP version undefined";
            case ERROR_VERSION_CHAR:
                return "invalid version character";
            case ERROR_STATUS_EMPTY:
                return "HTTP status undefined";
            case ERROR_STATUS_CHAR:
                return "invalid status character";
            case ERROR_HEADER_CHAR:
                return "invalid header character";
            case ERROR_HEADER_NAME_SIZE:
                return "header name exceeds maximum size";
            case ERROR_HEADER_VALUE_SIZE:
                return "header value exceeds maximum size";
            case ERROR_INVALID_CONTENT_LENGTH:
                return "invalid Content-Length header";
            case ERROR_CHUNK_CHAR:
                return "invalid chunk character";
            case ERROR_MISSING_HEADER_DATA:
                return "missing header data";
            case ERROR_MISSING_CHUNK_DATA:
                return "missing chunk data";
            case ERROR_MISSING_TOO_MUCH_CONTENT:
                return "missing too much content";
            }
            return "HTTPParser error";
        }
    };

    /**
     * creates new HTTPParser objects
     *
     * @param is_request if true, the message is parsed as an HTTP request;
     *                   if false, the message is parsed as an HTTP response
     * @param max_content_length maximum length for HTTP payload content
     */
    HTTPParser(const bool is_request, std::size_t max_content_length = DEFAULT_CONTENT_MAX)
        : m_logger(PION_GET_LOGGER("pion.net.HTTPParser")), m_is_request(is_request),
        m_read_ptr(NULL), m_read_end_ptr(NULL), m_message_parse_state(PARSE_START),
        m_headers_parse_state(is_request ? PARSE_METHOD_START : PARSE_HTTP_VERSION_H),
        m_chunked_content_parse_state(PARSE_CHUNK_SIZE_START), m_status_code(0),
        m_bytes_content_remaining(0), m_bytes_content_read(0),
        m_bytes_last_read(0), m_bytes_total_read(0),
        m_max_content_length(max_content_length),
        m_parse_headers_only(false), m_save_raw_headers(false)
    {}

    /// default destructor
    virtual ~HTTPParser() {}

    /**
     * parses an HTTP message including all payload content it might contain
     *
     * @param http_msg the HTTP message object to populate from parsing
     * @param ec error_code contains additional information for parsing errors
     *
     * @return boost::tribool result of parsing:
     *                        false = message has an error,
     *                        true = finished parsing HTTP message,
     *                        indeterminate = not yet finished parsing HTTP message
     */
    boost::tribool parse(HTTPMessage& http_msg, boost::system::error_code& ec);

    /**
     * attempts to continue parsing despite having missed data (length is known but content is not)
     *
     * @param http_msg the HTTP message object to populate from parsing
     * @param len the length in bytes of the missing data
     * @param ec error_code contains additional information for parsing errors
     *
     * @return boost::tribool result of parsing:
     *                        false = message has an error,
     *                        true = finished parsing HTTP message,
     *                        indeterminate = not yet finished parsing HTTP message
     */
    boost::tribool parseMissingData(HTTPMessage& http_msg, std::size_t len,
        boost::system::error_code& ec);

    /**
     * finishes parsing an HTTP response message
     *
     * @param http_msg the HTTP message object to finish
     */
    void finish(HTTPMessage& http_msg) const;

    /**
     * resets the location and size of the read buffer
     *
     * @param ptr pointer to the first bytes available to be read
     * @param len number of bytes available to be read
     */
    inline void setReadBuffer(const char *ptr, size_t len) {
        m_read_ptr = ptr;
        m_read_end_ptr = ptr + len;
    }

    /**
     * loads a read position bookmark
     *
     * @param read_ptr points to the next character to be consumed in the read_buffer
     * @param read_end_ptr points to the end of the read_buffer (last byte + 1)
     */
    inline void loadReadPosition(const char *&read_ptr, const char *&read_end_ptr) const {
        read_ptr = m_read_ptr;
        read_end_ptr = m_read_end_ptr;
    }

    /**
     * checks to see if a premature EOF was encountered while parsing.  This
     * should be called if there is no more data to parse, and if the last
     * call to the parse() function returned boost::indeterminate
     *
     * @param http_msg the HTTP message object being parsed
     * @return true if premature EOF, false if message is OK & finished parsing
     */
    inline bool checkPrematureEOF(HTTPMessage& http_msg) {
        if (m_message_parse_state != PARSE_CONTENT_NO_LENGTH)
            return true;
        m_message_parse_state = PARSE_END;
        http_msg.concatenateChunks();
        finish(http_msg);
        return false;
    }

    /**
     * controls headers-only parsing (default is disabled; content parsed also)
     *
     * @param b if true, then the parse() function returns true after headers
     */
    inline void parseHeadersOnly(bool b = true) { m_parse_headers_only = b; }

    /**
     * skip parsing all headers and parse payload content only
     *
     * @param http_msg the HTTP message object being parsed
     */
    inline void skipHeaderParsing(HTTPMessage& http_msg) {
        boost::system::error_code ec;
        finishHeaderParsing(http_msg, ec);
    }
    
    /// resets the parser to its initial state
    inline void reset(void) {
        m_message_parse_state = PARSE_START;
        m_headers_parse_state = (m_is_request ? PARSE_METHOD_START : PARSE_HTTP_VERSION_H);
        m_chunked_content_parse_state = PARSE_CHUNK_SIZE_START;
        m_status_code = 0;
        m_status_message.erase();
        m_method.erase();
        m_resource.erase();
        m_query_string.erase();
        m_raw_headers.erase();
        m_bytes_content_read = m_bytes_last_read = m_bytes_total_read = 0;
    }

    /// returns true if there are no more bytes available in the read buffer
    inline bool eof(void) const { return m_read_ptr == NULL || m_read_ptr >= m_read_end_ptr; }

    /// returns the number of bytes available in the read buffer
    inline std::size_t bytes_available(void) const { return (eof() ? 0 : (std::size_t)(m_read_end_ptr - m_read_ptr)); } 

    /// returns the number of bytes read during the last parse operation
    inline std::size_t gcount(void) const { return m_bytes_last_read; }

    /// returns the total number of bytes read while parsing the HTTP message
    inline std::size_t getTotalBytesRead(void) const { return m_bytes_total_read; }

    /// returns the total number of bytes read while parsing the payload content
    inline std::size_t getContentBytesRead(void) const { return m_bytes_content_read; }

    /// returns the maximum length for HTTP payload content
    inline std::size_t getMaxContentLength(void) const { return m_max_content_length; }

    /// returns the raw HTTP headers saved by the parser
    inline const std::string& getRawHeaders(void) const { return m_raw_headers; }

    /// returns true if the parser is saving raw HTTP header contents
    inline bool getSaveRawHeaders(void) const { return m_save_raw_headers; }

    /// returns true if the parser is being used to parse an HTTP request
    inline bool isParsingRequest(void) const { return m_is_request; }

    /// returns true if the parser is being used to parse an HTTP response
    inline bool isParsingResponse(void) const { return ! m_is_request; }

    /// defines a callback function to be used for consuming payload content
    inline void setPayloadHandler(PayloadHandler& h) { m_payload_handler = h; }

    /// sets the maximum length for HTTP payload content
    inline void setMaxContentLength(std::size_t n) { m_max_content_length = n; }

    /// resets the maximum length for HTTP payload content to the default value
    inline void resetMaxContentLength(void) { m_max_content_length = DEFAULT_CONTENT_MAX; }

    /// sets parameter for saving raw HTTP header content
    inline void setSaveRawHeaders(bool b) { m_save_raw_headers = b; }

    /// sets the logger to be used
    inline void setLogger(PionLogger log_ptr) { m_logger = log_ptr; }

    /// returns the logger currently in use
    inline PionLogger getLogger(void) { return m_logger; }


    /**
     * parse key-value pairs out of a url-encoded string
     * (i.e. this=that&a=value)
     * 
     * @param dict dictionary for key-values pairs
     * @param ptr points to the start of the encoded string
     * @param len length of the encoded string, in bytes
     * 
     * @return bool true if successful
     */
    static bool parseURLEncoded(HTTPTypes::QueryParams& dict,
                                const char *ptr, const std::size_t len);

    /**
     * parse key-value pairs out of a "Cookie" request header
     * (i.e. this=that; a=value)
     * 
     * @param dict dictionary for key-values pairs
     * @param ptr points to the start of the header string to be parsed
     * @param len length of the encoded string, in bytes
     * @param set_cookie_header set true if parsing Set-Cookie response header
     * 
     * @return bool true if successful
     */
    static bool parseCookieHeader(HTTPTypes::CookieParams& dict,
                                  const char *ptr, const std::size_t len,
                                  bool set_cookie_header);

    /**
     * parse key-value pairs out of a "Cookie" request header
     * (i.e. this=that; a=value)
     * 
     * @param dict dictionary for key-values pairs
     * @param cookie_header header string to be parsed
     * @param set_cookie_header set true if parsing Set-Cookie response header
     * 
     * @return bool true if successful
     */
    static inline bool parseCookieHeader(HTTPTypes::CookieParams& dict,
        const std::string& cookie_header, bool set_cookie_header)
    {
        return parseCookieHeader(dict, cookie_header.c_str(), cookie_header.size(), set_cookie_header);
    }

    /**
     * parse key-value pairs out of a url-encoded string
     * (i.e. this=that&a=value)
     * 
     * @param dict dictionary for key-values pairs
     * @param query the encoded query string to be parsed
     * 
     * @return bool true if successful
     */
    static inline bool parseURLEncoded(HTTPTypes::QueryParams& dict,
        const std::string& query)
    {
        return parseURLEncoded(dict, query.c_str(), query.size());
    }

    /**
     * parses an X-Forwarded-For HTTP header, and extracts from it an IP
     * address that best matches the client's public IP address (if any are found)
     *
     * @param header the X-Forwarded-For HTTP header to parse
     * @param public_ip the extract IP address, if found
     *
     * @return bool true if a public IP address was found and extracted
     */
    static bool parseForwardedFor(const std::string& header, std::string& public_ip);
    
    /// returns an instance of HTTPParser::ErrorCategory
    static inline ErrorCategory& getErrorCategory(void) {
        boost::call_once(HTTPParser::createErrorCategory, m_instance_flag);
        return *m_error_category_ptr;
    }


protected:

    /// Called after we have finished parsing the HTTP message headers
    virtual void finishedParsingHeaders(const boost::system::error_code& ec) {}
    
    /**
     * parses an HTTP message up to the end of the headers using bytes 
     * available in the read buffer
     *
     * @param http_msg the HTTP message object to populate from parsing
     * @param ec error_code contains additional information for parsing errors
     *
     * @return boost::tribool result of parsing:
     *                        false = message has an error,
     *                        true = finished parsing HTTP headers,
     *                        indeterminate = not yet finished parsing HTTP headers
     */
    boost::tribool parseHeaders(HTTPMessage& http_msg, boost::system::error_code& ec);

    /**
     * updates an HTTPMessage object with data obtained from parsing headers
     *
     * @param http_msg the HTTP message object to populate from parsing
     */
    void updateMessageWithHeaderData(HTTPMessage& http_msg) const;

    /**
     * should be called after parsing HTTP headers, to prepare for payload content parsing
     * available in the read buffer
     *
     * @param http_msg the HTTP message object to populate from parsing
     * @param ec error_code contains additional information for parsing errors
     *
     * @return boost::tribool result of parsing:
     *                        false = message has an error,
     *                        true = finished parsing HTTP message (no content),
     *                        indeterminate = payload content is available to be parsed
     */
    boost::tribool finishHeaderParsing(HTTPMessage& http_msg,
        boost::system::error_code& ec);

    /**
     * parses a chunked HTTP message-body using bytes available in the read buffer
     *
     * @param chunk_buffers buffers to be populated from parsing chunked content
     * @param ec error_code contains additional information for parsing errors
     *
     * @return boost::tribool result of parsing:
     *                        false = message has an error,
     *                        true = finished parsing message,
     *                        indeterminate = message is not yet finished
     */
    boost::tribool parseChunks(HTTPMessage::ChunkCache& chunk_buffers,
        boost::system::error_code& ec);

    /**
     * consumes payload content in the parser's read buffer 
     *
     * @param http_msg the HTTP message object to consume content for
     * @param ec error_code contains additional information for parsing errors
     *  
     * @return boost::tribool result of parsing:
     *                        false = message has an error,
     *                        true = finished parsing message,
     *                        indeterminate = message is not yet finished
     */
    boost::tribool consumeContent(HTTPMessage& http_msg,
        boost::system::error_code& ec);

    /**
     * consume the bytes available in the read buffer, converting them into
     * the next chunk for the HTTP message
     *
     * @param chunk_buffers buffers to be populated from parsing chunked content
     * @return std::size_t number of content bytes consumed, if any
     */
    std::size_t consumeContentAsNextChunk(HTTPMessage::ChunkCache& chunk_buffers);

    /**
     * compute and sets a HTTP Message data integrity status
     * @param http_msg target HTTP message 
     * @param msg_parsed_ok message parsing result
     */
    static void computeMsgStatus(HTTPMessage& http_msg, bool msg_parsed_ok);

    /**
     * sets an error code
     *
     * @param ec error code variable to define
     * @param ev error value to raise
     */
    static inline void setError(boost::system::error_code& ec, ErrorValue ev) {
        ec = boost::system::error_code(static_cast<int>(ev), getErrorCategory());
    }

    /// creates the unique HTTPParser ErrorCategory
    static void createErrorCategory(void);


    // misc functions used by the parsing functions
    inline static bool isChar(int c);
    inline static bool isControl(int c);
    inline static bool isSpecial(int c);
    inline static bool isDigit(int c);
    inline static bool isHexDigit(int c);
    inline static bool isCookieAttribute(const std::string& name, bool set_cookie_header);


    /// maximum length for response status message
    static const boost::uint32_t        STATUS_MESSAGE_MAX;

    /// maximum length for the request method
    static const boost::uint32_t        METHOD_MAX;

    /// maximum length for the resource requested
    static const boost::uint32_t        RESOURCE_MAX;

    /// maximum length for the query string
    static const boost::uint32_t        QUERY_STRING_MAX;

    /// maximum length for an HTTP header name
    static const boost::uint32_t        HEADER_NAME_MAX;

    /// maximum length for an HTTP header value
    static const boost::uint32_t        HEADER_VALUE_MAX;

    /// maximum length for the name of a query string variable
    static const boost::uint32_t        QUERY_NAME_MAX;

    /// maximum length for the value of a query string variable
    static const boost::uint32_t        QUERY_VALUE_MAX;

    /// maximum length for the name of a cookie name
    static const boost::uint32_t        COOKIE_NAME_MAX;

    /// maximum length for the value of a cookie; also used for path and domain
    static const boost::uint32_t        COOKIE_VALUE_MAX;


    /// primary logging interface used by this class
    mutable PionLogger                  m_logger;

    /// true if the message is an HTTP request; false if it is an HTTP response
    const bool                          m_is_request;

    /// points to the next character to be consumed in the read_buffer
    const char *                        m_read_ptr;

    /// points to the end of the read_buffer (last byte + 1)
    const char *                        m_read_end_ptr;


private:

    /// state used to keep track of where we are in parsing the HTTP message
    enum MessageParseState {
        PARSE_START, PARSE_HEADERS, PARSE_CONTENT,
        PARSE_CONTENT_NO_LENGTH, PARSE_CHUNKS, PARSE_END
    };

    /// state used to keep track of where we are in parsing the HTTP headers
    /// (only used if MessageParseState == PARSE_HEADERS)
    enum HeadersParseState {
        PARSE_METHOD_START, PARSE_METHOD, PARSE_URI_STEM, PARSE_URI_QUERY,
        PARSE_HTTP_VERSION_H, PARSE_HTTP_VERSION_T_1, PARSE_HTTP_VERSION_T_2,
        PARSE_HTTP_VERSION_P, PARSE_HTTP_VERSION_SLASH,
        PARSE_HTTP_VERSION_MAJOR_START, PARSE_HTTP_VERSION_MAJOR,
        PARSE_HTTP_VERSION_MINOR_START, PARSE_HTTP_VERSION_MINOR,
        PARSE_STATUS_CODE_START, PARSE_STATUS_CODE, PARSE_STATUS_MESSAGE,
        PARSE_EXPECTING_NEWLINE, PARSE_EXPECTING_CR,
        PARSE_HEADER_WHITESPACE, PARSE_HEADER_START, PARSE_HEADER_NAME,
        PARSE_SPACE_BEFORE_HEADER_VALUE, PARSE_HEADER_VALUE,
        PARSE_EXPECTING_FINAL_NEWLINE, PARSE_EXPECTING_FINAL_CR
    };

    /// state used to keep track of where we are in parsing chunked content
    /// (only used if MessageParseState == PARSE_CHUNKS)
    enum ChunkedContentParseState {
        PARSE_CHUNK_SIZE_START, PARSE_CHUNK_SIZE, 
        PARSE_EXPECTING_CR_AFTER_CHUNK_SIZE,
        PARSE_EXPECTING_LF_AFTER_CHUNK_SIZE, PARSE_CHUNK, 
        PARSE_EXPECTING_CR_AFTER_CHUNK, PARSE_EXPECTING_LF_AFTER_CHUNK,
        PARSE_EXPECTING_FINAL_CR_AFTER_LAST_CHUNK, 
        PARSE_EXPECTING_FINAL_LF_AFTER_LAST_CHUNK
    };


    /// the current state of parsing HTTP headers
    MessageParseState                   m_message_parse_state;

    /// the current state of parsing HTTP headers
    HeadersParseState                   m_headers_parse_state;

    /// the current state of parsing chunked content
    ChunkedContentParseState            m_chunked_content_parse_state;
    
    /// if defined, this function is used to consume payload content
    PayloadHandler                      m_payload_handler;

    /// Used for parsing the HTTP response status code
    boost::uint16_t                     m_status_code;

    /// Used for parsing the HTTP response status message
    std::string                         m_status_message;

    /// Used for parsing the request method
    std::string                         m_method;

    /// Used for parsing the name of resource requested
    std::string                         m_resource;

    /// Used for parsing the query string portion of a URI
    std::string                         m_query_string;

    /// Used to store the raw contents of HTTP headers when m_save_raw_headers is true
    std::string                         m_raw_headers;

    /// Used for parsing the name of HTTP headers
    std::string                         m_header_name;

    /// Used for parsing the value of HTTP headers
    std::string                         m_header_value;

    /// Used for parsing the chunk size
    std::string                         m_chunk_size_str;

    /// number of bytes in the chunk currently being parsed
    std::size_t                         m_size_of_current_chunk;

    /// number of bytes read so far in the chunk currently being parsed
    std::size_t                         m_bytes_read_in_current_chunk;

    /// number of payload content bytes that have not yet been read
    std::size_t                         m_bytes_content_remaining;

    /// number of bytes read so far into the message's payload content
    std::size_t                         m_bytes_content_read;

    /// number of bytes read during last parse operation
    std::size_t                         m_bytes_last_read;

    /// total number of bytes read while parsing the HTTP message
    std::size_t                         m_bytes_total_read;

    /// maximum length for HTTP payload content
    std::size_t                         m_max_content_length;
    
    /// if true, then only HTTP headers will be parsed (no content parsing)
    bool                                m_parse_headers_only;

    /// if true, the raw contents of HTTP headers are stored into m_raw_headers
    bool                                m_save_raw_headers;

    /// points to a single and unique instance of the HTTPParser ErrorCategory
    static ErrorCategory *              m_error_category_ptr;
        
    /// used to ensure thread safety of the HTTPParser ErrorCategory
    static boost::once_flag             m_instance_flag;
};


// inline functions for HTTPParser

inline bool HTTPParser::isChar(int c)
{
    return(c >= 0 && c <= 127);
}

inline bool HTTPParser::isControl(int c)
{
    return( (c >= 0 && c <= 31) || c == 127);
}

inline bool HTTPParser::isSpecial(int c)
{
    switch (c) {
    case '(': case ')': case '<': case '>': case '@':
    case ',': case ';': case ':': case '\\': case '"':
    case '/': case '[': case ']': case '?': case '=':
    case '{': case '}': case ' ': case '\t':
        return true;
    default:
        return false;
    }
}

inline bool HTTPParser::isDigit(int c)
{
    return(c >= '0' && c <= '9');
}

inline bool HTTPParser::isHexDigit(int c)
{
    return((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'));
}

inline bool HTTPParser::isCookieAttribute(const std::string& name, bool set_cookie_header)
{
    return (name.empty() || name[0] == '$' || (set_cookie_header &&
        (name=="Comment" || name=="Domain" || name=="Max-Age" || name=="Path" || name=="Secure" || name=="Version" || name=="Expires")
        ) );
}

}   // end namespace net
}   // end namespace pion

#endif
