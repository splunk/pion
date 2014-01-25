// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <cstdlib>
#include <cstring>
#include <boost/regex.hpp>
#include <boost/assert.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/algorithm/string.hpp>
#include <pion/algorithm.hpp>
#include <pion/http/parser.hpp>
#include <pion/http/request.hpp>
#include <pion/http/response.hpp>
#include <pion/http/message.hpp>


namespace pion {    // begin namespace pion
namespace http {    // begin namespace http


// static members of parser

const boost::uint32_t   parser::STATUS_MESSAGE_MAX = 1024;  // 1 KB
const boost::uint32_t   parser::METHOD_MAX = 1024;  // 1 KB
const boost::uint32_t   parser::RESOURCE_MAX = 256 * 1024;  // 256 KB
const boost::uint32_t   parser::QUERY_STRING_MAX = 1024 * 1024; // 1 MB
const boost::uint32_t   parser::HEADER_NAME_MAX = 1024; // 1 KB
const boost::uint32_t   parser::HEADER_VALUE_MAX = 1024 * 1024; // 1 MB
const boost::uint32_t   parser::QUERY_NAME_MAX = 1024;  // 1 KB
const boost::uint32_t   parser::QUERY_VALUE_MAX = 1024 * 1024;  // 1 MB
const boost::uint32_t   parser::COOKIE_NAME_MAX = 1024; // 1 KB
const boost::uint32_t   parser::COOKIE_VALUE_MAX = 1024 * 1024; // 1 MB
const std::size_t       parser::DEFAULT_CONTENT_MAX = 1024 * 1024;  // 1 MB
parser::error_category_t * parser::m_error_category_ptr = NULL;
boost::once_flag            parser::m_instance_flag = BOOST_ONCE_INIT;


// parser member functions

boost::tribool parser::parse(http::message& http_msg,
    boost::system::error_code& ec)
{
    BOOST_ASSERT(! eof() );

    boost::tribool rc = boost::indeterminate;
    std::size_t total_bytes_parsed = 0;

    if(http_msg.has_missing_packets()) {
        http_msg.set_data_after_missing_packet(true);
    }

    do {
        switch (m_message_parse_state) {
            // just started parsing the HTTP message
            case PARSE_START:
                m_message_parse_state = PARSE_HEADERS;
                // step through to PARSE_HEADERS

            // parsing the HTTP headers
            case PARSE_HEADERS:
            case PARSE_FOOTERS:
                rc = parse_headers(http_msg, ec);
                total_bytes_parsed += m_bytes_last_read;
                // check if we have finished parsing HTTP headers
                if (rc == true && m_message_parse_state == PARSE_HEADERS) {
                    // finish_header_parsing() updates m_message_parse_state
                    // We only call this for Headers and not Footers
                    rc = finish_header_parsing(http_msg, ec);
                }
                break;

            // parsing chunked payload content
            case PARSE_CHUNKS:
                rc = parse_chunks(http_msg.get_chunk_cache(), ec);
                total_bytes_parsed += m_bytes_last_read;
                // check if we have finished parsing all chunks
                if (rc == true && !m_payload_handler) {
                    http_msg.concatenate_chunks();
                    
                    // Handle footers if present
                    rc = ((m_message_parse_state == PARSE_FOOTERS) ?
                          boost::indeterminate : (boost::tribool)true);
                }
                break;

            // parsing regular payload content with a known length
            case PARSE_CONTENT:
                rc = consume_content(http_msg, ec);
                total_bytes_parsed += m_bytes_last_read;
                break;

            // parsing payload content with no length (until EOF)
            case PARSE_CONTENT_NO_LENGTH:
                consume_content_as_next_chunk(http_msg.get_chunk_cache());
                total_bytes_parsed += m_bytes_last_read;
                break;

            // finished parsing the HTTP message
            case PARSE_END:
                rc = true;
                break;
        }
    } while ( boost::indeterminate(rc) && ! eof() );

    // check if we've finished parsing the HTTP message
    if (rc == true) {
        m_message_parse_state = PARSE_END;
        finish(http_msg);
    } else if(rc == false) {
        compute_msg_status(http_msg, false);
    }

    // update bytes last read (aggregate individual operations for caller)
    m_bytes_last_read = total_bytes_parsed;

    return rc;
}

boost::tribool parser::parse_missing_data(http::message& http_msg,
    std::size_t len, boost::system::error_code& ec)
{
    static const char MISSING_DATA_CHAR = 'X';
    boost::tribool rc = boost::indeterminate;

    http_msg.set_missing_packets(true);

    switch (m_message_parse_state) {

        // cannot recover from missing data while parsing HTTP headers
        case PARSE_START:
        case PARSE_HEADERS:
        case PARSE_FOOTERS:
            set_error(ec, ERROR_MISSING_HEADER_DATA);
            rc = false;
            break;

        // parsing chunked payload content
        case PARSE_CHUNKS:
            // parsing chunk data -> we can only recover if data fits into current chunk
            if (m_chunked_content_parse_state == PARSE_CHUNK
                && m_bytes_read_in_current_chunk < m_size_of_current_chunk
                && (m_size_of_current_chunk - m_bytes_read_in_current_chunk) >= len)
            {
                // use dummy content for missing data
                if (m_payload_handler) {
                    for (std::size_t n = 0; n < len; ++n)
                        m_payload_handler(&MISSING_DATA_CHAR, 1);
                } else {
                    for (std::size_t n = 0; n < len && http_msg.get_chunk_cache().size() < m_max_content_length; ++n) 
                        http_msg.get_chunk_cache().push_back(MISSING_DATA_CHAR);
                }

                m_bytes_read_in_current_chunk += len;
                m_bytes_last_read = len;
                m_bytes_total_read += len;
                m_bytes_content_read += len;

                if (m_bytes_read_in_current_chunk == m_size_of_current_chunk) {
                    m_chunked_content_parse_state = PARSE_EXPECTING_CR_AFTER_CHUNK;
                }
            } else {
                // cannot recover from missing data
                set_error(ec, ERROR_MISSING_CHUNK_DATA);
                rc = false;
            }
            break;

        // parsing regular payload content with a known length
        case PARSE_CONTENT:
            // parsing content (with length) -> we can only recover if data fits into content
            if (m_bytes_content_remaining == 0) {
                // we have all of the remaining payload content
                rc = true;
            } else if (m_bytes_content_remaining < len) {
                // cannot recover from missing data
                set_error(ec, ERROR_MISSING_TOO_MUCH_CONTENT);
                rc = false;
            } else {

                // make sure content buffer is not already full
                if (m_payload_handler) {
                    for (std::size_t n = 0; n < len; ++n)
                        m_payload_handler(&MISSING_DATA_CHAR, 1);
                } else if ( (m_bytes_content_read+len) <= m_max_content_length) {
                    // use dummy content for missing data
                    for (std::size_t n = 0; n < len; ++n)
                        http_msg.get_content()[m_bytes_content_read++] = MISSING_DATA_CHAR;
                } else {
                    m_bytes_content_read += len;
                }

                m_bytes_content_remaining -= len;
                m_bytes_total_read += len;
                m_bytes_last_read = len;

                if (m_bytes_content_remaining == 0)
                    rc = true;
            }
            break;

        // parsing payload content with no length (until EOF)
        case PARSE_CONTENT_NO_LENGTH:
            // use dummy content for missing data
            if (m_payload_handler) {
                for (std::size_t n = 0; n < len; ++n)
                    m_payload_handler(&MISSING_DATA_CHAR, 1);
            } else {
                for (std::size_t n = 0; n < len && http_msg.get_chunk_cache().size() < m_max_content_length; ++n) 
                    http_msg.get_chunk_cache().push_back(MISSING_DATA_CHAR);
            }
            m_bytes_last_read = len;
            m_bytes_total_read += len;
            m_bytes_content_read += len;
            break;

        // finished parsing the HTTP message
        case PARSE_END:
            rc = true;
            break;
    }

    // check if we've finished parsing the HTTP message
    if (rc == true) {
        m_message_parse_state = PARSE_END;
        finish(http_msg);
    } else if(rc == false) {
        compute_msg_status(http_msg, false);
    }

    return rc;
}

boost::tribool parser::parse_headers(http::message& http_msg,
    boost::system::error_code& ec)
{
    //
    // note that boost::tribool may have one of THREE states:
    //
    // false: encountered an error while parsing HTTP headers
    // true: finished successfully parsing the HTTP headers
    // indeterminate: parsed bytes, but the HTTP headers are not yet finished
    //
    const char *read_start_ptr = m_read_ptr;
    m_bytes_last_read = 0;
    while (m_read_ptr < m_read_end_ptr) {

        if (m_save_raw_headers)
            m_raw_headers += *m_read_ptr;
        
        switch (m_headers_parse_state) {
        case PARSE_METHOD_START:
            // we have not yet started parsing the HTTP method string
            if (*m_read_ptr != ' ' && *m_read_ptr!='\r' && *m_read_ptr!='\n') { // ignore leading whitespace
                if (!is_char(*m_read_ptr) || is_control(*m_read_ptr) || is_special(*m_read_ptr)) {
                    set_error(ec, ERROR_METHOD_CHAR);
                    return false;
                }
                m_headers_parse_state = PARSE_METHOD;
                m_method.erase();
                m_method.push_back(*m_read_ptr);
            }
            break;

        case PARSE_METHOD:
            // we have started parsing the HTTP method string
            if (*m_read_ptr == ' ') {
                m_resource.erase();
                m_headers_parse_state = PARSE_URI_STEM;
            } else if (!is_char(*m_read_ptr) || is_control(*m_read_ptr) || is_special(*m_read_ptr)) {
                set_error(ec, ERROR_METHOD_CHAR);
                return false;
            } else if (m_method.size() >= METHOD_MAX) {
                set_error(ec, ERROR_METHOD_SIZE);
                return false;
            } else {
                m_method.push_back(*m_read_ptr);
            }
            break;

        case PARSE_URI_STEM:
            // we have started parsing the URI stem (or resource name)
            if (*m_read_ptr == ' ') {
                m_headers_parse_state = PARSE_HTTP_VERSION_H;
            } else if (*m_read_ptr == '?') {
                m_query_string.erase();
                m_headers_parse_state = PARSE_URI_QUERY;
            } else if (*m_read_ptr == '\r') {
                http_msg.set_version_major(0);
                http_msg.set_version_minor(0);
                m_headers_parse_state = PARSE_EXPECTING_NEWLINE;
            } else if (*m_read_ptr == '\n') {
                http_msg.set_version_major(0);
                http_msg.set_version_minor(0);
                m_headers_parse_state = PARSE_EXPECTING_CR;
            } else if (is_control(*m_read_ptr)) {
                set_error(ec, ERROR_URI_CHAR);
                return false;
            } else if (m_resource.size() >= RESOURCE_MAX) {
                set_error(ec, ERROR_URI_SIZE);
                return false;
            } else {
                m_resource.push_back(*m_read_ptr);
            }
            break;

        case PARSE_URI_QUERY:
            // we have started parsing the URI query string
            if (*m_read_ptr == ' ') {
                m_headers_parse_state = PARSE_HTTP_VERSION_H;
            } else if (*m_read_ptr == '\r') {
                http_msg.set_version_major(0);
                http_msg.set_version_minor(0);
                m_headers_parse_state = PARSE_EXPECTING_NEWLINE;
            } else if (*m_read_ptr == '\n') {
                http_msg.set_version_major(0);
                http_msg.set_version_minor(0);
                m_headers_parse_state = PARSE_EXPECTING_CR;
            } else if (is_control(*m_read_ptr)) {
                set_error(ec, ERROR_QUERY_CHAR);
                return false;
            } else if (m_query_string.size() >= QUERY_STRING_MAX) {
                set_error(ec, ERROR_QUERY_SIZE);
                return false;
            } else {
                m_query_string.push_back(*m_read_ptr);
            }
            break;

        case PARSE_HTTP_VERSION_H:
            // parsing "HTTP"
            if (*m_read_ptr == '\r') {
                // should only happen for requests (no HTTP/VERSION specified)
                if (! m_is_request) {
                    set_error(ec, ERROR_VERSION_EMPTY);
                    return false;
                }
                http_msg.set_version_major(0);
                http_msg.set_version_minor(0);
                m_headers_parse_state = PARSE_EXPECTING_NEWLINE;
            } else if (*m_read_ptr == '\n') {
                // should only happen for requests (no HTTP/VERSION specified)
                if (! m_is_request) {
                    set_error(ec, ERROR_VERSION_EMPTY);
                    return false;
                }
                http_msg.set_version_major(0);
                http_msg.set_version_minor(0);
                m_headers_parse_state = PARSE_EXPECTING_CR;
            } else if (*m_read_ptr != 'H') {
                set_error(ec, ERROR_VERSION_CHAR);
                return false;
            }
            m_headers_parse_state = PARSE_HTTP_VERSION_T_1;
            break;

        case PARSE_HTTP_VERSION_T_1:
            // parsing "HTTP"
            if (*m_read_ptr != 'T') {
                set_error(ec, ERROR_VERSION_CHAR);
                return false;
            }
            m_headers_parse_state = PARSE_HTTP_VERSION_T_2;
            break;

        case PARSE_HTTP_VERSION_T_2:
            // parsing "HTTP"
            if (*m_read_ptr != 'T') {
                set_error(ec, ERROR_VERSION_CHAR);
                return false;
            }
            m_headers_parse_state = PARSE_HTTP_VERSION_P;
            break;

        case PARSE_HTTP_VERSION_P:
            // parsing "HTTP"
            if (*m_read_ptr != 'P') {
                set_error(ec, ERROR_VERSION_CHAR);
                return false;
            }
            m_headers_parse_state = PARSE_HTTP_VERSION_SLASH;
            break;

        case PARSE_HTTP_VERSION_SLASH:
            // parsing slash after "HTTP"
            if (*m_read_ptr != '/') {
                set_error(ec, ERROR_VERSION_CHAR);
                return false;
            }
            m_headers_parse_state = PARSE_HTTP_VERSION_MAJOR_START;
            break;

        case PARSE_HTTP_VERSION_MAJOR_START:
            // parsing the first digit of the major version number
            if (!is_digit(*m_read_ptr)) {
                set_error(ec, ERROR_VERSION_CHAR);
                return false;
            }
            http_msg.set_version_major(*m_read_ptr - '0');
            m_headers_parse_state = PARSE_HTTP_VERSION_MAJOR;
            break;

        case PARSE_HTTP_VERSION_MAJOR:
            // parsing the major version number (not first digit)
            if (*m_read_ptr == '.') {
                m_headers_parse_state = PARSE_HTTP_VERSION_MINOR_START;
            } else if (is_digit(*m_read_ptr)) {
                http_msg.set_version_major( (http_msg.get_version_major() * 10)
                                          + (*m_read_ptr - '0') );
            } else {
                set_error(ec, ERROR_VERSION_CHAR);
                return false;
            }
            break;

        case PARSE_HTTP_VERSION_MINOR_START:
            // parsing the first digit of the minor version number
            if (!is_digit(*m_read_ptr)) {
                set_error(ec, ERROR_VERSION_CHAR);
                return false;
            }
            http_msg.set_version_minor(*m_read_ptr - '0');
            m_headers_parse_state = PARSE_HTTP_VERSION_MINOR;
            break;

        case PARSE_HTTP_VERSION_MINOR:
            // parsing the major version number (not first digit)
            if (*m_read_ptr == ' ') {
                // ignore trailing spaces after version in request
                if (! m_is_request) {
                    m_headers_parse_state = PARSE_STATUS_CODE_START;
                }
            } else if (*m_read_ptr == '\r') {
                // should only happen for requests
                if (! m_is_request) {
                    set_error(ec, ERROR_STATUS_EMPTY);
                    return false;
                }
                m_headers_parse_state = PARSE_EXPECTING_NEWLINE;
            } else if (*m_read_ptr == '\n') {
                // should only happen for requests
                if (! m_is_request) {
                    set_error(ec, ERROR_STATUS_EMPTY);
                    return false;
                }
                m_headers_parse_state = PARSE_EXPECTING_CR;
            } else if (is_digit(*m_read_ptr)) {
                http_msg.set_version_minor( (http_msg.get_version_minor() * 10)
                                          + (*m_read_ptr - '0') );
            } else {
                set_error(ec, ERROR_VERSION_CHAR);
                return false;
            }
            break;

        case PARSE_STATUS_CODE_START:
            // parsing the first digit of the response status code
            if (!is_digit(*m_read_ptr)) {
                set_error(ec, ERROR_STATUS_CHAR);
                return false;
            }
            m_status_code = (*m_read_ptr - '0');
            m_headers_parse_state = PARSE_STATUS_CODE;
            break;

        case PARSE_STATUS_CODE:
            // parsing the response status code (not first digit)
            if (*m_read_ptr == ' ') {
                m_status_message.erase();
                m_headers_parse_state = PARSE_STATUS_MESSAGE;
            } else if (is_digit(*m_read_ptr)) {
                m_status_code = ( (m_status_code * 10) + (*m_read_ptr - '0') );
            } else if (*m_read_ptr == '\r') {
                // recover from status message not sent
                m_status_message.erase();
                m_headers_parse_state = PARSE_EXPECTING_NEWLINE;
            } else if (*m_read_ptr == '\n') {
                // recover from status message not sent
                m_status_message.erase();
                m_headers_parse_state = PARSE_EXPECTING_CR;
            } else {
                set_error(ec, ERROR_STATUS_CHAR);
                return false;
            }
            break;

        case PARSE_STATUS_MESSAGE:
            // parsing the response status message
            if (*m_read_ptr == '\r') {
                m_headers_parse_state = PARSE_EXPECTING_NEWLINE;
            } else if (*m_read_ptr == '\n') {
                m_headers_parse_state = PARSE_EXPECTING_CR;
            } else if (is_control(*m_read_ptr)) {
                set_error(ec, ERROR_STATUS_CHAR);
                return false;
            } else if (m_status_message.size() >= STATUS_MESSAGE_MAX) {
                set_error(ec, ERROR_STATUS_CHAR);
                return false;
            } else {
                m_status_message.push_back(*m_read_ptr);
            }
            break;

        case PARSE_EXPECTING_NEWLINE:
            // we received a CR; expecting a newline to follow
            if (*m_read_ptr == '\n') {
                // check if this is a HTTP 0.9 "Simple Request"
                if (m_is_request && http_msg.get_version_major() == 0) {
                    PION_LOG_DEBUG(m_logger, "HTTP 0.9 Simple-Request found");
                    ++m_read_ptr;
                    m_bytes_last_read = (m_read_ptr - read_start_ptr);
                    m_bytes_total_read += m_bytes_last_read;
                    return true;
                } else {
                    m_headers_parse_state = PARSE_HEADER_START;
                }
            } else if (*m_read_ptr == '\r') {
                // we received two CR's in a row
                // assume CR only is (incorrectly) being used for line termination
                // therefore, the message is finished
                ++m_read_ptr;
                m_bytes_last_read = (m_read_ptr - read_start_ptr);
                m_bytes_total_read += m_bytes_last_read;
                return true;
            } else if (*m_read_ptr == '\t' || *m_read_ptr == ' ') {
                m_headers_parse_state = PARSE_HEADER_WHITESPACE;
            } else if (!is_char(*m_read_ptr) || is_control(*m_read_ptr) || is_special(*m_read_ptr)) {
                set_error(ec, ERROR_HEADER_CHAR);
                return false;
            } else {
                // assume it is the first character for the name of a header
                m_header_name.erase();
                m_header_name.push_back(*m_read_ptr);
                m_headers_parse_state = PARSE_HEADER_NAME;
            }
            break;

        case PARSE_EXPECTING_CR:
            // we received a newline without a CR
            if (*m_read_ptr == '\r') {
                m_headers_parse_state = PARSE_HEADER_START;
            } else if (*m_read_ptr == '\n') {
                // we received two newlines in a row
                // assume newline only is (incorrectly) being used for line termination
                // therefore, the message is finished
                ++m_read_ptr;
                m_bytes_last_read = (m_read_ptr - read_start_ptr);
                m_bytes_total_read += m_bytes_last_read;
                return true;
            } else if (*m_read_ptr == '\t' || *m_read_ptr == ' ') {
                m_headers_parse_state = PARSE_HEADER_WHITESPACE;
            } else if (!is_char(*m_read_ptr) || is_control(*m_read_ptr) || is_special(*m_read_ptr)) {
                set_error(ec, ERROR_HEADER_CHAR);
                return false;
            } else {
                // assume it is the first character for the name of a header
                m_header_name.erase();
                m_header_name.push_back(*m_read_ptr);
                m_headers_parse_state = PARSE_HEADER_NAME;
            }
            break;

        case PARSE_HEADER_WHITESPACE:
            // parsing whitespace before a header name
            if (*m_read_ptr == '\r') {
                m_headers_parse_state = PARSE_EXPECTING_NEWLINE;
            } else if (*m_read_ptr == '\n') {
                m_headers_parse_state = PARSE_EXPECTING_CR;
            } else if (*m_read_ptr != '\t' && *m_read_ptr != ' ') {
                if (!is_char(*m_read_ptr) || is_control(*m_read_ptr) || is_special(*m_read_ptr)) {
                    set_error(ec, ERROR_HEADER_CHAR);
                    return false;
                }
                // assume it is the first character for the name of a header
                m_header_name.erase();
                m_header_name.push_back(*m_read_ptr);
                m_headers_parse_state = PARSE_HEADER_NAME;
            }
            break;

        case PARSE_HEADER_START:
            // parsing the start of a new header
            if (*m_read_ptr == '\r') {
                m_headers_parse_state = PARSE_EXPECTING_FINAL_NEWLINE;
            } else if (*m_read_ptr == '\n') {
                m_headers_parse_state = PARSE_EXPECTING_FINAL_CR;
            } else if (*m_read_ptr == '\t' || *m_read_ptr == ' ') {
                m_headers_parse_state = PARSE_HEADER_WHITESPACE;
            } else if (!is_char(*m_read_ptr) || is_control(*m_read_ptr) || is_special(*m_read_ptr)) {
                set_error(ec, ERROR_HEADER_CHAR);
                return false;
            } else {
                // first character for the name of a header
                m_header_name.erase();
                m_header_name.push_back(*m_read_ptr);
                m_headers_parse_state = PARSE_HEADER_NAME;
            }
            break;

        case PARSE_HEADER_NAME:
            // parsing the name of a header
            if (*m_read_ptr == ':') {
                m_header_value.erase();
                m_headers_parse_state = PARSE_SPACE_BEFORE_HEADER_VALUE;
            } else if (!is_char(*m_read_ptr) || is_control(*m_read_ptr) || is_special(*m_read_ptr)) {
                set_error(ec, ERROR_HEADER_CHAR);
                return false;
            } else if (m_header_name.size() >= HEADER_NAME_MAX) {
                set_error(ec, ERROR_HEADER_NAME_SIZE);
                return false;
            } else {
                // character (not first) for the name of a header
                m_header_name.push_back(*m_read_ptr);
            }
            break;

        case PARSE_SPACE_BEFORE_HEADER_VALUE:
            // parsing space character before a header's value
            if (*m_read_ptr == ' ') {
                m_headers_parse_state = PARSE_HEADER_VALUE;
            } else if (*m_read_ptr == '\r') {
                http_msg.add_header(m_header_name, m_header_value);
                m_headers_parse_state = PARSE_EXPECTING_NEWLINE;
            } else if (*m_read_ptr == '\n') {
                http_msg.add_header(m_header_name, m_header_value);
                m_headers_parse_state = PARSE_EXPECTING_CR;
            } else if (!is_char(*m_read_ptr) || is_control(*m_read_ptr) || is_special(*m_read_ptr)) {
                set_error(ec, ERROR_HEADER_CHAR);
                return false;
            } else {
                // assume it is the first character for the value of a header
                m_header_value.push_back(*m_read_ptr);
                m_headers_parse_state = PARSE_HEADER_VALUE;
            }
            break;

        case PARSE_HEADER_VALUE:
            // parsing the value of a header
            if (*m_read_ptr == '\r') {
                http_msg.add_header(m_header_name, m_header_value);
                m_headers_parse_state = PARSE_EXPECTING_NEWLINE;
            } else if (*m_read_ptr == '\n') {
                http_msg.add_header(m_header_name, m_header_value);
                m_headers_parse_state = PARSE_EXPECTING_CR;
            } else if (*m_read_ptr != '\t' && is_control(*m_read_ptr)) {
                // RFC 2616, 2.2 basic Rules.
                // TEXT = <any OCTET except CTLs, but including LWS>
                // LWS  = [CRLF] 1*( SP | HT )
                //
                // TODO: parsing of folding LWS in multiple lines headers
                //       doesn't work properly still
                set_error(ec, ERROR_HEADER_CHAR);
                return false;
            } else if (m_header_value.size() >= HEADER_VALUE_MAX) {
                set_error(ec, ERROR_HEADER_VALUE_SIZE);
                return false;
            } else {
                // character (not first) for the value of a header
                m_header_value.push_back(*m_read_ptr);
            }
            break;

        case PARSE_EXPECTING_FINAL_NEWLINE:
            if (*m_read_ptr == '\n') ++m_read_ptr;
            m_bytes_last_read = (m_read_ptr - read_start_ptr);
            m_bytes_total_read += m_bytes_last_read;
            return true;

        case PARSE_EXPECTING_FINAL_CR:
            if (*m_read_ptr == '\r') ++m_read_ptr;
            m_bytes_last_read = (m_read_ptr - read_start_ptr);
            m_bytes_total_read += m_bytes_last_read;
            return true;
        }
        
        ++m_read_ptr;
    }

    m_bytes_last_read = (m_read_ptr - read_start_ptr);
    m_bytes_total_read += m_bytes_last_read;
    return boost::indeterminate;
}

void parser::update_message_with_header_data(http::message& http_msg) const
{
    if (is_parsing_request()) {

        // finish an HTTP request message

        http::request& http_request(dynamic_cast<http::request&>(http_msg));
        http_request.set_method(m_method);
        http_request.set_resource(m_resource);
        http_request.set_query_string(m_query_string);

        // parse query pairs from the URI query string
        if (! m_query_string.empty()) {
            if (! parse_url_encoded(http_request.get_queries(),
                                  m_query_string.c_str(),
                                  m_query_string.size())) 
                PION_LOG_WARN(m_logger, "Request query string parsing failed (URI)");
        }

        // parse "Cookie" headers in request
        std::pair<ihash_multimap::const_iterator, ihash_multimap::const_iterator>
        cookie_pair = http_request.get_headers().equal_range(http::types::HEADER_COOKIE);
        for (ihash_multimap::const_iterator cookie_iterator = cookie_pair.first;
             cookie_iterator != http_request.get_headers().end()
             && cookie_iterator != cookie_pair.second; ++cookie_iterator)
        {
            if (! parse_cookie_header(http_request.get_cookies(),
                                    cookie_iterator->second, false) )
                PION_LOG_WARN(m_logger, "Cookie header parsing failed");
        }

    } else {

        // finish an HTTP response message

        http::response& http_response(dynamic_cast<http::response&>(http_msg));
        http_response.set_status_code(m_status_code);
        http_response.set_status_message(m_status_message);

        // parse "Set-Cookie" headers in response
        std::pair<ihash_multimap::const_iterator, ihash_multimap::const_iterator>
        cookie_pair = http_response.get_headers().equal_range(http::types::HEADER_SET_COOKIE);
        for (ihash_multimap::const_iterator cookie_iterator = cookie_pair.first;
             cookie_iterator != http_response.get_headers().end()
             && cookie_iterator != cookie_pair.second; ++cookie_iterator)
        {
            if (! parse_cookie_header(http_response.get_cookies(),
                                    cookie_iterator->second, true) )
                PION_LOG_WARN(m_logger, "Set-Cookie header parsing failed");
        }

    }
}

boost::tribool parser::finish_header_parsing(http::message& http_msg,
    boost::system::error_code& ec)
{
    boost::tribool rc = boost::indeterminate;

    m_bytes_content_remaining = m_bytes_content_read = 0;
    http_msg.set_content_length(0);
    http_msg.update_transfer_encoding_using_header();
    update_message_with_header_data(http_msg);

    if (http_msg.is_chunked()) {

        // content is encoded using chunks
        m_message_parse_state = PARSE_CHUNKS;
        
        // return true if parsing headers only
        if (m_parse_headers_only)
            rc = true;

    } else if (http_msg.is_content_length_implied()) {

        // content length is implied to be zero
        m_message_parse_state = PARSE_END;
        rc = true;

    } else {
        // content length should be specified in the headers

        if (http_msg.has_header(http::types::HEADER_CONTENT_LENGTH)) {

            // message has a content-length header
            try {
                http_msg.update_content_length_using_header();
            } catch (...) {
                PION_LOG_ERROR(m_logger, "Unable to update content length");
                set_error(ec, ERROR_INVALID_CONTENT_LENGTH);
                return false;
            }

            // check if content-length header == 0
            if (http_msg.get_content_length() == 0) {
                m_message_parse_state = PARSE_END;
                rc = true;
            } else {
                m_message_parse_state = PARSE_CONTENT;
                m_bytes_content_remaining = http_msg.get_content_length();

                // check if content-length exceeds maximum allowed
                if (m_bytes_content_remaining > m_max_content_length)
                    http_msg.set_content_length(m_max_content_length);

                if (m_parse_headers_only) {
                    // return true if parsing headers only
                    rc = true;
                } else {
                    // allocate a buffer for payload content (may be zero-size)
                    http_msg.create_content_buffer();
                }
            }

        } else {
            // no content-length specified, and the content length cannot 
            // otherwise be determined

            // only if not a request, read through the close of the connection
            if (! m_is_request) {
                // clear the chunk buffers before we start
                http_msg.get_chunk_cache().clear();

                // continue reading content until there is no more data
                m_message_parse_state = PARSE_CONTENT_NO_LENGTH;

                // return true if parsing headers only
                if (m_parse_headers_only)
                    rc = true;
            } else {
                m_message_parse_state = PARSE_END;
                rc = true;
            }
        }
    }

    finished_parsing_headers(ec);
    
    return rc;
}
    
bool parser::parse_uri(const std::string& uri, std::string& proto, 
                      std::string& host, boost::uint16_t& port,
                      std::string& path, std::string& query)
{
    size_t proto_end = uri.find("://");
    size_t proto_len = 0;
    
    if(proto_end != std::string::npos) {
        proto = uri.substr(0, proto_end);
        proto_len = proto_end + 3; // add ://
    } else {
        proto.clear();
    }
    
    // find a first slash charact
    // that indicates the end of the <server>:<port> part
    size_t server_port_end = uri.find('/', proto_len);
    if(server_port_end == std::string::npos) {
        return false;
    }
    
    // copy <server>:<port> into temp string
    std::string t; 
    t = uri.substr(proto_len, server_port_end - proto_len);
    size_t port_pos = t.find(':', 0);
    
    // assign output host and port parameters
    
    host = t.substr(0, port_pos); // if port_pos == npos, copy whole string
    if(host.length() == 0) {
        return false;
    }
    
    // parse the port, if it's not empty
    if(port_pos != std::string::npos) {
        try {
            port = boost::lexical_cast<int>(t.substr(port_pos+1));
        } catch (boost::bad_lexical_cast &) {
            return false;
        }
    } else if (proto == "http" || proto == "HTTP") {
        port = 80;
    } else if (proto == "https" || proto == "HTTPS") {
        port = 443;
    } else {
        port = 0;
    }
    
    // copy the rest of the URI into path part
    path = uri.substr(server_port_end);
    
    // split the path and the query string parts
    size_t query_pos = path.find('?', 0);
    
    if(query_pos != std::string::npos) {
        query = path.substr(query_pos + 1, path.length() - query_pos - 1);
        path = path.substr(0, query_pos);
    } else {
        query.clear();
    }
    
    return true;
}

bool parser::parse_url_encoded(ihash_multimap& dict,
                               const char *ptr, const size_t len)
{
    // sanity check
    if (ptr == NULL || len == 0)
        return true;

    // used to track whether we are parsing the name or value
    enum QueryParseState {
        QUERY_PARSE_NAME, QUERY_PARSE_VALUE
    } parse_state = QUERY_PARSE_NAME;

    // misc other variables used for parsing
    const char * const end = ptr + len;
    std::string query_name;
    std::string query_value;

    // iterate through each encoded character
    while (ptr < end) {
        switch (parse_state) {

        case QUERY_PARSE_NAME:
            // parsing query name
            if (*ptr == '=') {
                // end of name found (OK if empty)
                parse_state = QUERY_PARSE_VALUE;
            } else if (*ptr == '&') {
                // if query name is empty, just skip it (i.e. "&&")
                if (! query_name.empty()) {
                    // assume that "=" is missing -- it's OK if the value is empty
                    dict.insert( std::make_pair(algorithm::url_decode(query_name), algorithm::url_decode(query_value)) );
                    query_name.erase();
                }
            } else if (*ptr == '\r' || *ptr == '\n' || *ptr == '\t') {
                // ignore linefeeds, carriage return and tabs (normally within POST content)
            } else if (is_control(*ptr) || query_name.size() >= QUERY_NAME_MAX) {
                // control character detected, or max sized exceeded
                return false;
            } else {
                // character is part of the name
                query_name.push_back(*ptr);
            }
            break;

        case QUERY_PARSE_VALUE:
            // parsing query value
            if (*ptr == '&') {
                // end of value found (OK if empty)
                if (! query_name.empty()) {
                    dict.insert( std::make_pair(algorithm::url_decode(query_name), algorithm::url_decode(query_value)) );
                    query_name.erase();
                }
                query_value.erase();
                parse_state = QUERY_PARSE_NAME;
            } else if (*ptr == ',') {
                // end of value found in multi-value list (OK if empty)
                if (! query_name.empty())
                    dict.insert( std::make_pair(algorithm::url_decode(query_name), algorithm::url_decode(query_value)) );
                query_value.erase();
            } else if (*ptr == '\r' || *ptr == '\n' || *ptr == '\t') {
                // ignore linefeeds, carriage return and tabs (normally within POST content)
            } else if (is_control(*ptr) || query_value.size() >= QUERY_VALUE_MAX) {
                // control character detected, or max sized exceeded
                return false;
            } else {
                // character is part of the value
                query_value.push_back(*ptr);
            }
            break;
        }

        ++ptr;
    }

    // handle last pair in string
    if (! query_name.empty())
        dict.insert( std::make_pair(algorithm::url_decode(query_name), algorithm::url_decode(query_value)) );

    return true;
}

bool parser::parse_multipart_form_data(ihash_multimap& dict,
                                       const std::string& content_type,
                                       const char *ptr, const size_t len)
{
    // sanity check
    if (ptr == NULL || len == 0)
        return true;
    
    // parse field boundary
    std::size_t pos = content_type.find("boundary=");
    if (pos == std::string::npos)
        return false;
    const std::string boundary = std::string("--") + content_type.substr(pos+9);
    
    // used to track what we are parsing
    enum MultiPartParseState {
        MP_PARSE_START,
        MP_PARSE_HEADER_CR, MP_PARSE_HEADER_LF,
        MP_PARSE_HEADER_NAME, MP_PARSE_HEADER_SPACE, MP_PARSE_HEADER_VALUE,
        MP_PARSE_HEADER_LAST_LF, MP_PARSE_FIELD_DATA
    } parse_state = MP_PARSE_START;

    // a few variables used for parsing
    std::string header_name;
    std::string header_value;
    std::string field_name;
    std::string field_value;
    bool found_parameter = false;
    bool save_current_field = true;
    const char * const end_ptr = ptr + len;

    ptr = std::search(ptr, end_ptr, boundary.begin(), boundary.end());

    while (ptr != NULL && ptr < end_ptr) {
        switch (parse_state) {
            case MP_PARSE_START:
                // start parsing a new field
                header_name.clear();
                header_value.clear();
                field_name.clear();
                field_value.clear();
                save_current_field = true;
                ptr += boundary.size() - 1;
                parse_state = MP_PARSE_HEADER_CR;
                break;
            case MP_PARSE_HEADER_CR:
                // expecting CR while parsing headers
                if (*ptr == '\r') {
                    // got it -> look for linefeed
                    parse_state = MP_PARSE_HEADER_LF;
                } else if (*ptr == '\n') {
                    // got a linefeed? try to ignore and start parsing header
                    parse_state = MP_PARSE_HEADER_NAME;
                } else if (*ptr == '-' && ptr+1 < end_ptr && ptr[1] == '-') {
                    // end of multipart content
                    return true;
                } else return false;
                break;
            case MP_PARSE_HEADER_LF:
                // expecting LF while parsing headers
                if (*ptr == '\n') {
                    // got it -> start parsing header name
                    parse_state = MP_PARSE_HEADER_NAME;
                } else return false;
                break;
            case MP_PARSE_HEADER_NAME:
                // parsing the name of a header
                if (*ptr == '\r' || *ptr == '\n') {
                    if (header_name.empty()) {
                        // got CR or LF at beginning; skip to data
                        parse_state = (*ptr == '\r' ? MP_PARSE_HEADER_LAST_LF : MP_PARSE_FIELD_DATA);
                    } else {
                        // premature CR or LF -> just ignore and start parsing next header
                        parse_state = (*ptr == '\r' ? MP_PARSE_HEADER_LF : MP_PARSE_HEADER_NAME);
                    }
                } else if (*ptr == ':') {
                    // done parsing header name -> consume space next
                    parse_state = MP_PARSE_HEADER_SPACE;
                } else {
                    // one more byte for header name
                    header_name += *ptr;
                }
                break;
            case MP_PARSE_HEADER_SPACE:
                // expecting a space before header value
                if (*ptr == '\r') {
                    // premature CR -> just ignore and start parsing next header
                    parse_state = MP_PARSE_HEADER_LF;
                } else if (*ptr == '\n') {
                    // premature LF -> just ignore and start parsing next header
                    parse_state = MP_PARSE_HEADER_NAME;
                } else if (*ptr != ' ') {
                    // not a space -> assume it's a value char
                    header_value += *ptr;
                    parse_state = MP_PARSE_HEADER_VALUE;
                }
                // otherwise just ignore the space(s)
                break;
            case MP_PARSE_HEADER_VALUE:
                // parsing the value of a header
                if (*ptr == '\r' || *ptr == '\n') {
                    // reached the end of the value -> check if it's important
                    if (boost::algorithm::iequals(header_name, types::HEADER_CONTENT_TYPE)) {
                        // only keep fields that have a text type or no type
                        save_current_field = boost::algorithm::iequals(header_value.substr(0, 5), "text/");
                    } else if (boost::algorithm::iequals(header_name, types::HEADER_CONTENT_DISPOSITION)) {
                        // get current field from content-disposition header
                        std::size_t name_pos = header_value.find("name=\"");
                        if (name_pos != std::string::npos) {
                            for (name_pos += 6; name_pos < header_value.size() && header_value[name_pos] != '\"'; ++name_pos) {
                                field_name += header_value[name_pos];
                            }
                        }
                    }
                    // clear values and start parsing next header
                    header_name.clear();
                    header_value.clear();
                    parse_state = (*ptr == '\r' ? MP_PARSE_HEADER_LF : MP_PARSE_HEADER_NAME);
                } else {
                    // one more byte for header value
                    header_value += *ptr;
                }
                break;
            case MP_PARSE_HEADER_LAST_LF:
                // expecting final linefeed to terminate headers and begin field data
                if (*ptr == '\n') {
                    // got it
                    if (save_current_field && !field_name.empty()) {
                        // parse the field if we care & know enough about it
                        parse_state = MP_PARSE_FIELD_DATA;
                    } else {
                        // otherwise skip ahead to next field
                        parse_state = MP_PARSE_START;
                        ptr = std::search(ptr, end_ptr, boundary.begin(), boundary.end());
                    }
                } else return false;
                break;
            case MP_PARSE_FIELD_DATA:
                // parsing the value of a field -> find the end of it
                const char *field_end_ptr = end_ptr;
                const char *next_ptr = std::search(ptr, end_ptr, boundary.begin(), boundary.end());
                if (next_ptr) {
                    // don't include CRLF before next boundary
                    const char *temp_ptr = next_ptr - 2;
                    if (temp_ptr[0] == '\r' && temp_ptr[1] == '\n')
                        field_end_ptr = temp_ptr;
                    else field_end_ptr = next_ptr;
                }
                field_value.assign(ptr, field_end_ptr - ptr);
                // add the field to the query dictionary
                dict.insert( std::make_pair(field_name, field_value) );
                found_parameter = true;
                // skip ahead to next field
                parse_state = MP_PARSE_START;
                ptr = next_ptr;
                break;
        }
        // we've already bumped position if MP_PARSE_START
        if (parse_state != MP_PARSE_START)
            ++ptr;
    }
    
    return found_parameter;
}

bool parser::parse_cookie_header(ihash_multimap& dict,
                                   const char *ptr, const size_t len,
                                   bool set_cookie_header)
{
    // BASED ON RFC 2109
    // http://www.ietf.org/rfc/rfc2109.txt
    // 
    // The current implementation ignores cookie attributes which begin with '$'
    // (i.e. $Path=/, $Domain=, etc.)

    // used to track what we are parsing
    enum CookieParseState {
        COOKIE_PARSE_NAME, COOKIE_PARSE_VALUE, COOKIE_PARSE_IGNORE
    } parse_state = COOKIE_PARSE_NAME;

    // misc other variables used for parsing
    const char * const end = ptr + len;
    std::string cookie_name;
    std::string cookie_value;
    char value_quote_character = '\0';

    // iterate through each character
    while (ptr < end) {
        switch (parse_state) {

        case COOKIE_PARSE_NAME:
            // parsing cookie name
            if (*ptr == '=') {
                // end of name found (OK if empty)
                value_quote_character = '\0';
                parse_state = COOKIE_PARSE_VALUE;
            } else if (*ptr == ';' || *ptr == ',') {
                // ignore empty cookie names since this may occur naturally
                // when quoted values are encountered
                if (! cookie_name.empty()) {
                    // value is empty (OK)
                    if (! is_cookie_attribute(cookie_name, set_cookie_header))
                        dict.insert( std::make_pair(cookie_name, cookie_value) );
                    cookie_name.erase();
                }
            } else if (*ptr != ' ') {   // ignore whitespace
                // check if control character detected, or max sized exceeded
                if (is_control(*ptr) || cookie_name.size() >= COOKIE_NAME_MAX)
                    return false;
                // character is part of the name
                cookie_name.push_back(*ptr);
            }
            break;

        case COOKIE_PARSE_VALUE:
            // parsing cookie value
            if (value_quote_character == '\0') {
                // value is not (yet) quoted
                if (*ptr == ';' || *ptr == ',') {
                    // end of value found (OK if empty)
                    if (! is_cookie_attribute(cookie_name, set_cookie_header))
                        dict.insert( std::make_pair(cookie_name, cookie_value) );
                    cookie_name.erase();
                    cookie_value.erase();
                    parse_state = COOKIE_PARSE_NAME;
                } else if (*ptr == '\'' || *ptr == '"') {
                    if (cookie_value.empty()) {
                        // begin quoted value
                        value_quote_character = *ptr;
                    } else if (cookie_value.size() >= COOKIE_VALUE_MAX) {
                        // max size exceeded
                        return false;
                    } else {
                        // assume character is part of the (unquoted) value
                        cookie_value.push_back(*ptr);
                    }
                } else if (*ptr != ' ' || !cookie_value.empty()) {  // ignore leading unquoted whitespace
                    // check if control character detected, or max sized exceeded
                    if (is_control(*ptr) || cookie_value.size() >= COOKIE_VALUE_MAX)
                        return false;
                    // character is part of the (unquoted) value
                    cookie_value.push_back(*ptr);
                }
            } else {
                // value is quoted
                if (*ptr == value_quote_character) {
                    // end of value found (OK if empty)
                    if (! is_cookie_attribute(cookie_name, set_cookie_header))
                        dict.insert( std::make_pair(cookie_name, cookie_value) );
                    cookie_name.erase();
                    cookie_value.erase();
                    parse_state = COOKIE_PARSE_IGNORE;
                } else if (cookie_value.size() >= COOKIE_VALUE_MAX) {
                    // max size exceeded
                    return false;
                } else {
                    // character is part of the (quoted) value
                    cookie_value.push_back(*ptr);
                }
            }
            break;

        case COOKIE_PARSE_IGNORE:
            // ignore everything until we reach a comma "," or semicolon ";"
            if (*ptr == ';' || *ptr == ',')
                parse_state = COOKIE_PARSE_NAME;
            break;
        }

        ++ptr;
    }

    // handle last cookie in string
    if (! is_cookie_attribute(cookie_name, set_cookie_header))
        dict.insert( std::make_pair(cookie_name, cookie_value) );

    return true;
}

boost::tribool parser::parse_chunks(http::message::chunk_cache_t& chunks,
    boost::system::error_code& ec)
{
    //
    // note that boost::tribool may have one of THREE states:
    //
    // false: encountered an error while parsing message
    // true: finished successfully parsing the message
    // indeterminate: parsed bytes, but the message is not yet finished
    //
    const char *read_start_ptr = m_read_ptr;
    m_bytes_last_read = 0;
    while (m_read_ptr < m_read_end_ptr) {

        switch (m_chunked_content_parse_state) {
        case PARSE_CHUNK_SIZE_START:
            // we have not yet started parsing the next chunk size
            if (is_hex_digit(*m_read_ptr)) {
                m_chunk_size_str.erase();
                m_chunk_size_str.push_back(*m_read_ptr);
                m_chunked_content_parse_state = PARSE_CHUNK_SIZE;
            } else if (*m_read_ptr == ' ' || *m_read_ptr == '\x09' || *m_read_ptr == '\x0D' || *m_read_ptr == '\x0A') {
                // Ignore leading whitespace.  Technically, the standard probably doesn't allow white space here, 
                // but we'll be flexible, since there's no ambiguity.
                break;
            } else {
                set_error(ec, ERROR_CHUNK_CHAR);
                return false;
            }
            break;

        case PARSE_CHUNK_SIZE:
            if (is_hex_digit(*m_read_ptr)) {
                m_chunk_size_str.push_back(*m_read_ptr);
            } else if (*m_read_ptr == '\x0D') {
                m_chunked_content_parse_state = PARSE_EXPECTING_LF_AFTER_CHUNK_SIZE;
            } else if (*m_read_ptr == ' ' || *m_read_ptr == '\x09') {
                // Ignore trailing tabs or spaces.  Technically, the standard probably doesn't allow this, 
                // but we'll be flexible, since there's no ambiguity.
                m_chunked_content_parse_state = PARSE_EXPECTING_CR_AFTER_CHUNK_SIZE;
            } else if (*m_read_ptr == ';') {
                // Following the semicolon we have text which will be ignored till we encounter
                //  a CRLF
                m_chunked_content_parse_state = PARSE_EXPECTING_IGNORED_TEXT_AFTER_CHUNK_SIZE;
            } else {
                set_error(ec, ERROR_CHUNK_CHAR);
                return false;
            }
            break;
                
        case PARSE_EXPECTING_IGNORED_TEXT_AFTER_CHUNK_SIZE:
            if (*m_read_ptr == '\x0D') {
                m_chunked_content_parse_state = PARSE_EXPECTING_LF_AFTER_CHUNK_SIZE;
            } 
            break;

        case PARSE_EXPECTING_CR_AFTER_CHUNK_SIZE:
            if (*m_read_ptr == '\x0D') {
                m_chunked_content_parse_state = PARSE_EXPECTING_LF_AFTER_CHUNK_SIZE;
            } else if (*m_read_ptr == ' ' || *m_read_ptr == '\x09') {
                // Ignore trailing tabs or spaces.  Technically, the standard probably doesn't allow this, 
                // but we'll be flexible, since there's no ambiguity.
                break;
            } else {
                set_error(ec, ERROR_CHUNK_CHAR);
                return false;
            }
            break;

        case PARSE_EXPECTING_LF_AFTER_CHUNK_SIZE:
            // We received a CR; expecting LF to follow.  We can't be flexible here because 
            // if we see anything other than LF, we can't be certain where the chunk starts.
            if (*m_read_ptr == '\x0A') {
                m_bytes_read_in_current_chunk = 0;
                m_size_of_current_chunk = strtol(m_chunk_size_str.c_str(), 0, 16);
                if (m_size_of_current_chunk == 0) {
                    m_chunked_content_parse_state = PARSE_EXPECTING_FINAL_CR_OR_FOOTERS_AFTER_LAST_CHUNK;
                } else {
                    m_chunked_content_parse_state = PARSE_CHUNK;
                }
            } else {
                set_error(ec, ERROR_CHUNK_CHAR);
                return false;
            }
            break;

        case PARSE_CHUNK:
            if (m_bytes_read_in_current_chunk < m_size_of_current_chunk) {
                if (m_payload_handler) {
                    const std::size_t bytes_avail = bytes_available();
                    const std::size_t bytes_in_chunk = m_size_of_current_chunk - m_bytes_read_in_current_chunk;
                    const std::size_t len = (bytes_in_chunk > bytes_avail) ? bytes_avail : bytes_in_chunk;
                    m_payload_handler(m_read_ptr, len);
                    m_bytes_read_in_current_chunk += len;
                    if (len > 1) m_read_ptr += (len - 1);
                } else if (chunks.size() < m_max_content_length) {
                    chunks.push_back(*m_read_ptr);
                    m_bytes_read_in_current_chunk++;
                }
            }
            if (m_bytes_read_in_current_chunk == m_size_of_current_chunk) {
                m_chunked_content_parse_state = PARSE_EXPECTING_CR_AFTER_CHUNK;
            }
            break;

        case PARSE_EXPECTING_CR_AFTER_CHUNK:
            // we've read exactly m_size_of_current_chunk bytes since starting the current chunk
            if (*m_read_ptr == '\x0D') {
                m_chunked_content_parse_state = PARSE_EXPECTING_LF_AFTER_CHUNK;
            } else {
                set_error(ec, ERROR_CHUNK_CHAR);
                return false;
            }
            break;

        case PARSE_EXPECTING_LF_AFTER_CHUNK:
            // we received a CR; expecting LF to follow
            if (*m_read_ptr == '\x0A') {
                m_chunked_content_parse_state = PARSE_CHUNK_SIZE_START;
            } else {
                set_error(ec, ERROR_CHUNK_CHAR);
                return false;
            }
            break;

        case PARSE_EXPECTING_FINAL_CR_OR_FOOTERS_AFTER_LAST_CHUNK:
            // we've read the final chunk; expecting final CRLF
            if (*m_read_ptr == '\x0D') {
                m_chunked_content_parse_state = PARSE_EXPECTING_FINAL_LF_AFTER_LAST_CHUNK;
            } else {
                // Packet contains footers; Chunk parsing is commplete
                // Footer data contains name value pairs to be added to HTTP Message
                m_message_parse_state = PARSE_FOOTERS;
                m_headers_parse_state = PARSE_HEADER_START;
                m_bytes_last_read = (m_read_ptr - read_start_ptr);
                m_bytes_total_read += m_bytes_last_read;
                m_bytes_content_read += m_bytes_last_read;
                PION_LOG_DEBUG(m_logger, "Parsed " << m_bytes_last_read << " chunked payload content bytes; chunked content complete.");
                return true;
            }
            break;

        case PARSE_EXPECTING_FINAL_LF_AFTER_LAST_CHUNK:
            // we received the final CR; expecting LF to follow
            if (*m_read_ptr == '\x0A') {
                ++m_read_ptr;
                m_bytes_last_read = (m_read_ptr - read_start_ptr);
                m_bytes_total_read += m_bytes_last_read;
                m_bytes_content_read += m_bytes_last_read;
                PION_LOG_DEBUG(m_logger, "Parsed " << m_bytes_last_read << " chunked payload content bytes; chunked content complete.");
                return true;
            } else {
                set_error(ec, ERROR_CHUNK_CHAR);
                return false;
            }
        }

        ++m_read_ptr;
    }

    m_bytes_last_read = (m_read_ptr - read_start_ptr);
    m_bytes_total_read += m_bytes_last_read;
    m_bytes_content_read += m_bytes_last_read;
    return boost::indeterminate;
}

boost::tribool parser::consume_content(http::message& http_msg,
    boost::system::error_code& ec)
{
    size_t content_bytes_to_read;
    size_t content_bytes_available = bytes_available();
    boost::tribool rc = boost::indeterminate;

    if (m_bytes_content_remaining == 0) {
        // we have all of the remaining payload content
        return true;
    } else {
        if (content_bytes_available >= m_bytes_content_remaining) {
            // we have all of the remaining payload content
            rc = true;
            content_bytes_to_read = m_bytes_content_remaining;
        } else {
            // only some of the payload content is available
            content_bytes_to_read = content_bytes_available;
        }
        m_bytes_content_remaining -= content_bytes_to_read;
    }

    // make sure content buffer is not already full
    if (m_payload_handler) {
        m_payload_handler(m_read_ptr, content_bytes_to_read);
    } else if (m_bytes_content_read < m_max_content_length) {
        if (m_bytes_content_read + content_bytes_to_read > m_max_content_length) {
            // read would exceed maximum size for content buffer
            // copy only enough bytes to fill up the content buffer
            memcpy(http_msg.get_content() + m_bytes_content_read, m_read_ptr, 
                m_max_content_length - m_bytes_content_read);
        } else {
            // copy all bytes available
            memcpy(http_msg.get_content() + m_bytes_content_read, m_read_ptr, content_bytes_to_read);
        }
    }

    m_read_ptr += content_bytes_to_read;
    m_bytes_content_read += content_bytes_to_read;
    m_bytes_total_read += content_bytes_to_read;
    m_bytes_last_read = content_bytes_to_read;

    return rc;
}

std::size_t parser::consume_content_as_next_chunk(http::message::chunk_cache_t& chunks)
{
    if (bytes_available() == 0) {
        m_bytes_last_read = 0;
    } else {
        // note: m_bytes_last_read must be > 0 because of bytes_available() check
        m_bytes_last_read = (m_read_end_ptr - m_read_ptr);
        if (m_payload_handler) {
            m_payload_handler(m_read_ptr, m_bytes_last_read);
            m_read_ptr += m_bytes_last_read;
        } else {
            while (m_read_ptr < m_read_end_ptr) {
                if (chunks.size() < m_max_content_length)
                    chunks.push_back(*m_read_ptr);
                ++m_read_ptr;
            }
        }
        m_bytes_total_read += m_bytes_last_read;
        m_bytes_content_read += m_bytes_last_read;
    }
    return m_bytes_last_read;
}

void parser::finish(http::message& http_msg) const
{
    switch (m_message_parse_state) {
    case PARSE_START:
        http_msg.set_is_valid(false);
        http_msg.set_content_length(0);
        http_msg.create_content_buffer();
        return;
    case PARSE_END:
        http_msg.set_is_valid(true);
        break;
    case PARSE_HEADERS:
    case PARSE_FOOTERS:
        http_msg.set_is_valid(false);
        update_message_with_header_data(http_msg);
        http_msg.set_content_length(0);
        http_msg.create_content_buffer();
        break;
    case PARSE_CONTENT:
        http_msg.set_is_valid(false);
        if (get_content_bytes_read() < m_max_content_length)   // NOTE: we can read more than we have allocated/stored
            http_msg.set_content_length(get_content_bytes_read());
        break;
    case PARSE_CHUNKS:
        http_msg.set_is_valid(m_chunked_content_parse_state==PARSE_CHUNK_SIZE_START);
        if (!m_payload_handler)
            http_msg.concatenate_chunks();
        break;
    case PARSE_CONTENT_NO_LENGTH:
        http_msg.set_is_valid(true);
        if (!m_payload_handler)
            http_msg.concatenate_chunks();
        break;
    }

    compute_msg_status(http_msg, http_msg.is_valid());

    if (is_parsing_request() && !m_payload_handler && !m_parse_headers_only) {
        // Parse query pairs from post content if content type is x-www-form-urlencoded.
        // Type could be followed by parameters (as defined in section 3.6 of RFC 2616)
        // e.g. Content-Type: application/x-www-form-urlencoded; charset=UTF-8
        http::request& http_request(dynamic_cast<http::request&>(http_msg));
        const std::string& content_type_header = http_request.get_header(http::types::HEADER_CONTENT_TYPE);
        if (content_type_header.compare(0, http::types::CONTENT_TYPE_URLENCODED.length(),
                                        http::types::CONTENT_TYPE_URLENCODED) == 0)
        {
            if (! parse_url_encoded(http_request.get_queries(),
                                  http_request.get_content(),
                                  http_request.get_content_length()))
                PION_LOG_WARN(m_logger, "Request form data parsing failed (POST urlencoded)");
        } else if (content_type_header.compare(0, http::types::CONTENT_TYPE_MULTIPART_FORM_DATA.length(),
                                               http::types::CONTENT_TYPE_MULTIPART_FORM_DATA) == 0)
        {
            if (! parse_multipart_form_data(http_request.get_queries(),
                                            content_type_header,
                                            http_request.get_content(),
                                            http_request.get_content_length()))
                PION_LOG_WARN(m_logger, "Request form data parsing failed (POST multipart)");
        }
    }
}

void parser::compute_msg_status(http::message& http_msg, bool msg_parsed_ok )
{
    http::message::data_status_t st = http::message::STATUS_NONE;

    if(http_msg.has_missing_packets()) {
        st = http_msg.has_data_after_missing_packets() ?
                        http::message::STATUS_PARTIAL : http::message::STATUS_TRUNCATED;
    } else {
        st = msg_parsed_ok ? http::message::STATUS_OK : http::message::STATUS_TRUNCATED;
    }

    http_msg.set_status(st);
}

void parser::create_error_category(void)
{
    static error_category_t UNIQUE_ERROR_CATEGORY;
    m_error_category_ptr = &UNIQUE_ERROR_CATEGORY;
}

bool parser::parse_forwarded_for(const std::string& header, std::string& public_ip)
{
    // static regex's used to check for ipv4 address
    static const boost::regex IPV4_ADDR_RX("[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}");

    /// static regex used to check for private/local networks:
    /// 10.*
    /// 127.*
    /// 192.168.*
    /// 172.16-31.*
    static const boost::regex PRIVATE_NET_RX("(10\\.[0-9]{1,3}|127\\.[0-9]{1,3}|192\\.168|172\\.1[6-9]|172\\.2[0-9]|172\\.3[0-1])\\.[0-9]{1,3}\\.[0-9]{1,3}");

    // sanity check
    if (header.empty())
        return false;

    // local variables re-used by while loop
    boost::match_results<std::string::const_iterator> m;
    std::string::const_iterator start_it = header.begin();

    // search for next ip address within the header
    while (boost::regex_search(start_it, header.end(), m, IPV4_ADDR_RX)) {
        // get ip that matched
        std::string ip_str(m[0].first, m[0].second);
        // check if public network ip address
        if (! boost::regex_match(ip_str, PRIVATE_NET_RX) ) {
            // match found!
            public_ip = ip_str;
            return true;
        }
        // update search starting position
        start_it = m[0].second;
    }

    // no matches found
    return false;
}

}   // end namespace http
}   // end namespace pion
