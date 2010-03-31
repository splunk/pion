// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/asio.hpp>
#include <boost/logic/tribool.hpp>
#include <pion/net/HTTPReader.hpp>
#include <pion/net/HTTPRequest.hpp>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)


// HTTPReader static members
	
const boost::uint32_t		HTTPReader::DEFAULT_READ_TIMEOUT = 10;


// HTTPReader member functions

void HTTPReader::receive(void)
{
	if (m_tcp_conn->getPipelined()) {
		// there are pipelined messages available in the connection's read buffer
		m_tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_CLOSE);	// default to close the connection
		m_tcp_conn->loadReadPosition(m_read_ptr, m_read_end_ptr);
		consumeBytes();
	} else {
		// no pipelined messages available in the read buffer -> read bytes from the socket
		m_tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_CLOSE);	// default to close the connection
		readBytesWithTimeout();
	}
}

void HTTPReader::consumeBytes(const boost::system::error_code& read_error,
							  std::size_t bytes_read)
{
	// cancel read timer if operation didn't time-out
	if (m_read_timeout > 0) {
		boost::mutex::scoped_lock timer_lock(m_timer_mutex);
		m_read_active = false;
		if (m_timer_active) {
			m_timer_stop.notify_all();
			timer_lock.unlock();
			m_timer_thread_ptr->join();
		}
	}

	if (read_error) {
		// a read error occured
		handleReadError(read_error);
		return;
	}
	
	PION_LOG_DEBUG(m_logger, "Read " << bytes_read << " bytes from HTTP "
				   << (isParsingRequest() ? "request" : "response"));

	// set pointers for new HTTP header data to be consumed
	setReadBuffer(m_tcp_conn->getReadBuffer().data(), bytes_read);

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
	boost::tribool result = parse(getMessage());
	
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
				m_tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_KEEPALIVE);
			} else {
				// the connection has pipelined messages
				m_tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_PIPELINED);

				// save the read position as a bookmark so that it can be retrieved
				// by a new HTTP parser, which will be created after the current
				// message has been handled
				m_tcp_conn->saveReadPosition(m_read_ptr, m_read_end_ptr);

				PION_LOG_DEBUG(m_logger, "HTTP pipelined "
							   << (isParsingRequest() ? "request (" : "response (")
							   << bytes_available() << " bytes available)");
			}
		} else {
			m_tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_CLOSE);
		}

		// we have finished parsing the HTTP message
		finishedReading();

	} else if (result == false) {
		// the message is invalid or an error occured

	#ifndef NDEBUG
		// display extra error information if debug mode is enabled
		std::string bad_message;
		m_read_ptr = m_tcp_conn->getReadBuffer().data();
		while (m_read_ptr < m_read_end_ptr && bad_message.size() < 50) {
#ifndef _MSC_VER
// There's a bug in MSVC's implementation of isprint().
			if (!isprint(*m_read_ptr) || *m_read_ptr == '\n' || *m_read_ptr=='\r')
				bad_message += '.';
			else bad_message += *m_read_ptr;
#endif
			++m_read_ptr;
		}
		PION_LOG_ERROR(m_logger, "Bad " << (isParsingRequest() ? "request" : "response")
					   << " debug: " << bad_message);
	#endif

		m_tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_CLOSE);	// make sure it will get closed
		getMessage().setIsValid(false);
		finishedReading();
		
	} else {
		// not yet finished parsing the message -> read more data
		readBytesWithTimeout();
	}
}

void HTTPReader::readBytesWithTimeout(void)
{
	if (m_read_timeout > 0) {
		boost::mutex::scoped_lock timer_lock(m_timer_mutex);
		m_read_active = true;
		m_timer_active = true;
		m_timer_thread_ptr.reset(new boost::thread(boost::bind(&HTTPReader::runTimer, this)));
	}
	readBytes();
}

void HTTPReader::runTimer(void)
{
	boost::mutex::scoped_lock timer_lock(m_timer_mutex);
	try {
		if (m_read_active)
			m_timer_stop.timed_wait(m_timer_mutex, boost::posix_time::seconds(m_read_timeout));
		if (m_read_active) {
			PION_LOG_DEBUG(m_logger, "Read operation timed-out, closing connection");
			m_tcp_conn->close();
		}
	} catch (std::exception& e) {
		PION_LOG_ERROR(m_logger, e.what());
	} catch (...) {
		PION_LOG_ERROR(m_logger, "caught unrecognized exception");
	}
	m_timer_active = false;
}

void HTTPReader::handleReadError(const boost::system::error_code& read_error)
{
	// close the connection, forcing the client to establish a new one
	m_tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_CLOSE);	// make sure it will get closed

	// check if this is just a message with unknown content length
	if (! checkPrematureEOF(getMessage())) {
		finishedReading();
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

	// do not trigger the callback when there are errors -> currently no way to propagate them
	m_tcp_conn->finish();
}

}	// end namespace net
}	// end namespace pion

