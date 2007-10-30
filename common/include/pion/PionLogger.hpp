// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PIONLOGGER_HEADER__
#define __PION_PIONLOGGER_HEADER__

#include <pion/PionConfig.hpp>


#if defined(PION_USE_LOG4CXX)

	// unfortunately, the current version of log4cxx has many problems that
	// produce very annoying warnings

	// this disables warnings before preprocessing the log4cxx headers
	#if defined __GNUC__
		#pragma GCC system_header
	#elif defined __SUNPRO_CC
		#pragma disable_warn
	#elif defined _MSC_VER
		#pragma warning(push, 1)
	#endif 

	// log4cxx headers
	#include <log4cxx/logger.h>
	#include <log4cxx/basicconfigurator.h>

	// this re-enables warnings
	#if defined __SUNPRO_CC
		#pragma enable_warn
	#elif defined _MSC_VER
		#pragma warning(pop)
	#endif 

	#if defined _MSC_VER
		#if defined _DEBUG
			#pragma comment(lib, "log4cxxd")
			#pragma comment(lib, "aprd")
		#else
			#pragma comment(lib, "log4cxx")
			#pragma comment(lib, "apr")
		#endif
		#pragma comment(lib, "odbc32")
	#endif 

	namespace pion {
		typedef log4cxx::LoggerPtr	PionLogger;
	}

	#define PION_LOG_CONFIG_BASIC	log4cxx::BasicConfigurator::configure();
	#define PION_GET_LOGGER(NAME)	log4cxx::Logger::getLogger(NAME)

	#define PION_LOG_SETLEVEL_DEBUG(LOG)	LOG->setLevel(log4cxx::Level::toLevel(log4cxx::Level::DEBUG_INT));
	#define PION_LOG_SETLEVEL_INFO(LOG)		LOG->setLevel(log4cxx::Level::toLevel(log4cxx::Level::INFO_INT));
	#define PION_LOG_SETLEVEL_WARN(LOG)		LOG->setLevel(log4cxx::Level::toLevel(log4cxx::Level::WARN_INT));
	#define PION_LOG_SETLEVEL_ERROR(LOG)	LOG->setLevel(log4cxx::Level::toLevel(log4cxx::Level::ERROR_INT));
	#define PION_LOG_SETLEVEL_FATAL(LOG)	LOG->setLevel(log4cxx::Level::toLevel(log4cxx::Level::FATAL_INT));

	#define PION_LOG_DEBUG	LOG4CXX_DEBUG
	#define PION_LOG_INFO	LOG4CXX_INFO
	#define PION_LOG_WARN	LOG4CXX_WARN
	#define PION_LOG_ERROR	LOG4CXX_ERROR
	#define PION_LOG_FATAL	LOG4CXX_FATAL

#elif defined(PION_USE_LOG4CPLUS)


	// log4cplus headers
	#include <log4cplus/logger.h>
	#include <log4cplus/configurator.h>

	namespace pion {
		typedef log4cplus::Logger	PionLogger;
	}

	#define PION_LOG_CONFIG_BASIC	log4cplus::BasicConfigurator::doConfigure();
	#define PION_GET_LOGGER(NAME)	log4cplus::Logger::getInstance(NAME)

	#define PION_LOG_SETLEVEL_DEBUG(LOG)	LOG.setLogLevel(log4cplus::DEBUG_LOG_LEVEL);
	#define PION_LOG_SETLEVEL_INFO(LOG)		LOG.setLogLevel(log4cplus::INFO_LOG_LEVEL);
	#define PION_LOG_SETLEVEL_WARN(LOG)		LOG.setLogLevel(log4cplus::WARN_LOG_LEVEL);
	#define PION_LOG_SETLEVEL_ERROR(LOG)	LOG.setLogLevel(log4cplus::ERROR_LOG_LEVEL);
	#define PION_LOG_SETLEVEL_FATAL(LOG)	LOG.setLogLevel(log4cplus::FATAL_LOG_LEVEL);

	#define PION_LOG_DEBUG	LOG4CPLUS_DEBUG
	#define PION_LOG_INFO	LOG4CPLUS_INFO
	#define PION_LOG_WARN	LOG4CPLUS_WARN
	#define PION_LOG_ERROR	LOG4CPLUS_ERROR
	#define PION_LOG_FATAL	LOG4CPLUS_FATAL


#elif defined(PION_USE_LOG4CPP)


	// log4cpp headers
	#include <log4cpp/Category.hh>
	#include <log4cpp/BasicLayout.hh>
	#include <log4cpp/OstreamAppender.hh>

	namespace pion {
		typedef log4cpp::Category*	PionLogger;
	}

	#define PION_LOG_CONFIG_BASIC	{ log4cpp::OstreamAppender *app = new log4cpp::OstreamAppender("cout", &std::cout); app->setLayout(new log4cpp::BasicLayout()); log4cpp::Category::getRoot().setAppender(app); }
	#define PION_GET_LOGGER(NAME)	(&log4cpp::Category::getInstance(NAME))

	#define PION_LOG_SETLEVEL_DEBUG(LOG)	{ LOG->setPriority(log4cpp::Priority::DEBUG); }
	#define PION_LOG_SETLEVEL_INFO(LOG)		{ LOG->setPriority(log4cpp::Priority::INFO); }
	#define PION_LOG_SETLEVEL_WARN(LOG)		{ LOG->setPriority(log4cpp::Priority::WARN); }
	#define PION_LOG_SETLEVEL_ERROR(LOG)	{ LOG->setPriority(log4cpp::Priority::ERROR); }
	#define PION_LOG_SETLEVEL_FATAL(LOG)	{ LOG->setPriority(log4cpp::Priority::FATAL); }

	#define PION_LOG_DEBUG(LOG, MSG)	if (LOG->getPriority()>=log4cpp::Priority::DEBUG) { LOG->debugStream() << MSG; }
	#define PION_LOG_INFO(LOG, MSG)		if (LOG->getPriority()>=log4cpp::Priority::INFO) { LOG->infoStream() << MSG; }
	#define PION_LOG_WARN(LOG, MSG)		if (LOG->getPriority()>=log4cpp::Priority::WARN) { LOG->warnStream() << MSG; }
	#define PION_LOG_ERROR(LOG, MSG)	if (LOG->getPriority()>=log4cpp::Priority::ERROR) { LOG->errorStream() << MSG; }
	#define PION_LOG_FATAL(LOG, MSG)	if (LOG->getPriority()>=log4cpp::Priority::FATAL) { LOG->fatalStream() << MSG; }

