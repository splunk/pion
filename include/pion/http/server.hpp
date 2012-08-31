// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTP_SERVER_HEADER__
#define __PION_HTTP_SERVER_HEADER__

#include <map>
#include <string>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/function/function2.hpp>
#include <boost/function/function3.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <pion/config.hpp>
#include <pion/tcp/server.hpp>
#include <pion/tcp/connection.hpp>
#include <pion/http/request.hpp>
#include <pion/http/auth.hpp>
#include <pion/http/parser.hpp>


namespace pion {    // begin namespace pion
namespace http {    // begin namespace http


///
/// server: a server that handles HTTP connections
///
class PION_API server :
    public tcp::server
{

public:

    /// type of function that is used to handle requests
    typedef boost::function2<void, http::request_ptr&, tcp::connection_ptr&>  request_handler_t;

    /// handler for requests that result in "500 Server Error"
    typedef boost::function3<void, http::request_ptr&, tcp::connection_ptr&,
        const std::string&> error_handler_t;


    /// default destructor
    virtual ~server() { if (is_listening()) stop(); }

    /**
     * creates a new server object
     * 
     * @param tcp_port port number used to listen for new connections (IPv4)
     */
    explicit server(const unsigned int tcp_port = 0)
        : tcp::server(tcp_port),
        m_bad_request_handler(server::handle_bad_request),
        m_not_found_handler(server::handle_not_found_request),
        m_server_error_handler(server::handle_server_error),
        m_max_content_length(http::parser::DEFAULT_CONTENT_MAX)
    { 
        set_logger(PION_GET_LOGGER("pion.http.server"));
    }

    /**
     * creates a new server object
     * 
     * @param endpoint TCP endpoint used to listen for new connections (see ASIO docs)
     */
    explicit server(const boost::asio::ip::tcp::endpoint& endpoint)
        : tcp::server(endpoint),
        m_bad_request_handler(server::handle_bad_request),
        m_not_found_handler(server::handle_not_found_request),
        m_server_error_handler(server::handle_server_error),
        m_max_content_length(http::parser::DEFAULT_CONTENT_MAX)
    { 
        set_logger(PION_GET_LOGGER("pion.http.server"));
    }

    /**
     * creates a new server object
     * 
     * @param sched the scheduler that will be used to manage worker threads
     * @param tcp_port port number used to listen for new connections (IPv4)
     */
    explicit server(scheduler& sched, const unsigned int tcp_port = 0)
        : tcp::server(sched, tcp_port),
        m_bad_request_handler(server::handle_bad_request),
        m_not_found_handler(server::handle_not_found_request),
        m_server_error_handler(server::handle_server_error),
        m_max_content_length(http::parser::DEFAULT_CONTENT_MAX)
    { 
        set_logger(PION_GET_LOGGER("pion.http.server"));
    }

    /**
     * creates a new server object
     * 
     * @param sched the scheduler that will be used to manage worker threads
     * @param endpoint TCP endpoint used to listen for new connections (see ASIO docs)
     */
    server(scheduler& sched, const boost::asio::ip::tcp::endpoint& endpoint)
        : tcp::server(sched, endpoint),
        m_bad_request_handler(server::handle_bad_request),
        m_not_found_handler(server::handle_not_found_request),
        m_server_error_handler(server::handle_server_error),
        m_max_content_length(http::parser::DEFAULT_CONTENT_MAX)
    { 
        set_logger(PION_GET_LOGGER("pion.http.server"));
    }

    /**
     * adds a new web service to the HTTP server
     *
     * @param resource the resource name or uri-stem to bind to the handler
     * @param request_handler function used to handle requests to the resource
     */
    void add_resource(const std::string& resource, request_handler_t request_handler);

    /**
     * removes a web service from the HTTP server
     *
     * @param resource the resource name or uri-stem to remove
     */
    void remove_resource(const std::string& resource);

    /**
     * adds a new resource redirection to the HTTP server
     *
     * @param requested_resource the resource name or uri-stem that will be redirected
     * @param new_resource the resource that requested_resource will be redirected to
     */
    void add_redirect(const std::string& requested_resource, const std::string& new_resource);

    /// sets the function that handles bad HTTP requests
    inline void set_bad_request_handler(request_handler_t h) { m_bad_request_handler = h; }

    /// sets the function that handles requests which match no other web services
    inline void set_not_found_handler(request_handler_t h) { m_not_found_handler = h; }

    /// sets the function that handles requests which match no other web services
    inline void set_error_handler(error_handler_t h) { m_server_error_handler = h; }

    /// clears the collection of resources recognized by the HTTP server
    virtual void clear(void) {
        if (is_listening()) stop();
        boost::mutex::scoped_lock resource_lock(m_resource_mutex);
        m_resources.clear();
    }

    /**
     * strips trailing slash from a string, if one exists
     *
     * @param str the original string
     * @return the resulting string, after any trailing slash is removed
     */
    static inline std::string strip_trailing_slash(const std::string& str) {
        std::string result(str);
        if (!result.empty() && result[result.size()-1]=='/')
            result.resize(result.size() - 1);
        return result;
    }

    /**
     * used to send responses when a bad HTTP request is made
     *
     * @param http_request_ptr the new HTTP request to handle
     * @param tcp_conn the TCP connection that has the new request
     */
    static void handle_bad_request(http::request_ptr& http_request_ptr,
                                 tcp::connection_ptr& tcp_conn);

    /**
     * used to send responses when no web services can handle the request
     *
     * @param http_request_ptr the new HTTP request to handle
     * @param tcp_conn the TCP connection that has the new request
     */
    static void handle_not_found_request(http::request_ptr& http_request_ptr,
                                      tcp::connection_ptr& tcp_conn);

    /**
     * used to send responses when a server error occurs
     *
     * @param http_request_ptr the new HTTP request to handle
     * @param tcp_conn the TCP connection that has the new request
     * @param error_msg message that explains what went wrong
     */
    static void handle_server_error(http::request_ptr& http_request_ptr,
                                  tcp::connection_ptr& tcp_conn,
                                  const std::string& error_msg);

    /**
     * used to send responses when a request is forbidden
     *
     * @param http_request_ptr the new HTTP request to handle
     * @param tcp_conn the TCP connection that has the new request
     * @param error_msg message that explains what went wrong
     */
    static void handle_forbidden_request(http::request_ptr& http_request_ptr,
                                       tcp::connection_ptr& tcp_conn,
                                       const std::string& error_msg);

    /**
     * used to send responses when a method is not allowed
     *
     * @param http_request_ptr the new HTTP request to handle
     * @param tcp_conn the TCP connection that has the new request
     * @param allowed_methods optional comma separated list of allowed methods
     */
    static void handle_method_not_allowed(http::request_ptr& http_request_ptr,
                                       tcp::connection_ptr& tcp_conn,
                                       const std::string& allowed_methods = "");

    /**
     * sets the handler object for authentication verification processing
     */ 
    inline void set_authentication(http::auth_ptr auth) { m_auth_ptr = auth; }

    /// sets the maximum length for HTTP request payload content
    inline void set_max_content_length(std::size_t n) { m_max_content_length = n; }

protected:

    /**
     * handles a new TCP connection
     * 
     * @param tcp_conn the new TCP connection to handle
     */
    virtual void handle_connection(tcp::connection_ptr& tcp_conn);

    /**
     * handles a new HTTP request
     *
     * @param http_request_ptr the HTTP request to handle
     * @param tcp_conn TCP connection containing a new request
     * @param ec error_code contains additional information for parsing errors
     */
    virtual void handle_request(http::request_ptr& http_request_ptr,
                                tcp::connection_ptr& tcp_conn, const boost::system::error_code& ec);

    /**
     * searches for the appropriate request handler to use for a given resource
     *
     * @param resource the name of the resource to search for
     * @param request_handler function that can handle requests for this resource
     */
    virtual bool find_request_handler(const std::string& resource,
                                      request_handler_t& request_handler) const;


private:

    /// maximum number of redirections
    static const unsigned int   MAX_REDIRECTS;

    /// data type for a map of resources to request handlers
    typedef std::map<std::string, request_handler_t>    resource_map_t;

    /// data type for a map of requested resources to other resources
    typedef std::map<std::string, std::string>          redirect_map_t;


    /// collection of resources that are recognized by this HTTP server
    resource_map_t              m_resources;

    /// collection of redirections from a requested resource to another resource
    redirect_map_t              m_redirects;

    /// points to a function that handles bad HTTP requests
    request_handler_t           m_bad_request_handler;

    /// points to a function that handles requests which match no web services
    request_handler_t           m_not_found_handler;

    /// points to the function that handles server errors
    error_handler_t             m_server_error_handler;

    /// mutex used to protect access to the resources
    mutable boost::mutex        m_resource_mutex;

    /// pointer to authentication handler object
    http::auth_ptr              m_auth_ptr;

    /// maximum length for HTTP request payload content
    std::size_t                 m_max_content_length;
};


/// data type for a HTTP server pointer
typedef boost::shared_ptr<server>	server_ptr;


}   // end namespace http
}   // end namespace pion

#endif
