// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/assert.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include "FileService.hpp"
#include <pion/error.hpp>
#include <pion/plugin.hpp>
#include <pion/algorithm.hpp>
#include <pion/http/response_writer.hpp>

using namespace pion;

namespace pion {        // begin namespace pion
namespace plugins {     // begin namespace plugins


// static members of FileService

const std::string           FileService::DEFAULT_MIME_TYPE("application/octet-stream");
const unsigned int          FileService::DEFAULT_CACHE_SETTING = 1;
const unsigned int          FileService::DEFAULT_SCAN_SETTING = 0;
const unsigned long         FileService::DEFAULT_MAX_CACHE_SIZE = 0;    /* 0=disabled */
const unsigned long         FileService::DEFAULT_MAX_CHUNK_SIZE = 0;    /* 0=disabled */
boost::once_flag            FileService::m_mime_types_init_flag = BOOST_ONCE_INIT;
FileService::MIMETypeMap    *FileService::m_mime_types_ptr = NULL;


// FileService member functions

FileService::FileService(void)
    : m_logger(PION_GET_LOGGER("pion.FileService")),
    m_cache_setting(DEFAULT_CACHE_SETTING),
    m_scan_setting(DEFAULT_SCAN_SETTING),
    m_max_cache_size(DEFAULT_MAX_CACHE_SIZE),
    m_max_chunk_size(DEFAULT_MAX_CHUNK_SIZE),
    m_writable(false)
{}

void FileService::set_option(const std::string& name, const std::string& value)
{
    if (name == "directory") {
        m_directory = value;
        m_directory.normalize();
        plugin::check_cygwin_path(m_directory, value);
        // make sure that the directory exists
        if (! boost::filesystem::exists(m_directory) || ! boost::filesystem::is_directory(m_directory)) {
# if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
            const std::string dir_name = m_directory.string();
#else
            const std::string dir_name = m_directory.directory_string();
#endif
            BOOST_THROW_EXCEPTION( error::directory_not_found() << error::errinfo_dir_name(dir_name) );
        }
    } else if (name == "file") {
        m_file = value;
        plugin::check_cygwin_path(m_file, value);
        // make sure that the directory exists
        if (! boost::filesystem::exists(m_file) || boost::filesystem::is_directory(m_file)) {
# if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
            const std::string file_name = m_file.string();
#else
            const std::string file_name = m_file.file_string();
#endif
            BOOST_THROW_EXCEPTION( error::file_not_found() << error::errinfo_file_name(file_name) );
        }
    } else if (name == "cache") {
        if (value == "0") {
            m_cache_setting = 0;
        } else if (value == "1") {
            m_cache_setting = 1;
        } else if (value == "2") {
            m_cache_setting = 2;
        } else {
            BOOST_THROW_EXCEPTION( error::bad_arg() << error::errinfo_arg_name(name) );
        }
    } else if (name == "scan") {
        if (value == "0") {
            m_scan_setting = 0;
        } else if (value == "1") {
            m_scan_setting = 1;
        } else if (value == "2") {
            m_scan_setting = 2;
        } else if (value == "3") {
            m_scan_setting = 3;
        } else {
            BOOST_THROW_EXCEPTION( error::bad_arg() << error::errinfo_arg_name(name) );
        }
    } else if (name == "max_chunk_size") {
        m_max_chunk_size = boost::lexical_cast<unsigned long>(value);
    } else if (name == "writable") {
        if (value == "true") {
            m_writable = true;
        } else if (value == "false") {
            m_writable = false;
        } else {
            BOOST_THROW_EXCEPTION( error::bad_arg() << error::errinfo_arg_name(name) );
        }
    } else {
        BOOST_THROW_EXCEPTION( error::bad_arg() << error::errinfo_arg_name(name) );
    }
}

void FileService::operator()(const http::request_ptr& http_request_ptr, const tcp::connection_ptr& tcp_conn)
{
    // get the relative resource path for the request
    const std::string relative_path(get_relative_resource(http_request_ptr->get_resource()));

    // determine the path of the file being requested
    boost::filesystem::path file_path;
    if (relative_path.empty()) {
        // request matches resource exactly

        if (m_file.empty()) {
            // no file is specified, either in the request or in the options
            PION_LOG_WARN(m_logger, "No file option defined ("
                          << get_resource() << ")");
            sendNotFoundResponse(http_request_ptr, tcp_conn);
            return;
        } else {
            file_path = m_file;
        }
    } else {
        // request does not match resource

        if (m_directory.empty()) {
            // no directory is specified for the relative file
            PION_LOG_WARN(m_logger, "No directory option defined ("
                          << get_resource() << "): " << relative_path);
            sendNotFoundResponse(http_request_ptr, tcp_conn);
            return;
        } else {
            file_path = m_directory / relative_path;
        }
    }

    // make sure that the requested file is within the configured directory
    file_path.normalize();
# if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
    std::string file_string = file_path.string();
    if (file_string.find(m_directory.string()) != 0) {
#else
    std::string file_string = file_path.file_string();
    if (file_string.find(m_directory.directory_string()) != 0) {
#endif
        PION_LOG_WARN(m_logger, "Request for file outside of directory ("
                      << get_resource() << "): " << relative_path);
        static const std::string FORBIDDEN_HTML_START =
            "<html><head>\n"
            "<title>403 Forbidden</title>\n"
            "</head><body>\n"
            "<h1>Forbidden</h1>\n"
            "<p>The requested URL ";
        static const std::string FORBIDDEN_HTML_FINISH =
            " is not in the configured directory.</p>\n"
            "</body></html>\n";
        http::response_writer_ptr writer(http::response_writer::create(tcp_conn, *http_request_ptr,
                                     boost::bind(&tcp::connection::finish, tcp_conn)));
        writer->get_response().set_status_code(http::types::RESPONSE_CODE_FORBIDDEN);
        writer->get_response().set_status_message(http::types::RESPONSE_MESSAGE_FORBIDDEN);
        if (http_request_ptr->get_method() != http::types::REQUEST_METHOD_HEAD) {
            writer->write_no_copy(FORBIDDEN_HTML_START);
            writer << algorithm::xml_encode(http_request_ptr->get_resource());
            writer->write_no_copy(FORBIDDEN_HTML_FINISH);
        }
        writer->send();
        return;
    }

    // requests specifying directories are not allowed
    if (boost::filesystem::is_directory(file_path)) {
        PION_LOG_WARN(m_logger, "Request for directory ("
                      << get_resource() << "): " << relative_path);
        static const std::string FORBIDDEN_HTML_START =
            "<html><head>\n"
            "<title>403 Forbidden</title>\n"
            "</head><body>\n"
            "<h1>Forbidden</h1>\n"
            "<p>The requested URL ";
        static const std::string FORBIDDEN_HTML_FINISH =
            " is a directory.</p>\n"
            "</body></html>\n";
        http::response_writer_ptr writer(http::response_writer::create(tcp_conn, *http_request_ptr,
                                     boost::bind(&tcp::connection::finish, tcp_conn)));
        writer->get_response().set_status_code(http::types::RESPONSE_CODE_FORBIDDEN);
        writer->get_response().set_status_message(http::types::RESPONSE_MESSAGE_FORBIDDEN);
        if (http_request_ptr->get_method() != http::types::REQUEST_METHOD_HEAD) {
            writer->write_no_copy(FORBIDDEN_HTML_START);
            writer << algorithm::xml_encode(http_request_ptr->get_resource());
            writer->write_no_copy(FORBIDDEN_HTML_FINISH);
        }
        writer->send();
        return;
    }

    if (http_request_ptr->get_method() == http::types::REQUEST_METHOD_GET 
        || http_request_ptr->get_method() == http::types::REQUEST_METHOD_HEAD)
    {
        // the type of response we will send
        enum ResponseType {
            RESPONSE_UNDEFINED,     // initial state until we know how to respond
            RESPONSE_OK,            // normal response that includes the file's content
            RESPONSE_HEAD_OK,       // response to HEAD request (would send file's content)
            RESPONSE_NOT_FOUND,     // Not Found (404)
            RESPONSE_NOT_MODIFIED   // Not Modified (304) response to If-Modified-Since
        } response_type = RESPONSE_UNDEFINED;

        // used to hold our response information
        DiskFile response_file;

        // get the If-Modified-Since request header
        const std::string if_modified_since(http_request_ptr->get_header(http::types::HEADER_IF_MODIFIED_SINCE));

        // check the cache for a corresponding entry (if enabled)
        // note that m_cache_setting may equal 0 if m_scan_setting == 1
        if (m_cache_setting > 0 || m_scan_setting > 0) {

            // search for a matching cache entry
            boost::mutex::scoped_lock cache_lock(m_cache_mutex);
            CacheMap::iterator cache_itr = m_cache_map.find(relative_path);

            if (cache_itr == m_cache_map.end()) {
                // no existing cache entries found

                if (m_scan_setting == 1 || m_scan_setting == 3) {
                    // do not allow files to be added;
                    // all requests must correspond with existing cache entries
                    // since no match was found, just return file not found
                    PION_LOG_WARN(m_logger, "Request for unknown file ("
                                  << get_resource() << "): " << relative_path);
                    response_type = RESPONSE_NOT_FOUND;
                } else {
                    PION_LOG_DEBUG(m_logger, "No cache entry for request ("
                                   << get_resource() << "): " << relative_path);
                }

            } else {
                // found an existing cache entry

                PION_LOG_DEBUG(m_logger, "Found cache entry for request ("
                               << get_resource() << "): " << relative_path);

                if (m_cache_setting == 0) {
                    // cache is disabled

                    // copy & re-use file_path and mime_type
                    response_file.setFilePath(cache_itr->second.getFilePath());
                    response_file.setMimeType(cache_itr->second.getMimeType());

                    // get the file_size and last_modified timestamp
                    response_file.update();

                    // just compare strings for simplicity (parsing this date format sucks!)
                    if (response_file.getLastModifiedString() == if_modified_since) {
                        // no need to read the file; the modified times match!
                        response_type = RESPONSE_NOT_MODIFIED;
                    } else {
                        if (http_request_ptr->get_method() == http::types::REQUEST_METHOD_HEAD) {
                            response_type = RESPONSE_HEAD_OK;
                        } else {
                            response_type = RESPONSE_OK;
                            PION_LOG_DEBUG(m_logger, "Cache disabled, reading file ("
                                           << get_resource() << "): " << relative_path);
                        }
                    }

                } else {
                    // cache is enabled

                    // true if the entry was updated (used for log message)
                    bool cache_was_updated = false;

                    if (cache_itr->second.getLastModified() == 0) {

                        // cache file for the first time
                        cache_was_updated = true;
                        cache_itr->second.update();
                        if (m_max_cache_size==0 || cache_itr->second.getFileSize() <= m_max_cache_size) {
                            // read the file (may throw exception)
                            cache_itr->second.read();
                        } else {
                            cache_itr->second.resetFileContent();
                        }

                    } else if (m_cache_setting == 1) {

                        // check if file has been updated (may throw exception)
                        cache_was_updated = cache_itr->second.checkUpdated();

                    } // else cache_setting == 2 (use existing values)

                    // get the response type
                    if (cache_itr->second.getLastModifiedString() == if_modified_since) {
                        response_type = RESPONSE_NOT_MODIFIED;
                    } else if (http_request_ptr->get_method() == http::types::REQUEST_METHOD_HEAD) {
                        response_type = RESPONSE_HEAD_OK;
                    } else {
                        response_type = RESPONSE_OK;
                    }

                    // copy cache contents so that we can release the mutex
                    response_file = cache_itr->second;

                    // check if max size has been exceeded
                    if (cache_was_updated && m_max_cache_size > 0 && cache_itr->second.getFileSize() > m_max_cache_size) {
                        cache_itr->second.resetFileContent();
                    }

                    PION_LOG_DEBUG(m_logger, (cache_was_updated ? "Updated" : "Using")
                                   << " cache entry for request ("
                                   << get_resource() << "): " << relative_path);
                }
            }
        }

        if (response_type == RESPONSE_UNDEFINED) {
            // make sure that the file exists
            if (! boost::filesystem::exists(file_path)) {
                PION_LOG_WARN(m_logger, "File not found ("
                              << get_resource() << "): " << relative_path);
                sendNotFoundResponse(http_request_ptr, tcp_conn);
                return;
            }

            response_file.setFilePath(file_path);

            PION_LOG_DEBUG(m_logger, "Found file for request ("
                           << get_resource() << "): " << relative_path);

            // determine the MIME type
# if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
            response_file.setMimeType(findMIMEType( response_file.getFilePath().filename().string()));
#else
            response_file.setMimeType(findMIMEType( response_file.getFilePath().leaf() ));
#endif

            // get the file_size and last_modified timestamp
            response_file.update();

            // just compare strings for simplicity (parsing this date format sucks!)
            if (response_file.getLastModifiedString() == if_modified_since) {
                // no need to read the file; the modified times match!
                response_type = RESPONSE_NOT_MODIFIED;
            } else if (http_request_ptr->get_method() == http::types::REQUEST_METHOD_HEAD) {
                response_type = RESPONSE_HEAD_OK;
            } else {
                response_type = RESPONSE_OK;
                if (m_cache_setting != 0) {
                    if (m_max_cache_size==0 || response_file.getFileSize() <= m_max_cache_size) {
                        // read the file (may throw exception)
                        response_file.read();
                    }
                    // add new entry to the cache
                    PION_LOG_DEBUG(m_logger, "Adding cache entry for request ("
                                   << get_resource() << "): " << relative_path);
                    boost::mutex::scoped_lock cache_lock(m_cache_mutex);
                    m_cache_map.insert( std::make_pair(relative_path, response_file) );
                }
            }
        }

        if (response_type == RESPONSE_OK) {
            // use DiskFileSender to send a file
            DiskFileSenderPtr sender_ptr(DiskFileSender::create(response_file,
                                                                http_request_ptr, tcp_conn,
                                                                m_max_chunk_size));
            sender_ptr->send();
        } else if (response_type == RESPONSE_NOT_FOUND) {
            sendNotFoundResponse(http_request_ptr, tcp_conn);
        } else {
            // sending headers only -> use our own response object

            // prepare a response and set the Content-Type
            http::response_writer_ptr writer(http::response_writer::create(tcp_conn, *http_request_ptr,
                                         boost::bind(&tcp::connection::finish, tcp_conn)));
            writer->get_response().set_content_type(response_file.getMimeType());

            // set Last-Modified header to enable client-side caching
            writer->get_response().add_header(http::types::HEADER_LAST_MODIFIED,
                                            response_file.getLastModifiedString());

            switch(response_type) {
                case RESPONSE_UNDEFINED:
                case RESPONSE_NOT_FOUND:
                case RESPONSE_OK:
                    // this should never happen
                    BOOST_ASSERT(false);
                    break;
                case RESPONSE_NOT_MODIFIED:
                    // set "Not Modified" response
                    writer->get_response().set_status_code(http::types::RESPONSE_CODE_NOT_MODIFIED);
                    writer->get_response().set_status_message(http::types::RESPONSE_MESSAGE_NOT_MODIFIED);
                    break;
                case RESPONSE_HEAD_OK:
                    // set "OK" response (not really necessary since this is the default)
                    writer->get_response().set_status_code(http::types::RESPONSE_CODE_OK);
                    writer->get_response().set_status_message(http::types::RESPONSE_MESSAGE_OK);
                    break;
            }

            // send the response
            writer->send();
        }
    } else if (http_request_ptr->get_method() == http::types::REQUEST_METHOD_POST
               || http_request_ptr->get_method() == http::types::REQUEST_METHOD_PUT
               || http_request_ptr->get_method() == http::types::REQUEST_METHOD_DELETE)
    {
        // If not writable, then send 405 (Method Not Allowed) response for POST, PUT or DELETE requests.
        if (!m_writable) {
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
                                         boost::bind(&tcp::connection::finish, tcp_conn)));
            writer->get_response().set_status_code(http::types::RESPONSE_CODE_METHOD_NOT_ALLOWED);
            writer->get_response().set_status_message(http::types::RESPONSE_MESSAGE_METHOD_NOT_ALLOWED);
            writer->write_no_copy(NOT_ALLOWED_HTML_START);
            writer << algorithm::xml_encode(http_request_ptr->get_method());
            writer->write_no_copy(NOT_ALLOWED_HTML_FINISH);
            writer->get_response().add_header("Allow", "GET, HEAD");
            writer->send();
        } else {
            http::response_writer_ptr writer(http::response_writer::create(tcp_conn, *http_request_ptr,
                                         boost::bind(&tcp::connection::finish, tcp_conn)));
            if (http_request_ptr->get_method() == http::types::REQUEST_METHOD_POST
                || http_request_ptr->get_method() == http::types::REQUEST_METHOD_PUT)
            {
                if (boost::filesystem::exists(file_path)) {
                    writer->get_response().set_status_code(http::types::RESPONSE_CODE_NO_CONTENT);
                    writer->get_response().set_status_message(http::types::RESPONSE_MESSAGE_NO_CONTENT);
                } else {
                    // The file doesn't exist yet, so it will be created below, unless the
                    // directory of the requested file also doesn't exist.
                    if (!boost::filesystem::exists(file_path.branch_path())) {
                        static const std::string NOT_FOUND_HTML_START =
                            "<html><head>\n"
                            "<title>404 Not Found</title>\n"
                            "</head><body>\n"
                            "<h1>Not Found</h1>\n"
                            "<p>The directory of the requested URL ";
                        static const std::string NOT_FOUND_HTML_FINISH =
                            " was not found on this server.</p>\n"
                            "</body></html>\n";
                        writer->get_response().set_status_code(http::types::RESPONSE_CODE_NOT_FOUND);
                        writer->get_response().set_status_message(http::types::RESPONSE_MESSAGE_NOT_FOUND);
                        writer->write_no_copy(NOT_FOUND_HTML_START);
                        writer << algorithm::xml_encode(http_request_ptr->get_resource());
                        writer->write_no_copy(NOT_FOUND_HTML_FINISH);
                        writer->send();
                        return;
                    }
                    static const std::string CREATED_HTML_START =
                        "<html><head>\n"
                        "<title>201 Created</title>\n"
                        "</head><body>\n"
                        "<h1>Created</h1>\n"
                        "<p>";
                    static const std::string CREATED_HTML_FINISH =
                        "</p>\n"
                        "</body></html>\n";
                    writer->get_response().set_status_code(http::types::RESPONSE_CODE_CREATED);
                    writer->get_response().set_status_message(http::types::RESPONSE_MESSAGE_CREATED);
                    writer->get_response().add_header(http::types::HEADER_LOCATION, http_request_ptr->get_resource());
                    writer->write_no_copy(CREATED_HTML_START);
                    writer << algorithm::xml_encode(http_request_ptr->get_resource());
                    writer->write_no_copy(CREATED_HTML_FINISH);
                }
                std::ios_base::openmode mode = http_request_ptr->get_method() == http::types::REQUEST_METHOD_POST?
                                               std::ios::app : std::ios::out;
                boost::filesystem::ofstream file_stream(file_path, mode);
                file_stream.write(http_request_ptr->get_content(), http_request_ptr->get_content_length());
                file_stream.close();
                if (!boost::filesystem::exists(file_path)) {
                    static const std::string PUT_FAILED_HTML_START =
                        "<html><head>\n"
                        "<title>500 Server Error</title>\n"
                        "</head><body>\n"
                        "<h1>Server Error</h1>\n"
                        "<p>Error writing to ";
                    static const std::string PUT_FAILED_HTML_FINISH =
                        ".</p>\n"
                        "</body></html>\n";
                    writer->get_response().set_status_code(http::types::RESPONSE_CODE_SERVER_ERROR);
                    writer->get_response().set_status_message(http::types::RESPONSE_MESSAGE_SERVER_ERROR);
                    writer->write_no_copy(PUT_FAILED_HTML_START);
                    writer << algorithm::xml_encode(http_request_ptr->get_resource());
                    writer->write_no_copy(PUT_FAILED_HTML_FINISH);
                }
                writer->send();
            } else if (http_request_ptr->get_method() == http::types::REQUEST_METHOD_DELETE) {
                if (!boost::filesystem::exists(file_path)) {
                    sendNotFoundResponse(http_request_ptr, tcp_conn);
                } else {
                    try {
                        boost::filesystem::remove(file_path);
                        writer->get_response().set_status_code(http::types::RESPONSE_CODE_NO_CONTENT);
                        writer->get_response().set_status_message(http::types::RESPONSE_MESSAGE_NO_CONTENT);
                        writer->send();
                    } catch (std::exception& e) {
                        static const std::string DELETE_FAILED_HTML_START =
                            "<html><head>\n"
                            "<title>500 Server Error</title>\n"
                            "</head><body>\n"
                            "<h1>Server Error</h1>\n"
                            "<p>Could not delete ";
                        static const std::string DELETE_FAILED_HTML_FINISH =
                            ".</p>\n"
                            "</body></html>\n";
                        writer->get_response().set_status_code(http::types::RESPONSE_CODE_SERVER_ERROR);
                        writer->get_response().set_status_message(http::types::RESPONSE_MESSAGE_SERVER_ERROR);
                        writer->write_no_copy(DELETE_FAILED_HTML_START);
                        writer << algorithm::xml_encode(http_request_ptr->get_resource())
                            << ".</p><p>"
                            << boost::diagnostic_information(e);
                        writer->write_no_copy(DELETE_FAILED_HTML_FINISH);
                        writer->send();
                    }
                }
            } else {
                // This should never be reached.
                writer->get_response().set_status_code(http::types::RESPONSE_CODE_SERVER_ERROR);
                writer->get_response().set_status_message(http::types::RESPONSE_MESSAGE_SERVER_ERROR);
                writer->send();
            }
        }
    }
    // Any method not handled above is unimplemented.
    else {
        static const std::string NOT_IMPLEMENTED_HTML_START =
            "<html><head>\n"
            "<title>501 Not Implemented</title>\n"
            "</head><body>\n"
            "<h1>Not Implemented</h1>\n"
            "<p>The requested method ";
        static const std::string NOT_IMPLEMENTED_HTML_FINISH =
            " is not implemented on this server.</p>\n"
            "</body></html>\n";
        http::response_writer_ptr writer(http::response_writer::create(tcp_conn, *http_request_ptr,
                                     boost::bind(&tcp::connection::finish, tcp_conn)));
        writer->get_response().set_status_code(http::types::RESPONSE_CODE_NOT_IMPLEMENTED);
        writer->get_response().set_status_message(http::types::RESPONSE_MESSAGE_NOT_IMPLEMENTED);
        writer->write_no_copy(NOT_IMPLEMENTED_HTML_START);
        writer << algorithm::xml_encode(http_request_ptr->get_method());
        writer->write_no_copy(NOT_IMPLEMENTED_HTML_FINISH);
        writer->send();
    }
}

void FileService::sendNotFoundResponse(const http::request_ptr& http_request_ptr,
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
                                 boost::bind(&tcp::connection::finish, tcp_conn)));
    writer->get_response().set_status_code(http::types::RESPONSE_CODE_NOT_FOUND);
    writer->get_response().set_status_message(http::types::RESPONSE_MESSAGE_NOT_FOUND);
    if (http_request_ptr->get_method() != http::types::REQUEST_METHOD_HEAD) {
        writer->write_no_copy(NOT_FOUND_HTML_START);
        writer << algorithm::xml_encode(http_request_ptr->get_resource());
        writer->write_no_copy(NOT_FOUND_HTML_FINISH);
    }
    writer->send();
}

