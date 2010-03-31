// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTPREADER_HEADER__
#define __PION_HTTPREADER_HEADER__

#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <pion/PionConfig.hpp>
#include <pion/net/HTTPParser.hpp>
#include <pion/net/HTTPMessage.hpp>
#include <pion/net/TCPConnection.hpp>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)


///
/// HTTPReader: asynchronously reads and parses HTTP messages
///
class PION_NET_API HTTPReader :
	public HTTPParser
{
public:

	// default destructor
	virtual ~HTTPReader() {}
	
	/// Incrementally reads & parses the HTTP message
	void receive(void);
	
	/// returns a shared pointer to the TCP connection
	inline TCPConnectionPtr& getTCPConnection(void) { return m_tcp_conn; }
	
	/// sets the maximum number of seconds for read operations
	inline void setTimeout(boost::uint32_t seconds) { m_read_timeout = seconds; }

	
protected:

	/**
	 * protected constructor: only derived classes may create objects
	 *
	 * @param is_request if true, the message is parsed as an HTTP request;
	 *                   if false, the message is parsed as an HTTP response
	 * @param tcp_conn TCP connection containing a new message to parse
	 */
	HTTPReader(const bool is_request, TCPConnectionPtr& tcp_conn)
		: HTTPParser(is_request), m_tcp_conn(tcp_conn),
		m_read_timer(m_tcp_conn->getIOService()),
		m_timer_active(false), m_read_active(false),
		m_read_timeout(DEFAULT_READ_TIMEOUT)
		{}	
	
	/**
	 * Consumes bytes that have been read using an HTTP parser
	 * 
	 * @param read_error error status from the last read operation
	 * @param bytes_read number of bytes consumed by the last read operation
	 */
	void consumeBytes(const boost::system::error_code& read_error,
                      std::size_t bytes_read);

    /// Consumes bytes that have been read using an HTTP parser
    void consumeBytes(void);
	
	/// Reads more bytes from the TCP connection
	virtual void readBytes(void) = 0;

    /// Called after we have finished reading/parsing the HTTP message
    virtual void finishedReading(void) = 0;

    /// Returns a reference to the HTTP message being parsed
    virtual HTTPMessage& getMessage(void) = 0;


private:

	/// reads more bytes for parsing, with timeout support
	void readBytesWithTimeout(void);

	/**
	 * Callback handler for the read timer
	 *
	 * @param ec deadline timer error status code
	 */
	void timerCallback(const boost::system::error_code& ec);

	/**
	 * Handles errors that occur during read operations
	 *
	 * @param read_error error status from the last read operation
	 */
	void handleReadError(const boost::system::error_code& read_error);


	/// default maximum number of seconds for read operations
	static const boost::uint32_t			DEFAULT_READ_TIMEOUT;


	/// The HTTP connection that has a new HTTP message to parse
	TCPConnectionPtr						m_tcp_conn;

	/// deadline timer used to timeout long-running read operations
	boost::asio::deadline_timer				m_read_timer;
	
	/// conditioned signaled after the read timer has finished
	boost::condition						m_timer_finished;

	/// mutex used to synchronize the timeout deadline timer
	boost::mutex							m_timer_mutex;

	/// true if the read timer is active
	bool									m_timer_active;	

	/// true if a read operation is active
	bool									m_read_active;	

	/// maximum number of seconds for read operations
	boost::uint32_t							m_read_timeout;
};


}	// end namespace net
}	// end namespace pion

#endif
