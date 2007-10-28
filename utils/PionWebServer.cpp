// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include <vector>
#include <iostream>
#include <pion/PionPlugin.hpp>
#include <pion/net/HTTPServer.hpp>
#include "ShutdownManager.hpp"

// these are used only when linking to static web service libraries
// #ifdef PION_STATIC_LINKING
PION_DECLARE_PLUGIN(EchoService)
PION_DECLARE_PLUGIN(FileService)
PION_DECLARE_PLUGIN(HelloService)
PION_DECLARE_PLUGIN(LogService)
PION_DECLARE_PLUGIN(CookieService)

using namespace std;
using namespace pion;
using namespace pion::net;


/// displays an error message if the arguments are invalid
void argument_error(void)
{
	std::cerr << "usage:   PionWebServer [OPTIONS] RESOURCE WEBSERVICE" << std::endl
		      << "         PionWebServer [OPTIONS] -c SERVICE_CONFIG_FILE" << std::endl
			  << "options: [-ssl PEM_FILE] [-i IP] [-p PORT] [-d PLUGINS_DIR] [-o OPTION=VALUE]" << std::endl;
}


/// main control function
int main (int argc, char *argv[])
{
	static const unsigned int DEFAULT_PORT = 8080;

	// used to keep track of web service name=value options
	typedef std::vector<std::pair<std::string, std::string> >	ServiceOptionsType;
	ServiceOptionsType service_options;
	
	// parse command line: determine port number, RESOURCE and WEBSERVICE
	boost::asio::ip::tcp::endpoint cfg_endpoint(boost::asio::ip::tcp::v4(), DEFAULT_PORT);
	std::string service_config_file;
	std::string resource_name;
	std::string service_name;
	std::string ssl_pem_file;
	bool ssl_flag = false;
	
	for (int argnum=1; argnum < argc; ++argnum) {
		if (argv[argnum][0] == '-') {
			if (argv[argnum][1] == 'p' && argv[argnum][2] == '\0' && argnum+1 < argc) {
				// set port number
				++argnum;
				cfg_endpoint.port(strtoul(argv[argnum], 0, 10));
				if (cfg_endpoint.port() == 0) cfg_endpoint.port(DEFAULT_PORT);
			} else if (argv[argnum][1] == 'i' && argv[argnum][2] == '\0' && argnum+1 < argc) {
				// set ip address
				cfg_endpoint.address(boost::asio::ip::address::from_string(argv[++argnum]));
			} else if (argv[argnum][1] == 'c' && argv[argnum][2] == '\0' && argnum+1 < argc) {
				service_config_file = argv[++argnum];
			} else if (argv[argnum][1] == 'd' && argv[argnum][2] == '\0' && argnum+1 < argc) {
				// add the service plug-ins directory to the search path
				try { PionPlugin::addPluginDirectory(argv[++argnum]); }
				catch (PionPlugin::DirectoryNotFoundException&) {
					std::cerr << "PionWebServer: Web service plug-ins directory does not exist: "
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
				service_options.push_back( std::make_pair(option_name, option_value) );
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
			// last argument = WEBSERVICE
			service_name = argv[argnum];
		} else {
			argument_error();
			return 1;
		}
	}
	
	if (service_config_file.empty() && (resource_name.empty() || service_name.empty())) {
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
	PionLogger main_log(PION_GET_LOGGER("PionWebServer"));
	PionLogger pion_log(PION_GET_LOGGER("pion"));
	PION_LOG_SETLEVEL_INFO(main_log);
	PION_LOG_SETLEVEL_INFO(pion_log);
	PION_LOG_CONFIG_BASIC;
	
	try {
		// add the Pion plug-ins installation directory to our path
		try { PionPlugin::addPluginDirectory(PION_PLUGINS_DIRECTORY); }
		catch (PionPlugin::DirectoryNotFoundException&) {
			PION_LOG_WARN(main_log, "Default plug-ins directory does not exist: "
				<< PION_PLUGINS_DIRECTORY);
		}

		// add the directory of the program we're running to our path
		try { PionPlugin::addPluginDirectory(boost::filesystem::path(argv[0]).branch_path().string()); }
		catch (PionPlugin::DirectoryNotFoundException&) {
			PION_LOG_WARN(main_log, "Directory of current executable does not exist: "
				<< boost::filesystem::path(argv[0]).branch_path());
		}

		// create a server for HTTP & add the Hello Service
		HTTPServerPtr http_server(HTTPServer::create(cfg_endpoint));

		if (ssl_flag) {
#ifdef PION_HAVE_SSL
			// configure server for SSL
			http_server->setSSLKeyFile(ssl_pem_file);
			PION_LOG_INFO(main_log, "SSL support enabled using key file: " << ssl_pem_file);
#else
			PION_LOG_ERROR(main_log, "SSL support is not enabled");
#endif
		}
		
		if (service_config_file.empty()) {
			// load a single web service using the command line arguments
			http_server->loadService(resource_name, service_name);

			// set web service options if any are defined
			for (ServiceOptionsType::iterator i = service_options.begin();
				 i != service_options.end(); ++i)
			{
				http_server->setServiceOption(resource_name, i->first, i->second);
			}
		} else {
			// load services using the configuration file
			http_server->loadServiceConfig(service_config_file);
		}

		// startup the server
		http_server->start();
		main_shutdown_manager.wait();
		
	} catch (std::exception& e) {
		PION_LOG_FATAL(main_log, e.what());
	}

	return 0;
}
