// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <signal.h>
#ifdef _MSC_VER
    #include <windows.h>
#else
    #include <fcntl.h>
    #include <unistd.h>
    #include <sys/stat.h>
#endif

#include <pion/config.hpp>
#include <pion/process.hpp>


namespace pion {    // begin namespace pion
    
// static members of process
    
boost::once_flag                process::m_instance_flag = BOOST_ONCE_INIT;
process::config_type *process::m_config_ptr = NULL;

    
// process member functions
    
void process::shutdown(void)
{
    config_type& cfg = get_config();
    boost::mutex::scoped_lock shutdown_lock(cfg.shutdown_mutex);
    if (! cfg.shutdown_now) {
        cfg.shutdown_now = true;
        cfg.shutdown_cond.notify_all();
    }
}

void process::wait_for_shutdown(void)
{
    config_type& cfg = get_config();
    boost::mutex::scoped_lock shutdown_lock(cfg.shutdown_mutex);
    while (! cfg.shutdown_now)
        cfg.shutdown_cond.wait(shutdown_lock);
}

void process::create_config(void)
{
    static config_type UNIQUE_PION_PROCESS_CONFIG;
    m_config_ptr = &UNIQUE_PION_PROCESS_CONFIG;
}



#ifdef _MSC_VER

BOOL WINAPI console_ctrl_handler(DWORD ctrl_type)
{
    switch(ctrl_type) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            process::shutdown();
            return TRUE;
        default:
            return FALSE;
    }
}

void process::initialize(void)
{
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
}

void process::daemonize(void)
{
    // not supported
}

#else   // NOT #ifdef _MSC_VER

void handle_signal(int sig)
{
    process::shutdown();
}

void process::initialize(void)
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
}

void process::daemonize(void)
{
    // adopted from "Unix Daemon Server Programming"
    // http://www.enderunix.org/docs/eng/daemon.php
    
    // return early if already running as a daemon
    if(getppid()==1) return;
    
    // for out the process 
    int i = fork();
    if (i<0) exit(1);   // error forking
    if (i>0) exit(0);   // exit if parent
    
    // child (daemon process) continues here after the fork...
    
    // obtain a new process group
    setsid();
    
    // close all descriptors
    for (i=getdtablesize();i>=0;--i) close(i);
    
    // bind stdio to /dev/null (ignore errors)
    i=open("/dev/null",O_RDWR);
    if (i != -1) {
        if (dup(i) == -1) {}
        if (dup(i) == -1) {}
    }
    
    // restrict file creation mode to 0750
    umask(027);
}

#endif  // #ifdef _MSC_VER

}   // end namespace pion
