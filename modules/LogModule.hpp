// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_LOGMODULE_HEADER__
#define __PION_LOGMODULE_HEADER__

#include <boost/thread/mutex.hpp>
#include <boost/scoped_ptr.hpp>
#include <pion/net/PionLogger.hpp>
#include <pion/net/HTTPModule.hpp>
#include <pion/net/HTTPResponse.hpp>
#include <string>
#include <list>

#if defined(PION_HAVE_LOG4CXX)
	#include <log4cxx/appenderskeleton.h>
#elif defined(PION_HAVE_LOG4CPLUS)
	#include <log4cplus/appender.h>
	#include <log4cplus/loglevel.h>
#elif defined(PION_HAVE_LOG4CPP)
	#include <log4cpp/AppenderSkeleton.hh>
#endif


///
/// LogModuleAppender: caches log events in memory for use by LogModule
/// 
class LogModuleAppender
	#if defined(PION_HAVE_LOG4CXX)
		: public log4cxx::AppenderSkeleton
	#elif defined(PION_HAVE_LOG4CPLUS)
		: public log4cplus::Appender
	#elif defined(PION_HAVE_LOG4CPP)
		: public log4cpp::AppenderSkeleton
	#endif
{
public:
	// default constructor and destructor
	LogModuleAppender(void);
	virtual ~LogModuleAppender() {}
	
	/// sets the maximum number of log events cached in memory
	inline void setMaxEvents(unsigned int n) { m_max_events = n; }
	
	/// adds a formatted log message to the memory cache
	void addLogString(const std::string& log_string);

	/// writes the events cached in memory to a response stream
	void writeLogEvents(pion::HTTPResponsePtr& response);

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

#if defined(PION_HAVE_LOG4CXX)
	public:
		// member functions inherited from the Appender interface class
        virtual void close() {}
		virtual bool requiresLayout() const { return false; }
	protected:
		/// adds log event to the memory cache
		virtual void append(const log4cxx::spi::LoggingEventPtr& event);
#elif defined(PION_HAVE_LOG4CPLUS)
	public:
		// member functions inherited from the Appender interface class
        virtual void close() {}
	protected:
        virtual void append(const log4cplus::spi::InternalLoggingEvent& event);
	private:
		/// this is used to convert numeric log levels into strings
		log4cplus::LogLevelManager		m_log_level_manager;
#elif defined(PION_HAVE_LOG4CPP)
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
/// LogModule: module that displays log messages
/// 
class LogModule :
	public pion::HTTPModule
{
public:
	// default constructor and destructor
	LogModule(void);
	virtual ~LogModule();
	
	/// handles a new HTTP request
	virtual bool handleRequest(pion::HTTPRequestPtr& request,
							   pion::TCPConnectionPtr& tcp_conn);

	/// returns the log appender used by LogModule
	inline LogModuleAppender& getLogAppender(void) { return *m_log_appender_ptr; }
	
private:
	/// map of file extensions to MIME types
	LogModuleAppender *		m_log_appender_ptr;
};

#endif
