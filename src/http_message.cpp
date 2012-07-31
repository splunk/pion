// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <iostream>
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <boost/regex.hpp>
#include <boost/logic/tribool.hpp>
#include <pion/http/message.hpp>
#include <pion/http/request.hpp>
#include <pion/http/parser.hpp>
#include <pion/tcp/connection.hpp>


namespace pion {    // begin namespace pion
namespace net {     // begin namespace net (Pion Network Library)

// static members of HTTPMessage

const boost::regex      HTTPMessage::REGEX_ICASE_CHUNKED(".*chunked.*", boost::regex::icase);


// HTTPMessage member functions

std::size_t HTTPMessage::send(TCPConnection& tcp_conn,
                              boost::system::error_code& ec,
                              bool headers_only)
{
    // initialize write buffers for send operation using HTTP headers
    WriteBuffers write_buffers;
    prepareBuffersForSend(write_buffers, tcp_conn.getKeepAlive(), false);

    // append payload content to write buffers (if there is any)
    if (!headers_only && getContentLength() > 0 && getContent() != NULL)
        write_buffers.push_back(boost::asio::buffer(getContent(), getContentLength()));

    // send the message and return the result
    return tcp_conn.write(write_buffers, ec);
}

std::size_t HTTPMessage::receive(TCPConnection& tcp_conn,
                                 boost::system::error_code& ec,
                                 bool headers_only)
{
    // assumption: this can only be either an HTTPRequest or an HTTPResponse
    const bool is_request = (dynamic_cast<HTTPRequest*>(this) != NULL);
    HTTPParser http_parser(is_request);
    http_parser.parseHeadersOnly(headers_only);
    std::size_t last_bytes_read = 0;

    // make sure that we start out with an empty message
    clear();

    if (tcp_conn.getPipelined()) {
        // there are pipelined messages available in the connection's read buffer
        const char *read_ptr;
        const char *read_end_ptr;
        tcp_conn.loadReadPosition(read_ptr, read_end_ptr);
        last_bytes_read = (read_end_ptr - read_ptr);
        http_parser.setReadBuffer(read_ptr, last_bytes_read);
    } else {
        // read buffer is empty (not pipelined) -> read some bytes from the connection
        last_bytes_read = tcp_conn.read_some(ec);
        if (ec) return 0;
        BOOST_ASSERT(last_bytes_read > 0);
        http_parser.setReadBuffer(tcp_conn.getReadBuffer().data(), last_bytes_read);
    }

    // incrementally read and parse bytes from the connection
    bool force_connection_closed = false;
    boost::tribool parse_result;
    while (true) {
        // parse bytes available in the read buffer
        parse_result = http_parser.parse(*this, ec);
        if (! boost::indeterminate(parse_result)) break;

        // read more bytes from the connection
        last_bytes_read = tcp_conn.read_some(ec);
        if (ec || last_bytes_read == 0) {
            if (http_parser.checkPrematureEOF(*this)) {
                // premature EOF encountered
                if (! ec)
                    ec = make_error_code(boost::system::errc::io_error);
                return http_parser.getTotalBytesRead();
            } else {
                // EOF reached when content length unknown
                // assume it is the correct end of content
                // and everything is OK
                force_connection_closed = true;
                parse_result = true;
                ec.clear();
                break;
            }
            break;
        }

        // update the HTTP parser's read buffer
        http_parser.setReadBuffer(tcp_conn.getReadBuffer().data(), last_bytes_read);
    }
    
    if (parse_result == false) {
        // an error occurred while parsing the message headers
        return http_parser.getTotalBytesRead();
    }

    // set the connection's lifecycle type
    if (!force_connection_closed && checkKeepAlive()) {
        if ( http_parser.eof() ) {
            // the connection should be kept alive, but does not have pipelined messages
            tcp_conn.setLifecycle(TCPConnection::LIFECYCLE_KEEPALIVE);
        } else {
            // the connection has pipelined messages
            tcp_conn.setLifecycle(TCPConnection::LIFECYCLE_PIPELINED);
            
            // save the read position as a bookmark so that it can be retrieved
            // by a new HTTP parser, which will be created after the current
            // message has been handled
            const char *read_ptr;
            const char *read_end_ptr;
            http_parser.loadReadPosition(read_ptr, read_end_ptr);
            tcp_conn.saveReadPosition(read_ptr, read_end_ptr);
        }
    } else {
        // default to close the connection
        tcp_conn.setLifecycle(TCPConnection::LIFECYCLE_CLOSE);
    }

    return (http_parser.getTotalBytesRead());
}

std::size_t HTTPMessage::write(std::ostream& out,
    boost::system::error_code& ec, bool headers_only)
{
    // reset error_code
    ec.clear();

    // initialize write buffers for send operation using HTTP headers
    WriteBuffers write_buffers;
    prepareBuffersForSend(write_buffers, true, false);

    // append payload content to write buffers (if there is any)
    if (!headers_only && getContentLength() > 0 && getContent() != NULL)
        write_buffers.push_back(boost::asio::buffer(getContent(), getContentLength()));

    // write message to the output stream
    std::size_t bytes_out = 0;
    for (WriteBuffers::const_iterator i=write_buffers.begin(); i!=write_buffers.end(); ++i) {
        const char *ptr = boost::asio::buffer_cast<const char*>(*i);
        size_t len = boost::asio::buffer_size(*i);
        out.write(ptr, len);
        bytes_out += len;
    }

    return bytes_out;
}

std::size_t HTTPMessage::read(std::istream& in,
    boost::system::error_code& ec, bool headers_only)
{
    // make sure that we start out with an empty message & clear error_code
    clear();
    ec.clear();
    
    // assumption: this can only be either an HTTPRequest or an HTTPResponse
    const bool is_request = (dynamic_cast<HTTPRequest*>(this) != NULL);
    HTTPParser http_parser(is_request);
    http_parser.parseHeadersOnly(headers_only);

    // parse data from file one byte at a time
    boost::tribool parse_result;
    char c;
    while (in) {
        in.read(&c, 1);
        if ( ! in ) {
            ec = make_error_code(boost::system::errc::io_error);
            break;
        }
        http_parser.setReadBuffer(&c, 1);
        parse_result = http_parser.parse(*this, ec);
        if (! boost::indeterminate(parse_result)) break;
    }

    if (boost::indeterminate(parse_result)) {
        if (http_parser.checkPrematureEOF(*this)) {
            // premature EOF encountered
            if (! ec)
                ec = make_error_code(boost::system::errc::io_error);
        } else {
            // EOF reached when content length unknown
            // assume it is the correct end of content
            // and everything is OK
            parse_result = true;
            ec.clear();
        }
    }
    
    return (http_parser.getTotalBytesRead());
}

void HTTPMessage::concatenateChunks(void)
{
    setContentLength(m_chunk_cache.size());
    char *post_buffer = createContentBuffer();
    if (m_chunk_cache.size() > 0)
        std::copy(m_chunk_cache.begin(), m_chunk_cache.end(), post_buffer);
}
    
}   // end namespace net
}   // end namespace pion
