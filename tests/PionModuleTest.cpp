// -----------------------------------------------------------------
// libpion: a C++ framework for building lightweight HTTP interfaces
// -----------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/bind.hpp>
#include <libpion/Pion.hpp>
#include <iostream>
#include <vector>
#ifndef PION_WIN32
	#include <signal.h>
#endif

// these are used only when linking to static HTTP module libraries
// #ifdef PION_STATIC_LINKING
PION_DECLARE_PLUGIN(EchoModule)
PION_DECLARE_PLUGIN(FileModule)
PION_DECLARE_PLUGIN(HelloModule)
PION_DECLARE_PLUGIN(LogModule)
PION_DECLARE_PLUGIN(CookieModule)

using namespace std;
using namespace pion;

/// stops Pion when it receives signals
#ifdef PION_WIN32
BOOL WINAPI console_ctrl_handler(DWORD ctrl_type)
{
	switch(ctrl_type) {
		case CTRL_C_EVENT:
		case CTRL_BREAK_EVENT:
		case CTRL_CLOSE_EVENT:
		case CTRL_SHUTDOWN_EVENT:
			Pion::shutdown();
			return TRUE;
		default:
			return FALSE;
	}
}
#else
void handle_signal(int sig)
{
	Pion::shutdown();
}
#endif


/// displays an error message if the arguments are invalid
void argument_error(void)
{
	std::cerr << "usage:   PionModuleTest [OPTIONS] RESOURCE MODULE" << std::endl
		      << "         PionModuleTest [OPTIONS] -c MODULE_CONFIG_NAME" << std::endl
			  << "options: [-ssl PEM_FILE] [-p PORT] [-d MODULE_DIR] [-o OPTION=VALUE]" << std::endl;
}


/// main control function
int main (int argc, char *argv[])
{
	static const unsigned int DEFAULT_PORT = 8080;

	// used to keep track of module name=value options
	typedef std::vector<std::pair<std::string, std::string> >	ModuleOptionsType;
	ModuleOptionsType module_options;
	
	// parse command line: determine port number, RESOURCE and MODULE
	unsigned int port = DEFAULT_PORT;
	std::string module_config_name;
	std::string resource_name;
	std::string module_name;
	std::string ssl_pem_file;
	bool ssl_flag = false;
	
	for (int argnum=1; argnum < argc; ++argnum) {
		if (argv[argnum][0] == '-') {
			if (argv[argnum][1] == 'p' && argv[argnum][2] == '\0' && argnum+1 < argc) {
				++argnum;
				port = strtoul(argv[argnum], 0, 10);
				if (port == 0) port = DEFAULT_PORT;
			} else if (argv[argnum][1] == 'c' && argv[argnum][2] == '\0' && argnum+1 < argc) {
				module_config_name = argv[++argnum];
			} else if (argv[argnum][1] == 'd' && argv[argnum][2] == '\0' && argnum+1 < argc) {
				// add the modules directory to the search path
				try { Pion::addPluginDirectory(argv[++argnum]); }
				catch (PionPlugin::DirectoryNotFoundException&) {
					std::cerr << "PionModuleTest: Modules directory does not exist: "
						<< argv[argnum] << std::endl;
					return 1;
				}
			} else if (argv[argnum][1] == 'o' && argv[argnum][2] == '\0' && argnum+1 < argc) {
				std::string option_name(argv[++argnum]);
				std::string::size_type pos = option_name.find('=');
				if (pos == std::string::npos) {
					argument_error();
					return 1;
				}
				std::string option_value(option_name, pos + 1);
				option_name.resize(pos);
				module_options.push_back( std::make_pair(option_name, option_value) );
			} else if (argv[argnum][1] == 's' && argv[argnum][2] == 's' &&
					   argv[argnum][3] == 'l' && argv[argnum][4] == '\0' && argnum+1 < argc) {
				ssl_flag = true;
				ssl_pem_file = argv[++argnum];
			} else {
				argument_error();
				return 1;
			}
		} else if (argnum+2 == argc) {
			// second to last argument = RESOURCE
			resource_name = argv[argnum];
		} else if (argnum+1 == argc) {
			// last argument = MODULE
			module_name = argv[argnum];
		} else {
			argument_error();
			return 1;
		}
	}
	
	if (module_config_name.empty() && (resource_name.empty() || module_name.empty())) {
		argument_error();
		return 1;
	}
	
	// setup signal handler
#ifdef PION_WIN32
	SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
#else
	signal(SIGINT, handle_signal);
#endif
	
	// initialize log system (use simple configuration)
	PionLogger main_log(PION_GET_LOGGER("PionModuleTest"));
	PionLogger pion_log(PION_GET_LOGGER("Pion"));
	PION_LOG_SETLEVEL_DEBUG(main_log);
	PION_LOG_SETLEVEL_DEBUG(pion_log);
	PION_LOG_CONFIG_BASIC;
	
	try {
		// add the modules installation directory to our path
		try { Pion::addPluginDirectory(PION_MODULES_DIRECTORY); }
		catch (PionPlugin::DirectoryNotFoundException&) {
			PION_LOG_WARN(main_log, "Default modules directory does not exist: "
				<< PION_MODULES_DIRECTORY);
		}

		// create a server for HTTP & add the Hello module
		HTTPServerPtr http_server(Pion::addHTTPServer(port));

		if (ssl_flag) {
#ifdef PION_HAVE_SSL
			// configure server for SSL
			http_server->setSSLFlag(true);
			boost::asio::ssl::context& ssl_context = http_server->getSSLContext();
			ssl_context.set_options(boost::asio::ssl::context::default_workarounds
									| boost::asio::ssl::context::no_sslv2
									| boost::asio::ssl::context::single_dh_use);
			ssl_context.use_certificate_file(ssl_pem_file, boost::asio::ssl::context::pem);
			ssl_context.use_private_key_file(ssl_pem_file, boost::asio::ssl::context::pem);
			PION_LOG_INFO(main_log, "SSL support enabled using key file: " << ssl_pem_file);
#else
			PION_LOG_ERROR(main_log, "SSL support is not enabled in libpion");
#endif
		}
		
		if (module_config_name.empty()) {
			// load a single module using the command line arguments
			http_server->loadModule(resource_name, module_name);

			// set module options if any are defined
			for (ModuleOptionsType::iterator i = module_options.begin();
				 i != module_options.end(); ++i)
			{
				http_server->setModuleOption(resource_name, i->first, i->second);
			}
		} else {
			// load modules using the configuration file
			http_server->loadModuleConfig(module_config_name);
		}
	
		// startup pion
		Pion::startup();
	
		// run until stopped
		Pion::join();
		
	} catch (std::exception& e) {
		PION_LOG_FATAL(main_log, e.what());
	}

	return 0;
}