void FileService::start(void)
{
    PION_LOG_DEBUG(m_logger, "Starting up resource (" << get_resource() << ')');

    // scan directory/file if scan setting != 0
    if (m_scan_setting != 0) {
        // force caching if scan == (2 | 3)
        if (m_cache_setting == 0 && m_scan_setting > 1)
            m_cache_setting = 1;

        boost::mutex::scoped_lock cache_lock(m_cache_mutex);

        // add entry for file if one is defined
        if (! m_file.empty()) {
            // use empty relative_path for file option
            // use placeholder entry (do not pre-populate) if scan == 1
            addCacheEntry("", m_file, m_scan_setting == 1);
        }

        // scan directory if one is defined
        if (! m_directory.empty())
            scanDirectory(m_directory);
    }
}

void FileService::stop(void)
{
    PION_LOG_DEBUG(m_logger, "Shutting down resource (" << get_resource() << ')');
    // clear cached files (if started again, it will re-scan)
    boost::mutex::scoped_lock cache_lock(m_cache_mutex);
    m_cache_map.clear();
}

void FileService::scanDirectory(const boost::filesystem::path& dir_path)
{
# if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
    PION_LOG_DEBUG(m_logger, "Scanning directory (" << get_resource() << "): "
                   << dir_path.string());
#else
    PION_LOG_DEBUG(m_logger, "Scanning directory (" << get_resource() << "): "
                   << dir_path.directory_string());
#endif

    // iterate through items in the directory
    boost::filesystem::directory_iterator end_itr;
    for ( boost::filesystem::directory_iterator itr( dir_path );
          itr != end_itr; ++itr )
    {
        if ( boost::filesystem::is_directory(*itr) ) {
            // item is a sub-directory

            // recursively call scanDirectory()
            scanDirectory(*itr);

        } else {
            // item is a regular file

            // figure out relative path to the file
# if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
            std::string file_path_string( itr->path().string() );
            std::string relative_path( file_path_string.substr(m_directory.string().size() + 1) );
#else
            std::string file_path_string( itr->path().file_string() );
            std::string relative_path( file_path_string.substr(m_directory.directory_string().size() + 1) );
#endif

            // add item to cache (use placeholder if scan == 1)
            addCacheEntry(relative_path, *itr, m_scan_setting == 1);
        }
    }
}

