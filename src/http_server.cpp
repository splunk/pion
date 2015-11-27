// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/exception/diagnostic_information.hpp>
#include <pion/algorithm.hpp>
#include <pion/http/server.hpp>
#include <pion/http/request.hpp>
#include <pion/http/request_reader.hpp>
#include <pion/http/response_writer.hpp>


namespace pion {    // begin namespace pion
namespace http {    // begin namespace http


// static members of server

const unsigned int          server::MAX_REDIRECTS = 10;


// server member functions

void server::handle_connection(const tcp::connection_ptr& tcp_conn)
{
    request_reader_ptr my_reader_ptr;
    my_reader_ptr = request_reader::create(tcp_conn, std::bind(&server::handle_request,
                                           this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    my_reader_ptr->set_max_content_length(m_max_content_length);
    my_reader_ptr->receive();
}

void server::handle_request(const http::request_ptr& http_request_ptr,
    const tcp::connection_ptr& tcp_conn, const asio::error_code& ec)
{
    if (ec || ! http_request_ptr->is_valid()) {
        tcp_conn->set_lifecycle(tcp::connection::LIFECYCLE_CLOSE); // make sure it will get closed
        if (tcp_conn->is_open() && (ec.category() == http::parser::get_error_category())) {
            // HTTP parser error
            PION_LOG_INFO(m_logger, "Invalid HTTP request (" << ec.message() << ")");
            m_bad_request_handler(http_request_ptr, tcp_conn);
        } else {
            if (ec == asio::error::operation_aborted || ec == asio::error::eof) {
                // don't spam the log with common (non-)errors that happen during normal operation
                PION_LOG_DEBUG(m_logger, "Lost connection on port " << get_port() << " (" << ec.message() << ")");
            } else {
                PION_LOG_INFO(m_logger, "Lost connection on port " << get_port() << " (" << ec.message() << ")");
            }

            tcp_conn->finish();
        }
        return;
    }
        
    PION_LOG_DEBUG(m_logger, "Received a valid HTTP request");

    // strip off trailing slash if the request has one
    std::string resource_requested(strip_trailing_slash(http_request_ptr->get_resource()));

    // apply any redirection
    redirect_map_t::const_iterator it = m_redirects.find(resource_requested);
    unsigned int num_redirects = 0;
    while (it != m_redirects.end()) {
        if (++num_redirects > MAX_REDIRECTS) {
            PION_LOG_ERROR(m_logger, "Maximum number of redirects (server::MAX_REDIRECTS) exceeded for requested resource: " << http_request_ptr->get_original_resource());
            m_server_error_handler(http_request_ptr, tcp_conn, "Maximum number of redirects (server::MAX_REDIRECTS) exceeded for requested resource");
            return;
        }
        resource_requested = it->second;
        http_request_ptr->change_resource(resource_requested);
        it = m_redirects.find(resource_requested);
    }

    // if authentication activated, check current request
    if (m_auth_ptr) {
        // try to verify authentication
        if (! m_auth_ptr->handle_request(http_request_ptr, tcp_conn)) {
            // the HTTP 401 message has already been sent by the authentication object
            PION_LOG_DEBUG(m_logger, "Authentication required for HTTP resource: "
                << resource_requested);
            if (http_request_ptr->get_resource() != http_request_ptr->get_original_resource()) {
                PION_LOG_DEBUG(m_logger, "Original resource requested was: " << http_request_ptr->get_original_resource());
            }
            return;
        }
    }
    
    // search for a handler matching the resource requested
    request_handler_t request_handler;
    if (find_request_handler(resource_requested, request_handler)) {
        
        // try to handle the request
        try {
            request_handler(http_request_ptr, tcp_conn);
            PION_LOG_DEBUG(m_logger, "Found request handler for HTTP resource: "
                           << resource_requested);
            if (http_request_ptr->get_resource() != http_request_ptr->get_original_resource()) {
                PION_LOG_DEBUG(m_logger, "Original resource requested was: " << http_request_ptr->get_original_resource());
            }
        } catch (std::bad_alloc&) {
            // propagate memory errors (FATAL)
            throw;
        } catch (std::exception& e) {
            // recover gracefully from other exceptions thrown by request handlers
            PION_LOG_ERROR(m_logger, "HTTP request handler: " << pion::diagnostic_information(e));
            m_server_error_handler(http_request_ptr, tcp_conn, e.what());
        } catch (boost::exception& e) {
            // recover gracefully from boost exceptions thrown by request handlers
            PION_LOG_ERROR(m_logger, "HTTP request handler: " << pion::diagnostic_information(e));
            m_server_error_handler(http_request_ptr, tcp_conn, pion::diagnostic_information(e));
        }
        
    } else {
        
        // no web services found that could handle the request
        PION_LOG_INFO(m_logger, "No HTTP request handlers found for resource: "
                      << resource_requested);
        if (http_request_ptr->get_resource() != http_request_ptr->get_original_resource()) {
            PION_LOG_DEBUG(m_logger, "Original resource requested was: " << http_request_ptr->get_original_resource());
        }
        m_not_found_handler(http_request_ptr, tcp_conn);
    }
}
    
bool server::find_request_handler(const std::string& resource,
                                    request_handler_t& request_handler) const
{
    // first make sure that HTTP resources are registered
    boost::mutex::scoped_lock resource_lock(m_resource_mutex);
    if (m_resources.empty())
        return false;
    
    // iterate through each resource entry that may match the resource
    resource_map_t::const_iterator i = m_resources.upper_bound(resource);
    while (i != m_resources.begin()) {
        --i;
        // check for a match if the first part of the strings match
        if (i->first.empty() || resource.compare(0, i->first.size(), i->first) == 0) {
            // only if the resource matches the plug-in's identifier
            // or if resource is followed first with a '/' character
            if (resource.size() == i->first.size() || resource[i->first.size()]=='/') {
                request_handler = i->second;
                return true;
            }
        }
    }
    
    return false;
}

void server::add_resource(const std::string& resource,
                             request_handler_t request_handler)
{
    boost::mutex::scoped_lock resource_lock(m_resource_mutex);
    const std::string clean_resource(strip_trailing_slash(resource));
    m_resources.insert(std::make_pair(clean_resource, request_handler));
    PION_LOG_INFO(m_logger, "Added request handler for HTTP resource: " << clean_resource);
}

void server::remove_resource(const std::string& resource)
{
    boost::mutex::scoped_lock resource_lock(m_resource_mutex);
    const std::string clean_resource(strip_trailing_slash(resource));
    m_resources.erase(clean_resource);
    PION_LOG_INFO(m_logger, "Removed request handler for HTTP resource: " << clean_resource);
}

void server::add_redirect(const std::string& requested_resource,
                             const std::string& new_resource)
{
    boost::mutex::scoped_lock resource_lock(m_resource_mutex);
    const std::string clean_requested_resource(strip_trailing_slash(requested_resource));
    const std::string clean_new_resource(strip_trailing_slash(new_resource));
    m_redirects.insert(std::make_pair(clean_requested_resource, clean_new_resource));
    PION_LOG_INFO(m_logger, "Added redirection for HTTP resource " << clean_requested_resource << " to resource " << clean_new_resource);
}

void server::handle_bad_request(const http::request_ptr& http_request_ptr,
                                  const tcp::connection_ptr& tcp_conn)
{
    static const std::string BAD_REQUEST_HTML =
        "<html><head>\n"
        "<title>400 Bad Request</title>\n"
        "</head><body>\n"
        "<h1>Bad Request</h1>\n"
        "<p>Your browser sent a request that this server could not understand.</p>\n"
        "</body></html>\n";
    http::response_writer_ptr writer(http::response_writer::create(tcp_conn, *http_request_ptr,
                                                            std::bind(&tcp::connection::finish, tcp_conn)));
    writer->get_response().set_status_code(http::types::RESPONSE_CODE_BAD_REQUEST);
    writer->get_response().set_status_message(http::types::RESPONSE_MESSAGE_BAD_REQUEST);
    writer->write_no_copy(BAD_REQUEST_HTML);
    writer->send();
}

void server::handle_not_found_request(const http::request_ptr& http_request_ptr,
                                       const tcp::connection_ptr& tcp_conn)
{
    static const std::string NOT_FOUND_HTML_START =
        "<html><head>\n"
        "<title>404 Not Found</title>\n"
        "</head><body>\n"
        "<h1>Not Found</h1>\n"
        "<p>The requested URL ";
    static const std::string NOT_FOUND_HTML_FINISH =
        " was not found on this server.</p>\n"
        "</body></html>\n";
    http::response_writer_ptr writer(http::response_writer::create(tcp_conn, *http_request_ptr,
                                                            std::bind(&tcp::connection::finish, tcp_conn)));
    writer->get_response().set_status_code(http::types::RESPONSE_CODE_NOT_FOUND);
    writer->get_response().set_status_message(http::types::RESPONSE_MESSAGE_NOT_FOUND);
    writer->write_no_copy(NOT_FOUND_HTML_START);
    writer << algorithm::xml_encode(http_request_ptr->get_resource());
    writer->write_no_copy(NOT_FOUND_HTML_FINISH);
    writer->send();
}

void server::handle_server_error(const http::request_ptr& http_request_ptr,
                                   const tcp::connection_ptr& tcp_conn,
                                   const std::string& error_msg)
{
    static const std::string SERVER_ERROR_HTML_START =
        "<html><head>\n"
        "<title>500 Server Error</title>\n"
        "</head><body>\n"
        "<h1>Internal Server Error</h1>\n"
        "<p>The server encountered an internal error: <strong>";
    static const std::string SERVER_ERROR_HTML_FINISH =
        "</strong></p>\n"
        "</body></html>\n";
    http::response_writer_ptr writer(http::response_writer::create(tcp_conn, *http_request_ptr,
                                                            std::bind(&tcp::connection::finish, tcp_conn)));
    writer->get_response().set_status_code(http::types::RESPONSE_CODE_SERVER_ERROR);
    writer->get_response().set_status_message(http::types::RESPONSE_MESSAGE_SERVER_ERROR);
    writer->write_no_copy(SERVER_ERROR_HTML_START);
    writer << algorithm::xml_encode(error_msg);
    writer->write_no_copy(SERVER_ERROR_HTML_FINISH);
    writer->send();
}

void server::handle_forbidden_request(const http::request_ptr& http_request_ptr,
                                        const tcp::connection_ptr& tcp_conn,
                                        const std::string& error_msg)
{
    static const std::string FORBIDDEN_HTML_START =
        "<html><head>\n"
        "<title>403 Forbidden</title>\n"
        "</head><body>\n"
        "<h1>Forbidden</h1>\n"
        "<p>User not authorized to access the requested URL ";
    static const std::string FORBIDDEN_HTML_MIDDLE =
        "</p><p><strong>\n";
    static const std::string FORBIDDEN_HTML_FINISH =
        "</strong></p>\n"
        "</body></html>\n";
    http::response_writer_ptr writer(http::response_writer::create(tcp_conn, *http_request_ptr,
                                                            std::bind(&tcp::connection::finish, tcp_conn)));
    writer->get_response().set_status_code(http::types::RESPONSE_CODE_FORBIDDEN);
    writer->get_response().set_status_message(http::types::RESPONSE_MESSAGE_FORBIDDEN);
    writer->write_no_copy(FORBIDDEN_HTML_START);
    writer << algorithm::xml_encode(http_request_ptr->get_resource());
    writer->write_no_copy(FORBIDDEN_HTML_MIDDLE);
    writer << error_msg;
    writer->write_no_copy(FORBIDDEN_HTML_FINISH);
    writer->send();
}

void server::handle_method_not_allowed(const http::request_ptr& http_request_ptr,
                                        const tcp::connection_ptr& tcp_conn,
                                        const std::string& allowed_methods)
{
    static const std::string NOT_ALLOWED_HTML_START =
        "<html><head>\n"
        "<title>405 Method Not Allowed</title>\n"
        "</head><body>\n"
        "<h1>Not Allowed</h1>\n"
        "<p>The requested method ";
    static const std::string NOT_ALLOWED_HTML_FINISH =
        " is not allowed on this server.</p>\n"
        "</body></html>\n";
    http::response_writer_ptr writer(http::response_writer::create(tcp_conn, *http_request_ptr,
                                                            std::bind(&tcp::connection::finish, tcp_conn)));
    writer->get_response().set_status_code(http::types::RESPONSE_CODE_METHOD_NOT_ALLOWED);
    writer->get_response().set_status_message(http::types::RESPONSE_MESSAGE_METHOD_NOT_ALLOWED);
    if (! allowed_methods.empty())
        writer->get_response().add_header("Allow", allowed_methods);
    writer->write_no_copy(NOT_ALLOWED_HTML_START);
    writer << algorithm::xml_encode(http_request_ptr->get_method());
    writer->write_no_copy(NOT_ALLOWED_HTML_FINISH);
    writer->send();
}

}   // end namespace http
}   // end namespace pion
