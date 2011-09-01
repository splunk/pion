// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007-2011 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <signal.h>
#ifdef _MSC_VER
	#include <windows.h>
#endif

#include <pion/PionConfig.hpp>
#include <pion/PionProcess.hpp>


namespace pion {	// begin namespace pion
	
// static members of PionProcess
	
boost::once_flag				PionProcess::m_instance_flag = BOOST_ONCE_INIT;
PionProcess::PionProcessConfig *PionProcess::m_config_ptr = NULL;

	
// PionProcess member functions
	
void PionProcess::shutdown(void)
{
	PionProcessConfig& cfg = getPionProcessConfig();
	boost::mutex::scoped_lock shutdown_lock(cfg.shutdown_mutex);
	if (! cfg.shutdown_now) {
		cfg.shutdown_now = true;
		cfg.shutdown_cond.notify_all();
	}
}

void PionProcess::wait_for_shutdown(void)
{
	PionProcessConfig& cfg = getPionProcessConfig();
	boost::mutex::scoped_lock shutdown_lock(cfg.shutdown_mutex);
	while (! cfg.shutdown_now)
		cfg.shutdown_cond.wait(shutdown_lock);
}

void PionProcess::createPionProcessConfig(void)
{
	static PionProcessConfig UNIQUE_PION_PROCESS_CONFIG;
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
			PionProcess::shutdown();
			return TRUE;
		default:
			return FALSE;
	}
}

void PionProcess::initialize(void)
{
	SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
}

#else	// NOT #ifdef _MSC_VER

void handle_signal(int sig)
{
	PionProcess::shutdown();
}

void PionProcess::initialize(void)
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

#endif	// #ifdef _MSC_VER

}	// end namespace pion
