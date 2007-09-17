// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include "LogModule.hpp"

#if defined(PION_HAVE_LOG4CXX)
	#include <log4cxx/spi/loggingevent.h>
	#include <boost/lexical_cast.hpp>
#elif defined(PION_HAVE_LOG4CPLUS)
	#include <log4cplus/spi/loggingevent.h>
	#include <boost/lexical_cast.hpp>
#elif defined(PION_HAVE_LOG4CPP)
	#include <log4cpp/BasicLayout.hh>
#endif

#include <pion/net/HTTPResponse.hpp>


using namespace pion;
using namespace pion::net;


// static members of LogModuleAppender

const unsigned int		LogModuleAppender::DEFAULT_MAX_EVENTS = 25;


// LogModuleAppender member functions

#if defined(PION_HAVE_LOG4CPP)
LogModuleAppender::LogModuleAppender(void)
	: log4cpp::AppenderSkeleton("LogModuleAppender"),
	m_max_events(DEFAULT_MAX_EVENTS), m_num_events(0),
	m_layout_ptr(new log4cpp::BasicLayout())
	{}
#else
LogModuleAppender::LogModuleAppender(void)
	: m_max_events(DEFAULT_MAX_EVENTS), m_num_events(0)
	{}
#endif


#if defined(PION_HAVE_LOG4CXX)
void LogModuleAppender::append(const log4cxx::spi::LoggingEventPtr& event)
{
	// custom layouts is not supported for log4cxx library
	std::string formatted_string(boost::lexical_cast<std::string>(event->getTimeStamp()));
	formatted_string += ' ';
	formatted_string += event->getLevel()->toString();
	formatted_string += ' ';
	formatted_string += event->getLoggerName();
	formatted_string += " - ";
	formatted_string += event->getRenderedMessage();
	formatted_string += '\n';
	addLogString(formatted_string);
}
#elif defined(PION_HAVE_LOG4CPLUS)
void LogModuleAppender::append(const log4cplus::spi::InternalLoggingEvent& event)
{
	// custom layouts is not supported for log4cplus library
	std::string formatted_string(boost::lexical_cast<std::string>(event.getTimestamp().sec()));
	formatted_string += ' ';
	formatted_string += m_log_level_manager.toString(event.getLogLevel());
	formatted_string += ' ';
	formatted_string += event.getLoggerName();
	formatted_string += " - ";
	formatted_string += event.getMessage();
	formatted_string += '\n';
	addLogString(formatted_string);
}
#elif defined(PION_HAVE_LOG4CPP)
void LogModuleAppender::_append(const log4cpp::LoggingEvent& event)
{
	std::string formatted_string(m_layout_ptr->format(event));
	addLogString(formatted_string);
}
#endif

void LogModuleAppender::addLogString(const std::string& log_string)
{
	boost::mutex::scoped_lock log_lock(m_log_mutex);
	m_log_events.push_back(log_string);
	++m_num_events;
	while (m_num_events > m_max_events) {
		m_log_events.erase(m_log_events.begin());
		--m_num_events;
	}
}

void LogModuleAppender::writeLogEvents(pion::net::HTTPResponsePtr& response)
{
#if defined(PION_HAVE_LOG4CXX) || defined(PION_HAVE_LOG4CPLUS) || defined(PION_HAVE_LOG4CPP)
	boost::mutex::scoped_lock log_lock(m_log_mutex);
	for (std::list<std::string>::const_iterator i = m_log_events.begin();
		 i != m_log_events.end(); ++i)
	{
		response << *i;
	}
#else
	response << "Logging is disabled." << HTTPTypes::STRING_CRLF;
#endif
}


// LogModule member functions

LogModule::LogModule(void)
	: m_log_appender_ptr(new LogModuleAppender())
{
#if defined(PION_HAVE_LOG4CXX)
	m_log_appender_ptr->setName("LogModuleAppender");
	log4cxx::Logger::getRootLogger()->addAppender(m_log_appender_ptr);
#elif defined(PION_HAVE_LOG4CPLUS)
	m_log_appender_ptr->setName("LogModuleAppender");
	log4cplus::Logger::getRoot().addAppender(m_log_appender_ptr);
#elif defined(PION_HAVE_LOG4CPP)
	log4cpp::Category::getRoot().addAppender(m_log_appender_ptr);
#endif
}

LogModule::~LogModule()
{
#if defined(PION_HAVE_LOG4CXX)
	// removeAppender() also deletes the object
	log4cxx::Logger::getRootLogger()->removeAppender(m_log_appender_ptr);
#elif defined(PION_HAVE_LOG4CPLUS)
	// removeAppender() also deletes the object
	log4cplus::Logger::getRoot().removeAppender("LogModuleAppender");
#elif defined(PION_HAVE_LOG4CPP)
	// removeAppender() also deletes the object
	log4cpp::Category::getRoot().removeAppender(m_log_appender_ptr);
#else
	delete m_log_appender_ptr;
#endif
}

/// handles requests for LogModule
bool LogModule::handleRequest(HTTPRequestPtr& request, TCPConnectionPtr& tcp_conn)
{
	// Set Content-type to "text/plain" (plain ascii text)
	HTTPResponsePtr response(HTTPResponse::create());
	response->setContentType(HTTPTypes::CONTENT_TYPE_TEXT);
	getLogAppender().writeLogEvents(response);
	response->send(tcp_conn);
	return true;
}


/// creates new LogModule objects
extern "C" PION_PLUGIN_API LogModule *pion_create_LogModule(void)
{
	return new LogModule();
}


/// destroys LogModule objects
extern "C" PION_PLUGIN_API void pion_destroy_LogModule(LogModule *module_ptr)
{
	delete module_ptr;
}
