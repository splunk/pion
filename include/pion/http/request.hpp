// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTP_REQUEST_HEADER__
#define __PION_HTTP_REQUEST_HEADER__

#include <boost/shared_ptr.hpp>
#include <pion/config.hpp>
#include <pion/http/message.hpp>
#include <pion/user.hpp>


namespace pion {    // begin namespace pion
namespace http {    // begin namespace http


///
/// request: container for HTTP request information
/// 
class request
    : public http::message
{
public:

    /**
     * constructs a new request object
     *
     * @param resource the HTTP resource to request
     */
    request(const std::string& resource)
        : m_method(REQUEST_METHOD_GET), m_resource(resource) {}
    
    /// constructs a new request object (default constructor)
    request(void) : m_method(REQUEST_METHOD_GET) {}
    
    /// virtual destructor
    virtual ~request() {}

    /// clears all request data
    virtual void clear(void) {
        http::message::clear();
        m_method.erase();
        m_resource.erase();
        m_original_resource.erase();
        m_query_string.erase();
        m_query_params.clear();
        m_user_record.reset();
    }

    /// the content length of the message can never be implied for requests
    virtual bool is_content_length_implied(void) const { return false; }

    /// returns the request method (i.e. GET, POST, PUT)
    inline const std::string& get_method(void) const { return m_method; }
    
    /// returns the resource uri-stem to be delivered (possibly the result of a redirect)
    inline const std::string& get_resource(void) const { return m_resource; }

    /// returns the resource uri-stem originally requested
    inline const std::string& get_original_resource(void) const { return m_original_resource; }

    /// returns the uri-query or query string requested
    inline const std::string& get_query_string(void) const { return m_query_string; }
    
    /// returns a value for the query key if any are defined; otherwise, an empty string
    inline const std::string& get_query(const std::string& key) const {
        return get_value(m_query_params, key);
    }

    /// returns the query parameters
    inline ihash_multimap& get_queries(void) {
        return m_query_params;
    }
    
    /// returns true if at least one value for the query key is defined
    inline bool has_query(const std::string& key) const {
        return(m_query_params.find(key) != m_query_params.end());
    }
        
    /// sets the HTTP request method (i.e. GET, POST, PUT)
    inline void set_method(const std::string& str) { 
        m_method = str;
        clear_first_line();
    }
    
    /// sets the resource or uri-stem originally requested
    inline void set_resource(const std::string& str) {
        m_resource = m_original_resource = str;
        clear_first_line();
    }

    /// changes the resource or uri-stem to be delivered (called as the result of a redirect)
    inline void change_resource(const std::string& str) { m_resource = str; }

    /// sets the uri-query or query string requested
    inline void set_query_string(const std::string& str) {
        m_query_string = str;
        clear_first_line();
    }
    
    /// adds a value for the query key
    inline void add_query(const std::string& key, const std::string& value) {
        m_query_params.insert(std::make_pair(key, value));
    }
    
    /// changes the value of a query key
    inline void change_query(const std::string& key, const std::string& value) {
        change_value(m_query_params, key, value);
    }
    
    /// removes all values for a query key
    inline void delete_query(const std::string& key) {
        delete_value(m_query_params, key);
    }
    
    /// use the query parameters to build a query string for the request
    inline void use_query_params_for_query_string(void) {
        set_query_string(make_query_string(m_query_params));
    }

    /// use the query parameters to build POST content for the request
    inline void use_query_params_for_post_content(void) {
        std::string post_content(make_query_string(m_query_params));
        set_content_length(post_content.size());
        char *ptr = create_content_buffer();  // null-terminates buffer
        if (! post_content.empty())
            memcpy(ptr, post_content.c_str(), post_content.size());
        set_method(REQUEST_METHOD_POST);
        set_content_type(CONTENT_TYPE_URLENCODED);
    }

    /// add content (for POST) from string
    inline void set_content(const std::string &value) {
        set_content_length(value.size());
        char *ptr = create_content_buffer();
        if (! value.empty())
            memcpy(ptr, value.c_str(), value.size());
    }
    
    /// add content (for POST) from buffer of given size
    /// does nothing if the buffer is invalid or the buffer size is zero
    inline void set_content(const char* value, const boost::uint64_t& size) {
        if ( NULL == value || 0 == size )
            return;
        set_content_length(size);
        char *ptr = create_content_buffer();
        memcpy(ptr, value, size);
    }
    
    /// sets the user record for HTTP request after authentication
    inline void set_user(user_ptr user) { m_user_record = user; }
    
    /// get the user record for HTTP request after authentication
    inline user_ptr get_user() const { return m_user_record; }


protected:

    /// updates the string containing the first line for the HTTP message
    virtual void update_first_line(void) const {
        // start out with the request method
        m_first_line = m_method;
        m_first_line += ' ';
        // append the resource requested
        m_first_line += m_resource;
        if (! m_query_string.empty()) {
            // append query string if not empty
            m_first_line += '?';
            m_first_line += m_query_string;
        }
        m_first_line += ' ';
        // append HTTP version
        m_first_line += get_version_string();
    }
    
    /// appends HTTP headers for any cookies defined by the http::message
    virtual void append_cookie_headers(void) {
        for (ihash_multimap::const_iterator i = get_cookies().begin(); i != get_cookies().end(); ++i) {
            std::string cookie_header;
            cookie_header = i->first;
            cookie_header += COOKIE_NAME_VALUE_DELIMITER;
            cookie_header += i->second;
            add_header(HEADER_COOKIE, cookie_header);
        }
    }

    
private:

    /// request method (GET, POST, PUT, etc.)
    std::string                     m_method;

    /// name of the resource or uri-stem to be delivered
    std::string                     m_resource;

    /// name of the resource or uri-stem originally requested
    std::string                     m_original_resource;

    /// query string portion of the URI
    std::string                     m_query_string;
    
    /// HTTP query parameters parsed from the request line and post content
    ihash_multimap                  m_query_params;

    /// pointer to user record if this request had been authenticated 
    user_ptr                        m_user_record;
};


/// data type for a HTTP request pointer
typedef boost::shared_ptr<request>      request_ptr;


}   // end namespace http
}   // end namespace pion

#endif