std::pair<FileService::CacheMap::iterator, bool>
FileService::addCacheEntry(const std::string& relative_path,
                           const boost::filesystem::path& file_path,
                           const bool placeholder)
{
# if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
    DiskFile cache_entry(file_path, NULL, 0, 0, findMIMEType(file_path.filename().string()));
#else
    DiskFile cache_entry(file_path, NULL, 0, 0, findMIMEType(file_path.leaf()));
#endif
    if (! placeholder) {
        cache_entry.update();
        // only read the file if its size is <= max_cache_size
        if (m_max_cache_size==0 || cache_entry.getFileSize() <= m_max_cache_size) {
            try { cache_entry.read(); }
            catch (std::exception&) {
# if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
                PION_LOG_ERROR(m_logger, "Unable to add file to cache: "
                               << file_path.string());
#else
                PION_LOG_ERROR(m_logger, "Unable to add file to cache: "
                               << file_path.file_string());
#endif
                return std::make_pair(m_cache_map.end(), false);
            }
        }
    }

    std::pair<CacheMap::iterator, bool> add_entry_result
        = m_cache_map.insert( std::make_pair(relative_path, cache_entry) );

    if (add_entry_result.second) {
# if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
        PION_LOG_DEBUG(m_logger, "Added file to cache: "
                       << file_path.string());
#else
        PION_LOG_DEBUG(m_logger, "Added file to cache: "
                       << file_path.file_string());
#endif
    } else {
# if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
        PION_LOG_ERROR(m_logger, "Unable to insert cache entry for file: "
                       << file_path.string());
#else
        PION_LOG_ERROR(m_logger, "Unable to insert cache entry for file: "
                       << file_path.file_string());
#endif
    }

    return add_entry_result;
}

