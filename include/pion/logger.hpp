// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_LOGGER_HEADER__
#define __PION_LOGGER_HEADER__

#include <pion/config.hpp>


#if defined(PION_USE_LOG4CXX)

    // unfortunately, the current version of log4cxx has many problems that
    // produce very annoying warnings

    // log4cxx headers
    #include <log4cxx/logger.h>
    #include <log4cxx/logmanager.h>
#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4231) // nonstandard extension used : 'extern' before template explicit instantiation
#endif
    #include <log4cxx/basicconfigurator.h>
    #include <log4cxx/propertyconfigurator.h>
#ifdef _MSC_VER
    #pragma warning(pop)
#endif

    #if defined _MSC_VER
        #if defined _DEBUG
            #pragma comment(lib, "log4cxxd")
        #else
            #pragma comment(lib, "log4cxx")
        #endif
        #pragma comment(lib, "odbc32")
    #endif

    namespace pion {
        typedef log4cxx::LoggerPtr  logger;
        typedef log4cxx::AppenderSkeleton   log_appender;
        typedef log_appender *   log_appender_ptr;
    }

    #define PION_HAS_LOG_APPENDER   1
    #define PION_LOG_CONFIG_BASIC   log4cxx::BasicConfigurator::configure();
    #define PION_LOG_CONFIG(FILE)   log4cxx::PropertyConfigurator::configure(FILE);
    #define PION_GET_LOGGER(NAME)   log4cxx::Logger::get_logger(NAME)
    #define PION_SHUTDOWN_LOGGER    log4cxx::LogManager::shutdown();

    #define PION_LOG_SETLEVEL_DEBUG(LOG)    LOG->setLevel(log4cxx::Level::toLevel(log4cxx::Level::DEBUG_INT));
    #define PION_LOG_SETLEVEL_INFO(LOG)     LOG->setLevel(log4cxx::Level::toLevel(log4cxx::Level::INFO_INT));
    #define PION_LOG_SETLEVEL_WARN(LOG)     LOG->setLevel(log4cxx::Level::toLevel(log4cxx::Level::WARN_INT));
    #define PION_LOG_SETLEVEL_ERROR(LOG)    LOG->setLevel(log4cxx::Level::toLevel(log4cxx::Level::ERROR_INT));
    #define PION_LOG_SETLEVEL_FATAL(LOG)    LOG->setLevel(log4cxx::Level::toLevel(log4cxx::Level::FATAL_INT));
    #define PION_LOG_SETLEVEL_UP(LOG)       LOG->setLevel(LOG->getLevel()->toInt()+1);
    #define PION_LOG_SETLEVEL_DOWN(LOG)     LOG->setLevel(LOG->getLevel()->toInt()-1);

    #define PION_LOG_DEBUG  LOG4CXX_DEBUG
    #define PION_LOG_INFO   LOG4CXX_INFO
    #define PION_LOG_WARN   LOG4CXX_WARN
    #define PION_LOG_ERROR  LOG4CXX_ERROR
    #define PION_LOG_FATAL  LOG4CXX_FATAL

