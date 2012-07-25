// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PIONPROCESS_HEADER__
#define __PION_PIONPROCESS_HEADER__

#include <string>
#include <boost/noncopyable.hpp>
#include <boost/thread/once.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <pion/config.hpp>


namespace pion {    // begin namespace pion

///
/// PionProcess: class for managing process/service related functions
///
class PION_COMMON_API PionProcess :
    private boost::noncopyable
{
public:

    // default destructor
    ~PionProcess() {}
    
    /// default constructor
    PionProcess(void) {}
    
    /// signals the shutdown condition
    static void shutdown(void);
    
    /// blocks until the shutdown condition has been signaled
    static void wait_for_shutdown(void);

    /// sets up basic signal handling for the process
    static void initialize(void);
    
    /// fork process and run as a background daemon
    static void daemonize(void);


protected:

    /// data type for static/global process configuration information
    struct PionProcessConfig {
        /// constructor just initializes native types
        PionProcessConfig() : shutdown_now(false) {}
    
        /// true if we should shutdown now
        bool                    shutdown_now;
        
        /// triggered when it is time to shutdown
        boost::condition        shutdown_cond;

        /// used to protect the shutdown condition
        boost::mutex            shutdown_mutex;
    };

    
    /// returns a singleton instance of PionProcessConfig
    static inline PionProcessConfig& getPionProcessConfig(void) {
        boost::call_once(PionProcess::createPionProcessConfig, m_instance_flag);
        return *m_config_ptr;
    }


private:

    /// creates the PionProcessConfig singleton
    static void createPionProcessConfig(void);

    
    /// used to ensure thread safety of the PionProcessConfig singleton
    static boost::once_flag             m_instance_flag;

    /// pointer to the PionProcessConfig singleton
    static PionProcessConfig *          m_config_ptr;
};


}   // end namespace pion

#endif