std::string FileService::findMIMEType(const std::string& file_name) {
    // initialize m_mime_types if it hasn't been done already
    boost::call_once(FileService::createMIMETypes, m_mime_types_init_flag);

    // determine the file's extension
    std::string extension(file_name.substr(file_name.find_last_of('.') + 1));
    boost::algorithm::to_lower(extension);

    // search for the matching mime type and return the result
    MIMETypeMap::iterator i = m_mime_types_ptr->find(extension);
    return (i == m_mime_types_ptr->end() ? DEFAULT_MIME_TYPE : i->second);
}

void FileService::createMIMETypes(void) {
    // create the map
    static MIMETypeMap mime_types;

    // populate mime types
    mime_types["js"] = "text/javascript";
    mime_types["txt"] = "text/plain";
    mime_types["xml"] = "text/xml";
    mime_types["css"] = "text/css";
    mime_types["htm"] = "text/html";
    mime_types["html"] = "text/html";
    mime_types["xhtml"] = "text/html";
    mime_types["gif"] = "image/gif";
    mime_types["png"] = "image/png";
    mime_types["jpg"] = "image/jpeg";
    mime_types["jpeg"] = "image/jpeg";
    mime_types["svg"] = "image/svg+xml";
    mime_types["eof"] = "application/vnd.ms-fontobject";
    mime_types["otf"] = "application/x-font-opentype";
    mime_types["ttf"] = "application/x-font-ttf";
    mime_types["woff"] = "application/font-woff";
    // ...

    // set the static pointer
    m_mime_types_ptr = &mime_types;
}


