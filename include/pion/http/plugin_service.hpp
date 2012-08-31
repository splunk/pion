// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PLUGIN_SERVICE_HEADER__
#define __PION_PLUGIN_SERVICE_HEADER__

#include <string>
#include <boost/noncopyable.hpp>
#include <pion/config.hpp>
#include <pion/error.hpp>
#include <pion/algorithm.hpp>
#include <pion/http/request.hpp>
#include <pion/tcp/connection.hpp>


namespace pion {    // begin namespace pion
namespace http {    // begin namespace http


///
/// plugin_service: interface class for web services
/// 
class plugin_service :
    private boost::noncopyable
{
public:

    /// default constructor
    plugin_service(void) {}

    /// virtual destructor
    virtual ~plugin_service() {}

    /**
     * attempts to handle a new HTTP request
     *
     * @param http_request_ptr the new HTTP request to handle
     * @param tcp_conn the TCP connection that has the new request
     */
    virtual void operator()(http::request_ptr& http_request_ptr, tcp::connection_ptr& tcp_conn) = 0;
    
    /**
     * sets a configuration option
     *
     * @param name the name of the option to change
     * @param value the value of the option
     */
    virtual void set_option(const std::string& name, const std::string& value) {
        BOOST_THROW_EXCEPTION( error::bad_arg() << error::errinfo_arg_name(name) );
    }
    
    /// called when the web service's server is starting
    virtual void start(void) {}
    
    /// called when the web service's server is stopping
    virtual void stop(void) {}
    
    /// sets the URI stem or resource that is bound to the web service
    inline void set_resource(const std::string& str) { m_resource = str; }

    /// returns the URI stem or resource that is bound to the web service   
    inline const std::string& get_resource(void) const { return m_resource; }
    
    /// returns the path to the resource requested, relative to the web service's location
    inline std::string get_relative_resource(const std::string& resource_requested) const {
        if (resource_requested.size() <= get_resource().size()) {
            // either the request matches the web service's resource path (a directory)
            // or the request does not match (should never happen)
            return std::string();
        }
        // strip the web service's resource path plus the slash after it
        return algorithm::url_decode(resource_requested.substr(get_resource().size() + 1));
    }
    
    
private:
        
    /// the URI stem or resource that is bound to the web service   
    std::string m_resource;
};


//
// The following symbols must be defined for any web service that you would
// like to be able to load dynamically using the http::server::load_service()
// function.  These are not required for any services that you only want to link
// directly into your programs.
//
// Make sure that you replace "MyPluginName" with the name of your derived class.
// This name must also match the name of the object file (excluding the
// extension).  These symbols must be linked into your service's object file,
// not included in any headers that it may use (declarations are OK in headers
// but not the definitions).
//
// The "pion_create" function is used to create new instances of your service.
// The "pion_destroy" function is used to destroy instances of your service.
//
// extern "C" MyPluginName *pion_create_MyPluginName(void) {
//      return new MyPluginName;
// }
//
// extern "C" void pion_destroy_MyPluginName(MyPluginName *service_ptr) {
//      delete service_ptr;
// }
//


}   // end namespace http
}   // end namespace pion

#endif
