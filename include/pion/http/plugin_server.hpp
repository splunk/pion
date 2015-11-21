// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PLUGIN_SERVER_HEADER__
#define __PION_PLUGIN_SERVER_HEADER__

#include <string>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <pion/config.hpp>
#include <pion/plugin.hpp>
#include <pion/plugin_manager.hpp>
#include <pion/http/server.hpp>
#include <pion/http/plugin_service.hpp>
#include <pion/stdx/asio.hpp>


namespace pion {    // begin namespace pion
namespace http {    // begin namespace http


///
/// plugin_server: a server that handles HTTP connections using http::plugin_service plug-ins
///
class PION_API plugin_server :
    public http::server
{

public:

    /// default destructor
    virtual ~plugin_server() { clear(); }
    
    /**
     * creates a new plugin_server object
     * 
     * @param tcp_port port number used to listen for new connections (IPv4)
     */
    explicit plugin_server(const unsigned int tcp_port = 0)
        : http::server(tcp_port)
    { 
        set_logger(PION_GET_LOGGER("pion.http.plugin_server"));
    }
    
    /**
     * creates a new plugin_server object
     * 
     * @param endpoint TCP endpoint used to listen for new connections (see ASIO docs)
     */
    explicit plugin_server(const stdx::asio::ip::tcp::endpoint& endpoint)
        : http::server(endpoint)
    { 
        set_logger(PION_GET_LOGGER("pion.http.plugin_server"));
    }

    /**
     * creates a new plugin_server object
     * 
     * @param sched the scheduler that will be used to manage worker threads
     * @param tcp_port port number used to listen for new connections (IPv4)
     */
    explicit plugin_server(scheduler& sched, const unsigned int tcp_port = 0)
        : http::server(sched, tcp_port)
    { 
        set_logger(PION_GET_LOGGER("pion.http.plugin_server"));
    }
    
    /**
     * creates a new plugin_server object
     * 
     * @param sched the scheduler that will be used to manage worker threads
     * @param endpoint TCP endpoint used to listen for new connections (see ASIO docs)
     */
    plugin_server(scheduler& sched, const stdx::asio::ip::tcp::endpoint& endpoint)
        : http::server(sched, endpoint)
    { 
        set_logger(PION_GET_LOGGER("pion.http.plugin_server"));
    }

    /**
     * adds a new web service to the web server
     *
     * @param resource the resource name or uri-stem to bind to the web service
     * @param service_ptr a pointer to the web service
     */
    void add_service(const std::string& resource, http::plugin_service *service_ptr);
    
    /**
     * loads a web service from a shared object file
     *
     * @param resource the resource name or uri-stem to bind to the web service
     * @param service_name the name of the web service to load (searches plug-in
     *        directories and appends extensions)
     */
    void load_service(const std::string& resource, const std::string& service_name);
    
    /**
     * sets a configuration option for the web service associated with resource
     *
     * @param resource the resource name or uri-stem that identifies the web service
     * @param name the name of the configuration option
     * @param value the value to set the option to
     */
    void set_service_option(const std::string& resource,
                          const std::string& name, const std::string& value);
    
    /**
     * Parses a simple web service configuration file. Each line in the file
     * starts with one of the following commands:
     *
     * path VALUE  :  adds a directory to the web service search path
     * service RESOURCE FILE  :  loads web service bound to RESOURCE from FILE
     * option RESOURCE NAME=VALUE  :  sets web service option NAME to VALUE
     *
     * Blank lines or lines that begin with # are ignored as comments.
     *
     * @param config_name the name of the config file to parse
     */
    void load_service_config(const std::string& config_name);

    /// clears all the web services that are currently configured
    virtual void clear(void) {
        if (is_listening()) stop();
        m_services.clear();
        http::server::clear();
    }

    
protected:
    
    /// called before the TCP server starts listening for new connections
    virtual void before_starting(void) {
        // call the start() method for each web service associated with this server
        m_services.run(boost::bind(&http::plugin_service::start, _1));
    }
    
    /// called after the TCP server has stopped listening for new connections
    virtual void after_stopping(void) {
        // call the stop() method for each web service associated with this server
        m_services.run(boost::bind(&http::plugin_service::stop, _1));
    }

    
private:
    
    /// data type for a collection of web services
    typedef plugin_manager<http::plugin_service>   service_manager_t;
    
    
    /// Web services associated with this server
    service_manager_t       m_services;
};


/// data type for a web server pointer
typedef boost::shared_ptr<plugin_server>        plugin_server_ptr;


}   // end namespace http
}   // end namespace pion

#endif