// DiskFile member functions

void DiskFile::update(void)
{
    // set file_size and last_modified
    m_file_size = boost::numeric_cast<std::streamsize>(boost::filesystem::file_size( m_file_path ));
    m_last_modified = boost::filesystem::last_write_time( m_file_path );
    m_last_modified_string = http::types::get_date_string( m_last_modified );
}

void DiskFile::read(void)
{
    // re-allocate storage buffer for the file's content
    m_file_content.reset(new char[m_file_size]);

    // open the file for reading
    boost::filesystem::ifstream file_stream;
    file_stream.open(m_file_path, std::ios::in | std::ios::binary);

    // read the file into memory
    if (!file_stream.is_open() || !file_stream.read(m_file_content.get(), m_file_size)) {
# if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
        const std::string file_name = m_file_path.string();
#else
        const std::string file_name = m_file_path.file_string();
#endif
        BOOST_THROW_EXCEPTION( error::read_file() << error::errinfo_file_name(file_name) );
    }
}

bool DiskFile::checkUpdated(void)
{
    // get current values
    std::streamsize cur_size = boost::numeric_cast<std::streamsize>(boost::filesystem::file_size( m_file_path ));
    time_t cur_modified = boost::filesystem::last_write_time( m_file_path );

    // check if file has not been updated
    if (cur_modified == m_last_modified && cur_size == m_file_size)
        return false;

    // file has been updated

    // update file_size and last_modified timestamp
    m_file_size = cur_size;
    m_last_modified = cur_modified;
    m_last_modified_string = http::types::get_date_string( m_last_modified );

    // read new contents
    read();

    return true;
}