#elif defined(PION_USE_LOG4CPLUS)


    // log4cplus headers
    #include <log4cplus/logger.h>
    #include <log4cplus/configurator.h>
    #include <log4cplus/appender.h>
    #include <log4cplus/spi/loggingevent.h>
    #include <log4cplus/loglevel.h>
    #include <log4cplus/loggingmacros.h>

    #include <boost/circular_buffer.hpp>
    #include <boost/thread/mutex.hpp>

    #if defined(_MSC_VER) && !defined(PION_CMAKE_BUILD)
        #if defined _DEBUG
            #if defined PION_STATIC_LINKING
                #pragma comment(lib, "log4cplusSD")
            #else
                #pragma comment(lib, "log4cplusD")
            #endif
        #else
            #if defined PION_STATIC_LINKING
                #pragma comment(lib, "log4cplusS")
            #else
                #pragma comment(lib, "log4cplus")
            #endif
        #endif
    #endif

    namespace pion {
        typedef log4cplus::Logger   logger;
        typedef log4cplus::Appender log_appender;
        typedef log4cplus::SharedAppenderPtr    log_appender_ptr;

        ///
        /// circular_buffer_appender: caches log events in a circular buffer
        ///
        class circular_buffer_appender : public log4cplus::Appender
        {
        public:
            typedef boost::circular_buffer<log4cplus::spi::InternalLoggingEvent> LogEventBuffer;

            // default constructor and destructor
            circular_buffer_appender(void) : m_log_events(1000) {};
            virtual ~circular_buffer_appender() {}

            /// returns an iterator to the log events in the buffer
            const LogEventBuffer& getLogIterator() const {
                return m_log_events;
            }

        public:
            // member functions inherited from the Appender interface class
            virtual void close() {}
        protected:
            virtual void append(const log4cplus::spi::InternalLoggingEvent& event) {
                boost::mutex::scoped_lock log_lock(m_log_mutex);
                m_log_events.push_back(*event.clone());
            }

        private:
            /// circular buffer for log events
            LogEventBuffer  m_log_events;

            /// mutex to make class thread-safe
            boost::mutex    m_log_mutex;
        };
    }

    #define PION_HAS_LOG_APPENDER   1
    #define PION_LOG_CONFIG_BASIC   log4cplus::BasicConfigurator::doConfigure();
    #define PION_LOG_CONFIG(FILE)   log4cplus::PropertyConfigurator::doConfigure(FILE);
    #define PION_GET_LOGGER(NAME)   log4cplus::Logger::getInstance(NAME)
    #define PION_SHUTDOWN_LOGGER    log4cplus::Logger::shutdown();

    #define PION_LOG_SETLEVEL_DEBUG(LOG)    LOG.setLogLevel(log4cplus::DEBUG_LOG_LEVEL);
    #define PION_LOG_SETLEVEL_INFO(LOG)     LOG.setLogLevel(log4cplus::INFO_LOG_LEVEL);
    #define PION_LOG_SETLEVEL_WARN(LOG)     LOG.setLogLevel(log4cplus::WARN_LOG_LEVEL);
    #define PION_LOG_SETLEVEL_ERROR(LOG)    LOG.setLogLevel(log4cplus::ERROR_LOG_LEVEL);
    #define PION_LOG_SETLEVEL_FATAL(LOG)    LOG.setLogLevel(log4cplus::FATAL_LOG_LEVEL);
    #define PION_LOG_SETLEVEL_UP(LOG)       LOG.setLogLevel(LOG.getLogLevel()+1);
    #define PION_LOG_SETLEVEL_DOWN(LOG)     LOG.setLogLevel(LOG.getLogLevel()-1);

    #define PION_LOG_DEBUG  LOG4CPLUS_DEBUG
    #define PION_LOG_INFO   LOG4CPLUS_INFO
    #define PION_LOG_WARN   LOG4CPLUS_WARN
    #define PION_LOG_ERROR  LOG4CPLUS_ERROR
    #define PION_LOG_FATAL  LOG4CPLUS_FATAL


#elif defined(PION_USE_LOG4CPP)


    // log4cpp headers
    #include <log4cpp/Category.hh>
    #include <log4cpp/BasicLayout.hh>
    #include <log4cpp/OstreamAppender.hh>
    #include <log4cpp/AppenderSkeleton.hh>

    namespace pion {
        typedef log4cpp::Category*  logger;
        typedef log4cpp::AppenderSkeleton   log_appender;
        typedef log_appender *   log_appender_ptr;
    }

    #define PION_HAS_LOG_APPENDER   1
    #define PION_LOG_CONFIG_BASIC   { log4cpp::OstreamAppender *app = new log4cpp::OstreamAppender("cout", &std::cout); app->setLayout(new log4cpp::BasicLayout()); log4cpp::Category::getRoot().setAppender(app); }
    #define PION_LOG_CONFIG(FILE)   { log4cpp::PropertyConfigurator::configure(FILE); }
    #define PION_GET_LOGGER(NAME)   (&log4cpp::Category::getInstance(NAME))
    #define PION_SHUTDOWN_LOGGER    log4cpp::Category::shutdown();

    #define PION_LOG_SETLEVEL_DEBUG(LOG)    { LOG->setPriority(log4cpp::Priority::DEBUG); }
    #define PION_LOG_SETLEVEL_INFO(LOG)     { LOG->setPriority(log4cpp::Priority::INFO); }
    #define PION_LOG_SETLEVEL_WARN(LOG)     { LOG->setPriority(log4cpp::Priority::WARN); }
    #define PION_LOG_SETLEVEL_ERROR(LOG)    { LOG->setPriority(log4cpp::Priority::ERROR); }
    #define PION_LOG_SETLEVEL_FATAL(LOG)    { LOG->setPriority(log4cpp::Priority::FATAL); }
    #define PION_LOG_SETLEVEL_UP(LOG)       { LOG->setPriority(LOG.getPriority()+1); }
    #define PION_LOG_SETLEVEL_DOWN(LOG)     { LOG->setPriority(LOG.getPriority()-1); }

    #define PION_LOG_DEBUG(LOG, MSG)    if (LOG->getPriority()>=log4cpp::Priority::DEBUG) { LOG->debugStream() << MSG; }
    #define PION_LOG_INFO(LOG, MSG)     if (LOG->getPriority()>=log4cpp::Priority::INFO) { LOG->infoStream() << MSG; }
    #define PION_LOG_WARN(LOG, MSG)     if (LOG->getPriority()>=log4cpp::Priority::WARN) { LOG->warnStream() << MSG; }
    #define PION_LOG_ERROR(LOG, MSG)    if (LOG->getPriority()>=log4cpp::Priority::ERROR) { LOG->errorStream() << MSG; }
    #define PION_LOG_FATAL(LOG, MSG)    if (LOG->getPriority()>=log4cpp::Priority::FATAL) { LOG->fatalStream() << MSG; }

