// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_ERROR_HEADER__
#define __PION_ERROR_HEADER__

#include <string>
#include <exception>
#include <boost/throw_exception.hpp>
#include <boost/exception/exception.hpp>
#include <boost/exception/error_info.hpp>
#include <boost/exception/info.hpp>
#include <pion/config.hpp>


namespace pion {    // begin namespace pion
namespace error {    // begin namespace error

    //
    // pion error info types
    //

    /// generic error message
    typedef boost::error_info<struct errinfo_arg_name_,std::string> errinfo_message;

    /// name of an unrecognized configuration argument or option
    typedef boost::error_info<struct errinfo_arg_name_,std::string> errinfo_arg_name;

    /// file name/path
    typedef boost::error_info<struct errinfo_file_name_,std::string> errinfo_file_name;
    
    /// directory name/path
    typedef boost::error_info<struct errinfo_dir_name_,std::string> errinfo_dir_name;
    
    /// plugin identifier
    typedef boost::error_info<struct errinfo_plugin_name_,std::string> errinfo_plugin_name;

    /// plugin symbol name
    typedef boost::error_info<struct errinfo_dir_name_,std::string> errinfo_symbol_name;
    
    //
    // pion exception types
    //

    /// exception thrown for an invalid configuration argument or option
    struct bad_arg : virtual std::exception, virtual boost::exception {};
    
    /// exception thrown if there is an error parsing a configuration file
    struct bad_config : virtual std::exception, virtual boost::exception {};
    
    /// exception thrown if we failed to open a file
    struct open_file : virtual std::exception, virtual boost::exception {};
    
    /// exception thrown if we are unable to open a plugin
    struct open_plugin : virtual std::exception, virtual boost::exception {};
    
    /// exception thrown if we failed to read data from a file
    struct read_file : virtual std::exception, virtual boost::exception {};
    
    /// exception thrown if a file is not found
    struct file_not_found : virtual std::exception, virtual boost::exception {};
    
    /// exception thrown if a required directory is not found
    struct directory_not_found : virtual std::exception, virtual boost::exception {};

    /// exception thrown if a plugin cannot be found
    struct plugin_not_found : virtual std::exception, virtual boost::exception {};
    
    /// exception thrown if a web service plugin cannot be found
    struct service_not_found : virtual std::exception, virtual boost::exception {};
    
    /// exception thrown if we try to add or load a duplicate plugin
    struct duplicate_plugin : virtual std::exception, virtual boost::exception {};

    /// exception thrown if a plugin is missing a required symbol
    struct plugin_missing_symbol : virtual std::exception, virtual boost::exception {};
  
    /// exception thrown if a plugin has an undefined state
    struct plugin_undefined : virtual std::exception, virtual boost::exception {};
    
    /// exception thrown if a bad password hash is provided
    struct bad_password_hash : virtual std::exception, virtual boost::exception {};
    
    
}   // end namespace error
}   // end namespace pion

#endif