#elif defined(PION_DISABLE_LOGGING)

	// Logging is disabled -> add do-nothing stubs for logging
	namespace pion {
		typedef int		PionLogger;
	}

	#define PION_LOG_CONFIG_BASIC	{}
	#define PION_GET_LOGGER(NAME)	0

	// use "++LOG" to avoid warnings about LOG not being used
	#define PION_LOG_SETLEVEL_DEBUG(LOG)	{ if (false) ++LOG; }
	#define PION_LOG_SETLEVEL_INFO(LOG)		{ if (false) ++LOG; }
	#define PION_LOG_SETLEVEL_WARN(LOG)		{ if (false) ++LOG; }
	#define PION_LOG_SETLEVEL_ERROR(LOG)	{ if (false) ++LOG; }
	#define PION_LOG_SETLEVEL_FATAL(LOG)	{ if (false) ++LOG; }

	// use "++LOG" to avoid warnings about LOG not being used
	#define PION_LOG_DEBUG(LOG, MSG)	{ if (false) ++LOG; }
	#define PION_LOG_INFO(LOG, MSG)		{ if (false) ++LOG; }
	#define PION_LOG_WARN(LOG, MSG)		{ if (false) ++LOG; }
	#define PION_LOG_ERROR(LOG, MSG)	{ if (false) ++LOG; }
	#define PION_LOG_FATAL(LOG, MSG)	{ if (false) ++LOG; }

#else

	#define PION_USE_OSTREAM_LOGGING

	// Logging uses std::cout and std::cerr
	#include <iostream>
	#include <string>
	#include <ctime>

	namespace pion {
		struct PION_COMMON_API PionLogger {
			enum PionPriorityType {
				LOG_LEVEL_DEBUG, LOG_LEVEL_INFO, LOG_LEVEL_WARN,
				LOG_LEVEL_ERROR, LOG_LEVEL_FATAL
			};
			~PionLogger() {}
			PionLogger(void) : m_name("pion") {}
			PionLogger(const std::string& name) : m_name(name) {}
			PionLogger(const PionLogger& p) : m_name(p.m_name) {}
			std::string					m_name;
			static PionPriorityType		m_priority;
		};
	}

	#define PION_LOG_CONFIG_BASIC	{}
	#define PION_GET_LOGGER(NAME)	pion::PionLogger(NAME)

	#define PION_LOG_SETLEVEL_DEBUG(LOG)	{ LOG.m_priority = pion::PionLogger::LOG_LEVEL_DEBUG; }
	#define PION_LOG_SETLEVEL_INFO(LOG)		{ LOG.m_priority = pion::PionLogger::LOG_LEVEL_INFO; }
	#define PION_LOG_SETLEVEL_WARN(LOG)		{ LOG.m_priority = pion::PionLogger::LOG_LEVEL_WARN; }
	#define PION_LOG_SETLEVEL_ERROR(LOG)	{ LOG.m_priority = pion::PionLogger::LOG_LEVEL_ERROR; }
	#define PION_LOG_SETLEVEL_FATAL(LOG)	{ LOG.m_priority = pion::PionLogger::LOG_LEVEL_FATAL; }

	#define PION_LOG_DEBUG(LOG, MSG)	if (LOG.m_priority <= pion::PionLogger::LOG_LEVEL_DEBUG) { std::cout << time(NULL) << " DEBUG " << LOG.m_name << ' ' << MSG << std::endl; }
	#define PION_LOG_INFO(LOG, MSG)		if (LOG.m_priority <= pion::PionLogger::LOG_LEVEL_INFO) { std::cout << time(NULL) << " INFO " << LOG.m_name << ' ' << MSG << std::endl; }
	#define PION_LOG_WARN(LOG, MSG)		if (LOG.m_priority <= pion::PionLogger::LOG_LEVEL_WARN) { std::cerr << time(NULL) << " WARN " << LOG.m_name << ' ' << MSG << std::endl; }
	#define PION_LOG_ERROR(LOG, MSG)	if (LOG.m_priority <= pion::PionLogger::LOG_LEVEL_ERROR) { std::cerr << time(NULL) << " ERROR " << LOG.m_name << ' ' << MSG << std::endl; }
	#define PION_LOG_FATAL(LOG, MSG)	if (LOG.m_priority <= pion::PionLogger::LOG_LEVEL_FATAL) { std::cerr << time(NULL) << " FATAL " << LOG.m_name << ' ' << MSG << std::endl; }

#endif

#endif