#elif defined(PION_DISABLE_LOGGING)

    // Logging is disabled -> add do-nothing stubs for logging
    namespace pion {
        struct PION_API logger {
            logger(int /* glog */) {}
            operator bool() const { return false; }
            static void shutdown() {}
        };
        typedef int     log_appender;
        typedef log_appender *   log_appender_ptr;
    }

    #undef PION_HAS_LOG_APPENDER
    #define PION_LOG_CONFIG_BASIC   {}
    #define PION_LOG_CONFIG(FILE)   {}
    #define PION_GET_LOGGER(NAME)   0
    #define PION_SHUTDOWN_LOGGER    0

    // use LOG to avoid warnings about LOG not being used
    #define PION_LOG_SETLEVEL_DEBUG(LOG)    { if (LOG) {} }
    #define PION_LOG_SETLEVEL_INFO(LOG)     { if (LOG) {} }
    #define PION_LOG_SETLEVEL_WARN(LOG)     { if (LOG) {} }
    #define PION_LOG_SETLEVEL_ERROR(LOG)    { if (LOG) {} }
    #define PION_LOG_SETLEVEL_FATAL(LOG)    { if (LOG) {} }
    #define PION_LOG_SETLEVEL_UP(LOG)       { if (LOG) {} }
    #define PION_LOG_SETLEVEL_DOWN(LOG)     { if (LOG) {} }

    #define PION_LOG_DEBUG(LOG, MSG)    { if (LOG) {} }
    #define PION_LOG_INFO(LOG, MSG)     { if (LOG) {} }
    #define PION_LOG_WARN(LOG, MSG)     { if (LOG) {} }
    #define PION_LOG_ERROR(LOG, MSG)    { if (LOG) {} }
    #define PION_LOG_FATAL(LOG, MSG)    { if (LOG) {} }
#else

    #define PION_USE_OSTREAM_LOGGING

    // Logging uses std::cout and std::cerr
    #include <iostream>
    #include <string>
    #include <ctime>

    namespace pion {
        struct PION_API logger {
            enum log_priority_type {
                LOG_LEVEL_DEBUG, LOG_LEVEL_INFO, LOG_LEVEL_WARN,
                LOG_LEVEL_ERROR, LOG_LEVEL_FATAL
            };
            ~logger() {}
            logger(void) : m_name("pion") {}
            logger(const std::string& name) : m_name(name) {}
            logger(const logger& p) : m_name(p.m_name) {}
            static void shutdown() {}
            std::string                 m_name;
            static log_priority_type     m_priority;
        };
        typedef int     log_appender;
        typedef log_appender *   log_appender_ptr;
    }

    #undef PION_HAS_LOG_APPENDER
    #define PION_LOG_CONFIG_BASIC   {}
    #define PION_LOG_CONFIG(FILE)   {}
    #define PION_GET_LOGGER(NAME)   pion::logger(NAME)
    #define PION_SHUTDOWN_LOGGER    {}

    #define PION_LOG_SETLEVEL_DEBUG(LOG)    { LOG.m_priority = pion::logger::LOG_LEVEL_DEBUG; }
    #define PION_LOG_SETLEVEL_INFO(LOG)     { LOG.m_priority = pion::logger::LOG_LEVEL_INFO; }
    #define PION_LOG_SETLEVEL_WARN(LOG)     { LOG.m_priority = pion::logger::LOG_LEVEL_WARN; }
    #define PION_LOG_SETLEVEL_ERROR(LOG)    { LOG.m_priority = pion::logger::LOG_LEVEL_ERROR; }
    #define PION_LOG_SETLEVEL_FATAL(LOG)    { LOG.m_priority = pion::logger::LOG_LEVEL_FATAL; }
    #define PION_LOG_SETLEVEL_UP(LOG)       { ++LOG.m_priority; }
    #define PION_LOG_SETLEVEL_DOWN(LOG)     { --LOG.m_priority; }

    #define PION_LOG_DEBUG(LOG, MSG)    if (LOG.m_priority <= pion::logger::LOG_LEVEL_DEBUG) { std::cout << time(NULL) << " DEBUG " << LOG.m_name << ' ' << MSG << std::endl; }
    #define PION_LOG_INFO(LOG, MSG)     if (LOG.m_priority <= pion::logger::LOG_LEVEL_INFO) { std::cout << time(NULL) << " INFO " << LOG.m_name << ' ' << MSG << std::endl; }
    #define PION_LOG_WARN(LOG, MSG)     if (LOG.m_priority <= pion::logger::LOG_LEVEL_WARN) { std::cerr << time(NULL) << " WARN " << LOG.m_name << ' ' << MSG << std::endl; }
    #define PION_LOG_ERROR(LOG, MSG)    if (LOG.m_priority <= pion::logger::LOG_LEVEL_ERROR) { std::cerr << time(NULL) << " ERROR " << LOG.m_name << ' ' << MSG << std::endl; }
    #define PION_LOG_FATAL(LOG, MSG)    if (LOG.m_priority <= pion::logger::LOG_LEVEL_FATAL) { std::cerr << time(NULL) << " FATAL " << LOG.m_name << ' ' << MSG << std::endl; }

#endif

#endif