// DiskFileSender member functions

DiskFileSender::DiskFileSender(DiskFile& file, const pion::http::request_ptr& http_request_ptr,
                               const pion::tcp::connection_ptr& tcp_conn,
                               unsigned long max_chunk_size)
    : m_logger(PION_GET_LOGGER("pion.FileService.DiskFileSender")), m_disk_file(file),
    m_writer(pion::http::response_writer::create(tcp_conn, *http_request_ptr, boost::bind(&tcp::connection::finish, tcp_conn))),
    m_max_chunk_size(max_chunk_size), m_file_bytes_to_send(0), m_bytes_sent(0)
{
# if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
    PION_LOG_DEBUG(m_logger, "Preparing to send file"
                   << (m_disk_file.hasFileContent() ? " (cached): " : ": ")
                   << m_disk_file.getFilePath().string());
#else
    PION_LOG_DEBUG(m_logger, "Preparing to send file"
                   << (m_disk_file.hasFileContent() ? " (cached): " : ": ")
                   << m_disk_file.getFilePath().file_string());
#endif

        // set the Content-Type HTTP header using the file's MIME type
    m_writer->get_response().set_content_type(m_disk_file.getMimeType());

    // set Last-Modified header to enable client-side caching
    m_writer->get_response().add_header(http::types::HEADER_LAST_MODIFIED,
                                      m_disk_file.getLastModifiedString());

    // use "200 OK" HTTP response
    m_writer->get_response().set_status_code(http::types::RESPONSE_CODE_OK);
    m_writer->get_response().set_status_message(http::types::RESPONSE_MESSAGE_OK);
}

