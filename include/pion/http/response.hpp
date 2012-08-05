// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTP_RESPONSE_HEADER__
#define __PION_HTTP_RESPONSE_HEADER__

#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <pion/config.hpp>
#include <pion/http/message.hpp>
#include <pion/http/request.hpp>


namespace pion {    // begin namespace pion
namespace http {    // begin namespace http

    
///
/// response: container for HTTP response information
/// 
class response
    : public http::message
{
public:

    /**
     * constructs a new response object for a particular request
     *
     * @param http_request_ptr the request that this is responding to
     */
    response(const http::request& http_request_ptr)
        : m_status_code(RESPONSE_CODE_OK),
        m_status_message(RESPONSE_MESSAGE_OK)
    {
        updateRequestInfo(http_request_ptr);
    }

    /**
     * constructs a new response object for a particular request method
     *
     * @param request_method the method used by the HTTP request we are responding to
     */
    response(const std::string& request_method)
        : m_status_code(RESPONSE_CODE_OK), m_status_message(RESPONSE_MESSAGE_OK),
        m_request_method(request_method)
    {}
    
    /// copy constructor
    response(const response& http_response)
        : message(http_response),
        m_status_code(http_response.m_status_code),
        m_status_message(http_response.m_status_message),
        m_request_method(http_response.m_request_method)
    {}
    
    /// default constructor: you are strongly encouraged to use one of the other
    /// constructors, since response parsing is influenced by the request method
    response(void)
        : m_status_code(RESPONSE_CODE_OK),
        m_status_message(RESPONSE_MESSAGE_OK)
    {}
    
    /// virtual destructor
    virtual ~response() {}

    /// clears all response data
    virtual void clear(void) {
        http::message::clear();
        m_status_code = RESPONSE_CODE_OK;
        m_status_message = RESPONSE_MESSAGE_OK;
        m_request_method.clear();
    }

    /// the content length may be implied for certain types of responses
    virtual bool isContentLengthImplied(void) const {
        return (m_request_method == REQUEST_METHOD_HEAD             // HEAD responses have no content
                || (m_status_code >= 100 && m_status_code <= 199)       // 1xx responses have no content
                || m_status_code == 204 || m_status_code == 205     // no content & reset content responses
                || m_status_code == 304                             // not modified responses have no content
                );
    }

    /**
     * Updates HTTP request information for the response object (use 
     * this if the response cannot be constructed using the request)
     *
     * @param http_request the request that this is responding to
     */
    inline void updateRequestInfo(const http::request& http_request) {
        m_request_method = http_request.getMethod();
        if (http_request.getVersionMajor() == 1 && http_request.getVersionMinor() >= 1)
            setChunksSupported(true);
    }
    
    /// sets the HTTP response status code
    inline void setStatusCode(unsigned int n) {
        m_status_code = n;
        clearFirstLine();
    }

    /// sets the HTTP response status message
    inline void setStatusMessage(const std::string& msg) {
        m_status_message = msg;
        clearFirstLine();
    }
    
    /// returns the HTTP response status code
    inline unsigned int getStatusCode(void) const { return m_status_code; }
    
    /// returns the HTTP response status message
    inline const std::string& getStatusMessage(void) const { return m_status_message; }
    

    /**
     * sets a cookie by adding a Set-Cookie header (see RFC 2109)
     * the cookie will be discarded by the user-agent when it closes
     *
     * @param name the name of the cookie
     * @param value the value of the cookie
     */
    inline void setCookie(const std::string& name, const std::string& value) {
        std::string set_cookie_header(make_set_cookie_header(name, value, "/"));
        addHeader(HEADER_SET_COOKIE, set_cookie_header);
    }
    
    /**
     * sets a cookie by adding a Set-Cookie header (see RFC 2109)
     * the cookie will be discarded by the user-agent when it closes
     *
     * @param name the name of the cookie
     * @param value the value of the cookie
     * @param path the path of the cookie
     */
    inline void setCookie(const std::string& name, const std::string& value,
                          const std::string& path)
    {
        std::string set_cookie_header(make_set_cookie_header(name, value, path));
        addHeader(HEADER_SET_COOKIE, set_cookie_header);
    }
    
    /**
     * sets a cookie by adding a Set-Cookie header (see RFC 2109)
     *
     * @param name the name of the cookie
     * @param value the value of the cookie
     * @param path the path of the cookie
     * @param max_age the life of the cookie, in seconds (0 = discard)
     */
    inline void setCookie(const std::string& name, const std::string& value,
                          const std::string& path, const unsigned long max_age)
    {
        std::string set_cookie_header(make_set_cookie_header(name, value, path, true, max_age));
        addHeader(HEADER_SET_COOKIE, set_cookie_header);
    }
    
    /**
     * sets a cookie by adding a Set-Cookie header (see RFC 2109)
     *
     * @param name the name of the cookie
     * @param value the value of the cookie
     * @param max_age the life of the cookie, in seconds (0 = discard)
     */
    inline void setCookie(const std::string& name, const std::string& value,
                          const unsigned long max_age)
    {
        std::string set_cookie_header(make_set_cookie_header(name, value, "/", true, max_age));
        addHeader(HEADER_SET_COOKIE, set_cookie_header);
    }
    
    /// deletes cookie called name by adding a Set-Cookie header (cookie has no path)
    inline void deleteCookie(const std::string& name) {
        std::string set_cookie_header(make_set_cookie_header(name, "", "/", true, 0));
        addHeader(HEADER_SET_COOKIE, set_cookie_header);
    }
    
    /// deletes cookie called name by adding a Set-Cookie header (cookie has a path)
    inline void deleteCookie(const std::string& name, const std::string& path) {
        std::string set_cookie_header(make_set_cookie_header(name, "", path, true, 0));
        addHeader(HEADER_SET_COOKIE, set_cookie_header);
    }
    
    /// sets the time that the response was last modified (Last-Modified)
    inline void setLastModified(const unsigned long t) {
        changeHeader(HEADER_LAST_MODIFIED, get_date_string(t));
    }
    
    
protected:
    
    /// updates the string containing the first line for the HTTP message
    virtual void updateFirstLine(void) const {
        // start out with the HTTP version
        m_first_line = getVersionString();
        m_first_line += ' ';
        // append the response status code
        m_first_line +=  boost::lexical_cast<std::string>(m_status_code);
        m_first_line += ' ';
        // append the response status message
        m_first_line += m_status_message;
    }
    
    
private:

    /// The HTTP response status code
    unsigned int            m_status_code;
    
    /// The HTTP response status message
    std::string             m_status_message;
    
    /// HTTP method used by the request
    std::string             m_request_method;
};


/// data type for a HTTP response pointer
typedef boost::shared_ptr<response>     response_ptr;


}   // end namespace http
}   // end namespace pion

#endif
