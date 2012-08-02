// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/algorithm/string.hpp>
#include <pion/algorithm.hpp>
#include <pion/http/basic_auth.hpp>
#include <pion/http/response_writer.hpp>
#include <pion/http/server.hpp>


namespace pion {    // begin namespace pion
namespace http {    // begin namespace http
    
    
// static members of basic_auth

const unsigned int  basic_auth::CACHE_EXPIRATION = 300;  // 5 minutes


// basic_auth member functions

basic_auth::basic_auth(PionUserManagerPtr userManager, const std::string& realm)
    : http::auth(userManager), m_realm(realm),
    m_cache_cleanup_time(boost::posix_time::second_clock::universal_time())
{
    setLogger(PION_GET_LOGGER("pion.http.basic_auth"));
}
    
bool basic_auth::handleRequest(HTTPRequestPtr& request, tcp::connection_ptr& tcp_conn)
{
    if (!needAuthentication(request)) {
        return true; // this request does not require authentication
    }
    
    boost::posix_time::ptime time_now(boost::posix_time::second_clock::universal_time());
    if (time_now > m_cache_cleanup_time + boost::posix_time::seconds(CACHE_EXPIRATION)) {
        // expire cache
        boost::mutex::scoped_lock cache_lock(m_cache_mutex);
        user_cache_type::iterator i;
        user_cache_type::iterator next=m_user_cache.begin();
        while (next!=m_user_cache.end()) {
            i=next;
            ++next;
            if (time_now > i->second.first + boost::posix_time::seconds(CACHE_EXPIRATION)) {
                // ok - this is an old record.. expire it now
                m_user_cache.erase(i);
            }
        }
        m_cache_cleanup_time = time_now;
    }
    
    // if we are here, we need to check if access authorized...
    std::string authorization = request->getHeader(HTTPTypes::HEADER_AUTHORIZATION);
    if (!authorization.empty()) {
        std::string credentials;
        if (parseAuthorization(authorization, credentials)) {
            // to do - use fast cache to match with active credentials
            boost::mutex::scoped_lock cache_lock(m_cache_mutex);
            user_cache_type::iterator user_cache_ptr=m_user_cache.find(credentials);
            if (user_cache_ptr!=m_user_cache.end()) {
                // we found the credentials in our cache...
                // we can approve authorization now!
                request->setUser(user_cache_ptr->second.second);
                user_cache_ptr->second.first = time_now;
                return true;
            }
    
            std::string username;
            std::string password;
    
            if (parseCredentials(credentials, username, password)) {
                // match username/password
                PionUserPtr user=m_user_manager->getUser(username, password);
                if (user) {
                    // add user to the cache
                    m_user_cache.insert(std::make_pair(credentials, std::make_pair(time_now, user)));
                    // add user credentials to the request object
                    request->setUser(user);
                    return true;
                }
            }
        }
    }

    // user not found
    handleUnauthorized(request, tcp_conn);
    return false;
}
    
void basic_auth::setOption(const std::string& name, const std::string& value) 
{
    if (name=="realm")
        m_realm = value;
    else
        BOOST_THROW_EXCEPTION( error::bad_arg() << error::errinfo_arg_name(name) );
}
    
bool basic_auth::parseAuthorization(const std::string& authorization, std::string &credentials)
{
    if (!boost::algorithm::starts_with(authorization, "Basic "))
        return false;
    credentials = authorization.substr(6);
    if (credentials.empty())
        return false;
    return true;
}
    
bool basic_auth::parseCredentials(const std::string &credentials,
    std::string &username, std::string &password)
{
    std::string user_password;
    
    if (! algorithm::base64_decode(credentials, user_password))
        return false;

    // find ':' symbol
    std::string::size_type i = user_password.find(':');
    if (i==0 || i==std::string::npos)
        return false;
    
    username = user_password.substr(0, i);
    password = user_password.substr(i+1);
    
    return true;
}
    
void basic_auth::handleUnauthorized(HTTPRequestPtr& http_request,
    tcp::connection_ptr& tcp_conn)
{
    // authentication failed, send 401.....
    static const std::string CONTENT =
        " <!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\""
        "\"http://www.w3.org/TR/1999/REC-html401-19991224/loose.dtd\">"
        "<HTML>"
        "<HEAD>"
        "<TITLE>Error</TITLE>"
        "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=ISO-8859-1\">"
        "</HEAD>"
        "<BODY><H1>401 Unauthorized.</H1></BODY>"
        "</HTML> ";
    HTTPResponseWriterPtr writer(HTTPResponseWriter::create(tcp_conn, *http_request,
    boost::bind(&connection::finish, tcp_conn)));
    writer->getResponse().setStatusCode(HTTPTypes::RESPONSE_CODE_UNAUTHORIZED);
    writer->getResponse().setStatusMessage(HTTPTypes::RESPONSE_MESSAGE_UNAUTHORIZED);
    writer->getResponse().addHeader("WWW-Authenticate", "Basic realm=\"" + m_realm + "\"");
    writer->writeNoCopy(CONTENT);
    writer->send();
}
    
}   // end namespace http
}   // end namespace pion