void DiskFileSender::send(void)
{
    // check if we have nothing to send (send 0 byte response content)
    if (m_disk_file.getFileSize() <= m_bytes_sent) {
        m_writer->send();
        return;
    }

    // calculate the number of bytes to send (m_file_bytes_to_send)
    m_file_bytes_to_send = m_disk_file.getFileSize() - m_bytes_sent;
    if (m_max_chunk_size > 0 && m_file_bytes_to_send > m_max_chunk_size)
        m_file_bytes_to_send = m_max_chunk_size;

    // get the content to send (file_content_ptr)
    char *file_content_ptr;

    if (m_disk_file.hasFileContent()) {

        // the entire file IS cached in memory (m_disk_file.file_content)
        file_content_ptr = m_disk_file.getFileContent() + m_bytes_sent;

    } else {
        // the file is not cached in memory

        // check if the file has been opened yet
        if (! m_file_stream.is_open()) {
            // open the file for reading
            m_file_stream.open(m_disk_file.getFilePath(), std::ios::in | std::ios::binary);
            if (! m_file_stream.is_open()) {
# if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
                PION_LOG_ERROR(m_logger, "Unable to open file: "
                               << m_disk_file.getFilePath().string());
#else
                PION_LOG_ERROR(m_logger, "Unable to open file: "
                               << m_disk_file.getFilePath().file_string());
#endif
                return;
            }
        }

        // check if the content buffer was initialized yet
        if (! m_content_buf) {
            // allocate memory for the new content buffer
            m_content_buf.reset(new char[m_file_bytes_to_send]);
        }
        file_content_ptr = m_content_buf.get();

        // read a block of data from the file into the content buffer
        if (! m_file_stream.read(m_content_buf.get(), m_file_bytes_to_send)) {
            if (m_file_stream.gcount() > 0) {
# if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
                PION_LOG_ERROR(m_logger, "File size inconsistency: "
                               << m_disk_file.getFilePath().string());
#else
                PION_LOG_ERROR(m_logger, "File size inconsistency: "
                               << m_disk_file.getFilePath().file_string());
#endif
            } else {
# if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
                PION_LOG_ERROR(m_logger, "Unable to read file: "
                               << m_disk_file.getFilePath().string());
#else
                PION_LOG_ERROR(m_logger, "Unable to read file: "
                               << m_disk_file.getFilePath().file_string());
#endif
            }
            return;
        }
    }

    // send the content
    m_writer->write_no_copy(file_content_ptr, m_file_bytes_to_send);

    if (m_bytes_sent + m_file_bytes_to_send >= m_disk_file.getFileSize()) {
        // this is the last piece of data to send
        if (m_bytes_sent > 0) {
            // send last chunk in a series
            m_writer->send_final_chunk(boost::bind(&DiskFileSender::handle_write,
                                                 shared_from_this(),
                                                 boost::asio::placeholders::error,
                                                 boost::asio::placeholders::bytes_transferred));
        } else {
            // sending entire file at once
            m_writer->send(boost::bind(&DiskFileSender::handle_write,
                                       shared_from_this(),
                                       boost::asio::placeholders::error,
                                       boost::asio::placeholders::bytes_transferred));
        }
    } else {
        // there will be more data -> send a chunk
        m_writer->send_chunk(boost::bind(&DiskFileSender::handle_write,
                                        shared_from_this(),
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred));
    }
}

