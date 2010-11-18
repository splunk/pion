// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2010 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/net/TCPTimer.hpp>
#include <boost/bind.hpp>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)


// TCPTimer member functions

TCPTimer::TCPTimer(TCPConnectionPtr& conn_ptr)
	: m_conn_ptr(conn_ptr), m_timer(conn_ptr->getIOService()),
	m_timer_active(false), m_was_cancelled(false)
{
}

void TCPTimer::start(const boost::uint32_t seconds)
{
	boost::mutex::scoped_lock timer_lock(m_mutex);
	m_timer_active = true;
	m_timer.expires_from_now(boost::posix_time::seconds(seconds));
	m_timer.async_wait(boost::bind(&TCPTimer::timerCallback,
		shared_from_this(), _1));
}

void TCPTimer::cancel(void)
{
	boost::mutex::scoped_lock timer_lock(m_mutex);
	m_was_cancelled = true;
	if (m_timer_active)
		m_timer.cancel();
}

void TCPTimer::timerCallback(const boost::system::error_code& ec)
{
	boost::mutex::scoped_lock timer_lock(m_mutex);
	m_timer_active = false;
	if (! m_was_cancelled)
		m_conn_ptr->close();
}


}	// end namespace net
}	// end namespace pion
