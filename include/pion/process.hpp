// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PROCESS_HEADER__
#define __PION_PROCESS_HEADER__

#include <string>
#include <boost/noncopyable.hpp>
#include <boost/thread/once.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <pion/config.hpp>

// Dump file generation support on Windows
#ifdef PION_WIN32
#include <windows.h>
#include <tchar.h>
#include <DbgHelp.h>
// based on dbghelp.h
typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
									CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
									CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
									CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
#endif

namespace pion {    // begin namespace pion

///
/// process: class for managing process/service related functions
///
class PION_API process :
    private boost::noncopyable
{
public:

    // default destructor
    ~process() {}
    
    /// default constructor
    process(void) {}
    
    /// signals the shutdown condition
    static void shutdown(void);
    
    /// blocks until the shutdown condition has been signaled
    static void wait_for_shutdown(void);

    /// sets up basic signal handling for the process
    static void initialize(void);
    
    /// fork process and run as a background daemon
    static void daemonize(void);

#ifdef PION_WIN32

    class dumpfile_init_exception : public std::exception
    {
    public:
        dumpfile_init_exception(const std::string& cause) : m_cause(cause) {}

        virtual const char* what() const { return m_cause.c_str(); }
    protected:
        std::string m_cause;
    };

    /**
     * enables mini-dump generation on unhandled exceptions (AVs, etc.)
     * throws an dumpfile_init_exception if unable to set the unhandled exception processing
     * @param dir file system path to store mini dumps  
     */ 
    static void set_dumpfile_directory(const std::string& dir);

protected:
    /// unhandled exception filter proc
    static LONG WINAPI unhandled_exception_filter(struct _EXCEPTION_POINTERS *pExceptionInfo);

    /// generates a name for a dump file
    static std::string generate_dumpfile_name();
#endif

protected:

    /// data type for static/global process configuration information
    struct config_type {
        /// constructor just initializes native types
#ifdef PION_WIN32
        config_type() : shutdown_now(false), h_dbghelp(NULL), p_dump_proc(NULL) {}
#else
        config_type() : shutdown_now(false) {}
#endif
    
        /// true if we should shutdown now
        bool                    shutdown_now;
        
        /// triggered when it is time to shutdown
        boost::condition        shutdown_cond;

        /// used to protect the shutdown condition
        boost::mutex            shutdown_mutex;

// Dump file generation support on Windows
#ifdef PION_WIN32
        /// mini-dump file location
        std::string             dumpfile_dir;
        
        /// dbghelp.dll library handle
        HMODULE                 h_dbghelp;

        /// address of MiniDumpWriteDump inside dbghelp.dll
        MINIDUMPWRITEDUMP       p_dump_proc;
#endif
    };

    /// returns a singleton instance of config_type
    static inline config_type& get_config(void) {
        boost::call_once(process::create_config, m_instance_flag);
        return *m_config_ptr;
    }


private:

    /// creates the config_type singleton
    static void create_config(void);

    
    /// used to ensure thread safety of the config_type singleton
    static boost::once_flag             m_instance_flag;

    /// pointer to the config_type singleton
    static config_type *          m_config_ptr;
};


}   // end namespace pion

#endif