void DiskFileSender::handle_write(const boost::system::error_code& write_error,
                                 std::size_t bytes_written)
{
    bool finished_sending = true;

    if (write_error) {
        // encountered error sending response data
        m_writer->get_connection()->set_lifecycle(tcp::connection::LIFECYCLE_CLOSE); // make sure it will get closed
        PION_LOG_WARN(m_logger, "Error sending file (" << write_error.message() << ')');
    } else {
        // response data sent OK

        // use m_file_bytes_to_send instead of bytes_written; bytes_written
        // includes bytes for HTTP headers and chunking headers
        m_bytes_sent += m_file_bytes_to_send;

        if (m_bytes_sent >= m_disk_file.getFileSize()) {
            // finished sending
            PION_LOG_DEBUG(m_logger, "Sent "
                           << (m_file_bytes_to_send < m_disk_file.getFileSize() ? "file chunk" : "complete file")
                           << " of " << m_file_bytes_to_send << " bytes (finished"
                           << (m_writer->get_connection()->get_keep_alive() ? ", keeping alive)" : ", closing)") );
        } else {
            // NOT finished sending
            PION_LOG_DEBUG(m_logger, "Sent file chunk of " << m_file_bytes_to_send << " bytes");
            finished_sending = false;
            m_writer->clear();
        }
    }

    if (finished_sending) {
        // connection::finish() calls tcp::server::finish_connection, which will either:
        // a) call http::server::handle_connection again if keep-alive is true; or,
        // b) close the socket and remove it from the server's connection pool
        m_writer->get_connection()->finish();
    } else {
        send();
    }
}


}   // end namespace plugins
}   // end namespace pion


/// creates new FileService objects
extern "C" PION_PLUGIN pion::plugins::FileService *pion_create_FileService(void)
{
    return new pion::plugins::FileService();
}

/// destroys FileService objects
extern "C" PION_PLUGIN void pion_destroy_FileService(pion::plugins::FileService *service_ptr)
{
    delete service_ptr;
}
