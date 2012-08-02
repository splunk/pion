// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/asio.hpp>
#include <boost/logic/tribool.hpp>
#include <pion/http/reader.hpp>
#include <pion/http/request.hpp>


namespace pion {    // begin namespace pion
namespace http {    // begin namespace http


// HTTPReader static members
    
const boost::uint32_t       HTTPReader::DEFAULT_READ_TIMEOUT = 10;


// HTTPReader member functions

void HTTPReader::receive(void)
{
    if (m_tcp_conn->get_pipelined()) {
        // there are pipelined messages available in the connection's read buffer
        m_tcp_conn->set_lifecycle(connection::LIFECYCLE_CLOSE);   // default to close the connection
        m_tcp_conn->load_read_pos(m_read_ptr, m_read_end_ptr);
        consumeBytes();
    } else {
        // no pipelined messages available in the read buffer -> read bytes from the socket
        m_tcp_conn->set_lifecycle(connection::LIFECYCLE_CLOSE);   // default to close the connection
        readBytesWithTimeout();
    }
}

void HTTPReader::consumeBytes(const boost::system::error_code& read_error,
                              std::size_t bytes_read)
{
    // cancel read timer if operation didn't time-out
    if (m_timer_ptr) {
        m_timer_ptr->cancel();
        m_timer_ptr.reset();
    }

    if (read_error) {
        // a read error occured
        handleReadError(read_error);
        return;
    }
    
    PION_LOG_DEBUG(m_logger, "Read " << bytes_read << " bytes from HTTP "
                   << (isParsingRequest() ? "request" : "response"));

    // set pointers for new HTTP header data to be consumed
    set_read_buffer(m_tcp_conn->get_read_buffer().data(), bytes_read);

    consumeBytes();
}


void HTTPReader::consumeBytes(void)
{
    // parse the bytes read from the last operation
    //
    // note that boost::tribool may have one of THREE states:
    //
    // false: encountered an error while parsing message
    // true: finished successfully parsing the message
    // indeterminate: parsed bytes, but the message is not yet finished
    //
    boost::system::error_code ec;
    boost::tribool result = parse(getMessage(), ec);
    
    if (gcount() > 0) {
        // parsed > 0 bytes in HTTP headers
        PION_LOG_DEBUG(m_logger, "Parsed " << gcount() << " HTTP bytes");
    }

    if (result == true) {
        // finished reading HTTP message and it is valid

        // set the connection's lifecycle type
        if (getMessage().checkKeepAlive()) {
            if ( eof() ) {
                // the connection should be kept alive, but does not have pipelined messages
                m_tcp_conn->set_lifecycle(connection::LIFECYCLE_KEEPALIVE);
            } else {
                // the connection has pipelined messages
                m_tcp_conn->set_lifecycle(connection::LIFECYCLE_PIPELINED);

                // save the read position as a bookmark so that it can be retrieved
                // by a new HTTP parser, which will be created after the current
                // message has been handled
                m_tcp_conn->save_read_pos(m_read_ptr, m_read_end_ptr);

                PION_LOG_DEBUG(m_logger, "HTTP pipelined "
                               << (isParsingRequest() ? "request (" : "response (")
                               << bytes_available() << " bytes available)");
            }
        } else {
            m_tcp_conn->set_lifecycle(connection::LIFECYCLE_CLOSE);
        }

        // we have finished parsing the HTTP message
        finishedReading(ec);

    } else if (result == false) {
        // the message is invalid or an error occured
        m_tcp_conn->set_lifecycle(connection::LIFECYCLE_CLOSE);   // make sure it will get closed
        getMessage().setIsValid(false);
        finishedReading(ec);
    } else {
        // not yet finished parsing the message -> read more data
        readBytesWithTimeout();
    }
}

void HTTPReader::readBytesWithTimeout(void)
{
    if (m_read_timeout > 0) {
        m_timer_ptr.reset(new tcp::timer(m_tcp_conn));
        m_timer_ptr->start(m_read_timeout);
    } else if (m_timer_ptr) {
        m_timer_ptr.reset();
    }
    readBytes();
}

void HTTPReader::handleReadError(const boost::system::error_code& read_error)
{
    // close the connection, forcing the client to establish a new one
    m_tcp_conn->set_lifecycle(connection::LIFECYCLE_CLOSE);   // make sure it will get closed

    // check if this is just a message with unknown content length
    if (! checkPrematureEOF(getMessage())) {
        boost::system::error_code ec;   // clear error code
        finishedReading(ec);
        return;
    }
    
    // only log errors if the parsing has already begun
    if (getTotalBytesRead() > 0) {
        if (read_error == boost::asio::error::operation_aborted) {
            // if the operation was aborted, the acceptor was stopped,
            // which means another thread is shutting-down the server
            PION_LOG_INFO(m_logger, "HTTP " << (isParsingRequest() ? "request" : "response")
                          << " parsing aborted (shutting down)");
        } else {
            PION_LOG_INFO(m_logger, "HTTP " << (isParsingRequest() ? "request" : "response")
                          << " parsing aborted (" << read_error.message() << ')');
        }
    }

    finishedReading(read_error);
}

}   // end namespace http
}   // end namespace pion
