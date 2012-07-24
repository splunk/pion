// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_LOGSERVICE_HEADER__
#define __PION_LOGSERVICE_HEADER__

#include <boost/thread/mutex.hpp>
#include <boost/scoped_ptr.hpp>
#include <pion/logger.hpp>
#include <pion/http/plugin_service.hpp>
#include <pion/http/response_writer.hpp>
#include <string>
#include <list>

#if defined(PION_USE_LOG4CXX)
	#include <log4cxx/appenderskeleton.h>
	// version 0.10.x introduces a new data type that is declared in a
	// pool.h header file.  If we're using 0.9.x, just declare the type
	// as an int since it is not being used at all
	#ifndef _LOG4CXX_HELPERS_POOL_H
		namespace log4cxx {
			namespace helpers {
				typedef int Pool;
			}
		}
	#endif
#endif


namespace pion {		// begin namespace pion
namespace plugins {		// begin namespace plugins

	
///
/// LogServiceAppender: caches log events in memory for use by LogService
/// 
class LogServiceAppender
#ifdef PION_HAS_LOG_APPENDER
	: public PionLogAppender
#endif
{
public:
	// default constructor and destructor
	LogServiceAppender(void);
	virtual ~LogServiceAppender() {}
	
	/// sets the maximum number of log events cached in memory
	inline void setMaxEvents(unsigned int n) { m_max_events = n; }
	
	/// adds a formatted log message to the memory cache
	void addLogString(const std::string& log_string);

	/// writes the events cached in memory to a response stream
	void writeLogEvents(pion::net::HTTPResponseWriterPtr& writer);

private:
	/// default maximum number of events cached in memory
	static const unsigned int				DEFAULT_MAX_EVENTS;
	
	/// maxiumum number of events cached in memory
	unsigned int							m_max_events;
	
	/// number of events currently cached in memory
	unsigned int							m_num_events;

	/// memory cache of pre-formatted log events
	std::list<std::string>					m_log_events;

	/// mutex to make class thread-safe
	boost::mutex							m_log_mutex;

#if defined(PION_USE_LOG4CXX)
	public:
		// member functions inherited from the Appender interface class
		virtual void close() {}
		virtual bool requiresLayout() const { return false; }
	protected:
		/// adds log event to the memory cache
		virtual void append(const log4cxx::spi::LoggingEventPtr& event);
		// version 0.10.x adds a second "pool" argument -> just ignore it
		virtual void append(const log4cxx::spi::LoggingEventPtr& event,
							log4cxx::helpers::Pool& pool)
		{
			append(event);
		}
#elif defined(PION_USE_LOG4CPLUS)
	public:
		// member functions inherited from the Appender interface class
		virtual void close() {}
	protected:
		virtual void append(const log4cplus::spi::InternalLoggingEvent& event);
	private:
		/// this is used to convert numeric log levels into strings
		log4cplus::LogLevelManager		m_log_level_manager;
#elif defined(PION_USE_LOG4CPP)
	public:
		// member functions inherited from the AppenderSkeleton class
		virtual void close() {}
		virtual bool requiresLayout() const { return true; }
		virtual void setLayout(log4cpp::Layout* layout) { m_layout_ptr.reset(layout); }
	protected:
		/// adds log event to the memory cache
		virtual void _append(const log4cpp::LoggingEvent& event);
	private:
		/// the logging layout used to format events
		boost::scoped_ptr<log4cpp::Layout>		m_layout_ptr;
#endif

};


///
/// LogService: web service that displays log messages
/// 
class LogService :
	public pion::net::WebService
{
public:
	// default constructor and destructor
	LogService(void);
	virtual ~LogService();
	
	/// handles a new HTTP request
	virtual void operator()(pion::net::HTTPRequestPtr& request,
							pion::net::TCPConnectionPtr& tcp_conn);

	/// returns the log appender used by LogService
	inline LogServiceAppender& getLogAppender(void) {
#ifdef PION_HAS_LOG_APPENDER
		return dynamic_cast<LogServiceAppender&>(*m_log_appender_ptr);
#else
		return *m_log_appender_ptr;
#endif
	}
	
private:
	/// this is used to keep track of log messages
#ifdef PION_HAS_LOG_APPENDER
	PionLogAppenderPtr		m_log_appender_ptr;
#else
	LogServiceAppender *	m_log_appender_ptr;
#endif
};

	
}	// end namespace plugins
}	// end namespace pion

#endif
