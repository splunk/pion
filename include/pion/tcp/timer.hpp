// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_TCPTIMER_HEADER__
#define __PION_TCPTIMER_HEADER__

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/mutex.hpp>
#include <pion/config.hpp>
#include <pion/tcp/connection.hpp>


namespace pion {    // begin namespace pion
namespace net {     // begin namespace net (Pion Network Library)


///
/// TCPTimer: helper class used to time-out TCP connections
///
class TCPTimer
    : public boost::enable_shared_from_this<TCPTimer>
{
public:

    /**
     * creates a new TCP connection timer
     *
     * @param conn_ptr pointer to TCP connection to monitor
     */
    TCPTimer(TCPConnectionPtr& conn_ptr);

    /**
     * starts a timer for closing a TCP connection
     *
     * @param seconds number of seconds before the timeout triggers
     */
    void start(const boost::uint32_t seconds);

    /// cancel the timer (operation completed)
    void cancel(void);


private:

    /**
     * Callback handler for the deadline timer
     *
     * @param ec deadline timer error status code
     */
    void timerCallback(const boost::system::error_code& ec);

    /// pointer to the TCP connection that is being monitored
    TCPConnectionPtr                        m_conn_ptr;

    /// deadline timer used to timeout TCP operations
    boost::asio::deadline_timer             m_timer;
    
    /// mutex used to synchronize the TCP connection timer
    boost::mutex                            m_mutex;

    /// true if the deadline timer is active
    bool                                    m_timer_active; 

    /// true if the timer was cancelled (operation completed)
    bool                                    m_was_cancelled;    
};


/// shared pointer to a TCPTimer object
typedef boost::shared_ptr<TCPTimer>     TCPTimerPtr;


}   // end namespace net
}   // end namespace pion

#endif
