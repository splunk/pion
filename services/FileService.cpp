// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include "FileService.hpp"
#include <pion/PionPlugin.hpp>
#include <pion/net/HTTPResponseWriter.hpp>

using namespace pion;
using namespace pion::net;


// static members of FileService

const std::string			FileService::DEFAULT_MIME_TYPE("application/octet-stream");
const unsigned int			FileService::DEFAULT_CACHE_SETTING = 1;
const unsigned int			FileService::DEFAULT_SCAN_SETTING = 0;
const unsigned long			FileService::DEFAULT_MAX_CACHE_SIZE = 0;	/* 0=disabled */
const unsigned long			FileService::DEFAULT_MAX_CHUNK_SIZE = 0;	/* 0=disabled */
boost::once_flag			FileService::m_mime_types_init_flag = BOOST_ONCE_INIT;
FileService::MIMETypeMap	*FileService::m_mime_types_ptr = NULL;


// FileService member functions

FileService::FileService(void)
	: m_logger(PION_GET_LOGGER("pion.FileService")),
	m_cache_setting(DEFAULT_CACHE_SETTING),
	m_scan_setting(DEFAULT_SCAN_SETTING),
	m_max_cache_size(DEFAULT_MAX_CACHE_SIZE),
	m_max_chunk_size(DEFAULT_MAX_CHUNK_SIZE),
	m_writable(false)
{}

void FileService::setOption(const std::string& name, const std::string& value)
{
	if (name == "directory") {
		m_directory = value;
		PionPlugin::checkCygwinPath(m_directory, value);
		// make sure that the directory exists
		if (! boost::filesystem::exists(m_directory) )
			throw DirectoryNotFoundException(value);
		if (! boost::filesystem::is_directory(m_directory) )
			throw NotADirectoryException(value);
	} else if (name == "file") {
		m_file = value;
		PionPlugin::checkCygwinPath(m_file, value);
		// make sure that the directory exists
		if (! boost::filesystem::exists(m_file) )
			throw FileNotFoundException(value);
		if (boost::filesystem::is_directory(m_file) )
			throw NotAFileException(value);
	} else if (name == "cache") {
		if (value == "0") {
			m_cache_setting = 0;
		} else if (value == "1") {
			m_cache_setting = 1;
		} else if (value == "2") {
			m_cache_setting = 2;
		} else {
			throw InvalidCacheException(value);
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
			throw InvalidScanException(value);
		}
	} else if (name == "max_chunk_size") {
		m_max_chunk_size = boost::lexical_cast<unsigned long>(value);
	} else if (name == "writable") {
		if (value == "true") {
			m_writable = true;
		} else if (value == "false") {
			m_writable = false;
		} else {
			throw InvalidOptionValueException("writable", value);
		}
	} else {
		throw UnknownOptionException(name);
	}
}

void FileService::operator()(HTTPRequestPtr& request, TCPConnectionPtr& tcp_conn)
{
	// get the relative resource path for the request
	const std::string relative_path(getRelativeResource(request->getResource()));

	// determine the path of the file being requested
	boost::filesystem::path file_path;
	if (relative_path.empty()) {
		// request matches resource exactly

		if (m_file.empty()) {
			// no file is specified, either in the request or in the options
			PION_LOG_WARN(m_logger, "No file option defined ("
						  << getResource() << ")");
			sendNotFoundResponse(request, tcp_conn);
			return;
		} else {
			file_path = m_file;
		}
	} else {
		// request does not match resource

		if (m_directory.empty()) {
			// no directory is specified for the relative file
			PION_LOG_WARN(m_logger, "No directory option defined ("
						  << getResource() << "): " << relative_path);
			sendNotFoundResponse(request, tcp_conn);
			return;
		} else {
			file_path = m_directory / relative_path;
		}
	}
	
	// make sure that the requested file is within the configured directory
	file_path.normalize();
	std::string file_string = file_path.file_string();
	if (file_string.find(m_directory.directory_string()) != 0) {
		PION_LOG_WARN(m_logger, "Request for file outside of directory ("
					  << getResource() << "): " << relative_path);
		static const std::string FORBIDDEN_HTML_START =
			"<html><head>\n"
			"<title>403 Forbidden</title>\n"
			"</head><body>\n"
			"<h1>Forbidden</h1>\n"
			"<p>The requested URL ";
		static const std::string FORBIDDEN_HTML_FINISH =
			" is not in the configured directory.</p>\n"
			"</body></html>\n";
		HTTPResponseWriterPtr writer(HTTPResponseWriter::create(tcp_conn, *request,
									 boost::bind(&TCPConnection::finish, tcp_conn)));
		writer->getResponse().setStatusCode(HTTPTypes::RESPONSE_CODE_FORBIDDEN);
		writer->getResponse().setStatusMessage(HTTPTypes::RESPONSE_MESSAGE_FORBIDDEN);
		if (request->getMethod() != HTTPTypes::REQUEST_METHOD_HEAD) {
			writer->writeNoCopy(FORBIDDEN_HTML_START);
			writer << request->getResource();
			writer->writeNoCopy(FORBIDDEN_HTML_FINISH);
		}
		writer->send();
		return;
	}

	// requests specifying directories are not allowed
	if (boost::filesystem::is_directory(file_path)) {
		PION_LOG_WARN(m_logger, "Request for directory ("
					  << getResource() << "): " << relative_path);
		static const std::string FORBIDDEN_HTML_START =
			"<html><head>\n"
			"<title>403 Forbidden</title>\n"
			"</head><body>\n"
			"<h1>Forbidden</h1>\n"
			"<p>The requested URL ";
		static const std::string FORBIDDEN_HTML_FINISH =
			" is a directory.</p>\n"
			"</body></html>\n";
		HTTPResponseWriterPtr writer(HTTPResponseWriter::create(tcp_conn, *request,
									 boost::bind(&TCPConnection::finish, tcp_conn)));
		writer->getResponse().setStatusCode(HTTPTypes::RESPONSE_CODE_FORBIDDEN);
		writer->getResponse().setStatusMessage(HTTPTypes::RESPONSE_MESSAGE_FORBIDDEN);
		if (request->getMethod() != HTTPTypes::REQUEST_METHOD_HEAD) {
			writer->writeNoCopy(FORBIDDEN_HTML_START);
			writer << request->getResource();
			writer->writeNoCopy(FORBIDDEN_HTML_FINISH);
		}
		writer->send();
		return;
	}
				
	if (request->getMethod() == HTTPTypes::REQUEST_METHOD_GET 
		|| request->getMethod() == HTTPTypes::REQUEST_METHOD_HEAD)
	{
		// the type of response we will send
		enum ResponseType {
			RESPONSE_UNDEFINED,		// initial state until we know how to respond
			RESPONSE_OK,			// normal response that includes the file's content
			RESPONSE_HEAD_OK,		// response to HEAD request (would send file's content)
			RESPONSE_NOT_FOUND,		// Not Found (404)
			RESPONSE_NOT_MODIFIED	// Not Modified (304) response to If-Modified-Since
		} response_type = RESPONSE_UNDEFINED;

		// used to hold our response information
		DiskFile response_file;
		
		// get the If-Modified-Since request header
		const std::string if_modified_since(request->getHeader(HTTPTypes::HEADER_IF_MODIFIED_SINCE));
		
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
								  << getResource() << "): " << relative_path);
					response_type = RESPONSE_NOT_FOUND;
				} else {
					PION_LOG_DEBUG(m_logger, "No cache entry for request ("
								   << getResource() << "): " << relative_path);
				}
				
			} else {
				// found an existing cache entry
				
				PION_LOG_DEBUG(m_logger, "Found cache entry for request ("
							   << getResource() << "): " << relative_path);

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
						if (request->getMethod() == HTTPTypes::REQUEST_METHOD_HEAD) {
							response_type = RESPONSE_HEAD_OK;
						} else {
							response_type = RESPONSE_OK;
							PION_LOG_DEBUG(m_logger, "Cache disabled, reading file ("
										   << getResource() << "): " << relative_path);
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
					} else if (request->getMethod() == HTTPTypes::REQUEST_METHOD_HEAD) {
						response_type = RESPONSE_HEAD_OK;
					} else {
						response_type = RESPONSE_OK;
					}

					// copy cache contents so that we can release the mutex
					response_file = cache_itr->second;

					PION_LOG_DEBUG(m_logger, (cache_was_updated ? "Updated" : "Using")
								   << " cache entry for request ("
								   << getResource() << "): " << relative_path);
				}
			}
		}
		
		if (response_type == RESPONSE_UNDEFINED) {
			// make sure that the file exists
			if (! boost::filesystem::exists(file_path)) {
				PION_LOG_WARN(m_logger, "File not found ("
							  << getResource() << "): " << relative_path);
				sendNotFoundResponse(request, tcp_conn);
				return;
			}

			response_file.setFilePath(file_path);

			PION_LOG_DEBUG(m_logger, "Found file for request ("
						   << getResource() << "): " << relative_path);

			// determine the MIME type
			response_file.setMimeType(findMIMEType( response_file.getFilePath().leaf() ));

			// get the file_size and last_modified timestamp
			response_file.update();

			// just compare strings for simplicity (parsing this date format sucks!)
			if (response_file.getLastModifiedString() == if_modified_since) {
				// no need to read the file; the modified times match!
				response_type = RESPONSE_NOT_MODIFIED;
			} else if (request->getMethod() == HTTPTypes::REQUEST_METHOD_HEAD) {
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
								   << getResource() << "): " << relative_path);
					boost::mutex::scoped_lock cache_lock(m_cache_mutex);
					m_cache_map.insert( std::make_pair(relative_path, response_file) );
				}
			}
		}
		
		if (response_type == RESPONSE_OK) {
			// use DiskFileSender to send a file
			DiskFileSenderPtr sender_ptr(DiskFileSender::create(response_file,
																request, tcp_conn,
																m_max_chunk_size));
			sender_ptr->send();
		} else if (response_type == RESPONSE_NOT_FOUND) {
			sendNotFoundResponse(request, tcp_conn);
		} else {
			// sending headers only -> use our own response object
			
			// prepare a response and set the Content-Type
			HTTPResponseWriterPtr writer(HTTPResponseWriter::create(tcp_conn, *request,
										 boost::bind(&TCPConnection::finish, tcp_conn)));
			writer->getResponse().setContentType(response_file.getMimeType());
			
			// set Last-Modified header to enable client-side caching
			writer->getResponse().addHeader(HTTPTypes::HEADER_LAST_MODIFIED,
											response_file.getLastModifiedString());

			switch(response_type) {
				case RESPONSE_UNDEFINED:
				case RESPONSE_NOT_FOUND:
				case RESPONSE_OK:
					// this should never happen
					throw UndefinedResponseException(request->getResource());
					break;
				case RESPONSE_NOT_MODIFIED:
					// set "Not Modified" response
					writer->getResponse().setStatusCode(HTTPTypes::RESPONSE_CODE_NOT_MODIFIED);
					writer->getResponse().setStatusMessage(HTTPTypes::RESPONSE_MESSAGE_NOT_MODIFIED);
					break;
				case RESPONSE_HEAD_OK:
					// set "OK" response (not really necessary since this is the default)
					writer->getResponse().setStatusCode(HTTPTypes::RESPONSE_CODE_OK);
					writer->getResponse().setStatusMessage(HTTPTypes::RESPONSE_MESSAGE_OK);
					break;
			}
			
			// send the response
			writer->send();
		}
	} else if (request->getMethod() == HTTPTypes::REQUEST_METHOD_POST
			   || request->getMethod() == HTTPTypes::REQUEST_METHOD_PUT
			   || request->getMethod() == HTTPTypes::REQUEST_METHOD_DELETE)
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
			HTTPResponseWriterPtr writer(HTTPResponseWriter::create(tcp_conn, *request,
										 boost::bind(&TCPConnection::finish, tcp_conn)));
			writer->getResponse().setStatusCode(HTTPTypes::RESPONSE_CODE_METHOD_NOT_ALLOWED);
			writer->getResponse().setStatusMessage(HTTPTypes::RESPONSE_MESSAGE_METHOD_NOT_ALLOWED);
			writer->writeNoCopy(NOT_ALLOWED_HTML_START);
			writer << request->getMethod();
			writer->writeNoCopy(NOT_ALLOWED_HTML_FINISH);
			writer->getResponse().addHeader("Allow", "GET, HEAD");
			writer->send();
		} else {
			HTTPResponseWriterPtr writer(HTTPResponseWriter::create(tcp_conn, *request,
										 boost::bind(&TCPConnection::finish, tcp_conn)));
			if (request->getMethod() == HTTPTypes::REQUEST_METHOD_POST
				|| request->getMethod() == HTTPTypes::REQUEST_METHOD_PUT)
			{
				if (boost::filesystem::exists(file_path)) {
					writer->getResponse().setStatusCode(HTTPTypes::RESPONSE_CODE_NO_CONTENT);
					writer->getResponse().setStatusMessage(HTTPTypes::RESPONSE_MESSAGE_NO_CONTENT);
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
						writer->getResponse().setStatusCode(HTTPTypes::RESPONSE_CODE_NOT_FOUND);
						writer->getResponse().setStatusMessage(HTTPTypes::RESPONSE_MESSAGE_NOT_FOUND);
						writer->writeNoCopy(NOT_FOUND_HTML_START);
						writer << request->getResource();
						writer->writeNoCopy(NOT_FOUND_HTML_FINISH);
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
					writer->getResponse().setStatusCode(HTTPTypes::RESPONSE_CODE_CREATED);
					writer->getResponse().setStatusMessage(HTTPTypes::RESPONSE_MESSAGE_CREATED);
					writer->getResponse().addHeader(HTTPTypes::HEADER_LOCATION, request->getResource());
					writer->writeNoCopy(CREATED_HTML_START);
					writer << request->getResource();
					writer->writeNoCopy(CREATED_HTML_FINISH);
				}
				std::ios_base::openmode mode = request->getMethod() == HTTPTypes::REQUEST_METHOD_POST?
											   std::ios::app : std::ios::out;
				boost::filesystem::ofstream file_stream(file_path, mode);
				file_stream.write(request->getContent(), request->getContentLength());
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
					writer->getResponse().setStatusCode(HTTPTypes::RESPONSE_CODE_SERVER_ERROR);
					writer->getResponse().setStatusMessage(HTTPTypes::RESPONSE_MESSAGE_SERVER_ERROR);
					writer->writeNoCopy(PUT_FAILED_HTML_START);
					writer << request->getResource();
					writer->writeNoCopy(PUT_FAILED_HTML_FINISH);
				}
				writer->send();
			} else if (request->getMethod() == HTTPTypes::REQUEST_METHOD_DELETE) {
				if (!boost::filesystem::exists(file_path)) {
					sendNotFoundResponse(request, tcp_conn);
				} else {
					try {
						boost::filesystem::remove(file_path);
						writer->getResponse().setStatusCode(HTTPTypes::RESPONSE_CODE_NO_CONTENT);
						writer->getResponse().setStatusMessage(HTTPTypes::RESPONSE_MESSAGE_NO_CONTENT);
						writer->send();
					} catch (...) {
						static const std::string DELETE_FAILED_HTML_START =
							"<html><head>\n"
							"<title>500 Server Error</title>\n"
							"</head><body>\n"
							"<h1>Server Error</h1>\n"
							"<p>Could not delete ";
						static const std::string DELETE_FAILED_HTML_FINISH =
							".</p>\n"
							"</body></html>\n";
						writer->getResponse().setStatusCode(HTTPTypes::RESPONSE_CODE_SERVER_ERROR);
						writer->getResponse().setStatusMessage(HTTPTypes::RESPONSE_MESSAGE_SERVER_ERROR);
						writer->writeNoCopy(DELETE_FAILED_HTML_START);
						writer << request->getResource();
						writer->writeNoCopy(DELETE_FAILED_HTML_FINISH);
						writer->send();
					}
				}
			} else {
				// This should never be reached.
				writer->getResponse().setStatusCode(HTTPTypes::RESPONSE_CODE_SERVER_ERROR);
				writer->getResponse().setStatusMessage(HTTPTypes::RESPONSE_MESSAGE_SERVER_ERROR);
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
		HTTPResponseWriterPtr writer(HTTPResponseWriter::create(tcp_conn, *request,
									 boost::bind(&TCPConnection::finish, tcp_conn)));
		writer->getResponse().setStatusCode(HTTPTypes::RESPONSE_CODE_NOT_IMPLEMENTED);
		writer->getResponse().setStatusMessage(HTTPTypes::RESPONSE_MESSAGE_NOT_IMPLEMENTED);
		writer->writeNoCopy(NOT_IMPLEMENTED_HTML_START);
		writer << request->getMethod();
		writer->writeNoCopy(NOT_IMPLEMENTED_HTML_FINISH);
		writer->send();
	}
}

void FileService::sendNotFoundResponse(HTTPRequestPtr& http_request,
									   TCPConnectionPtr& tcp_conn)
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
	HTTPResponseWriterPtr writer(HTTPResponseWriter::create(tcp_conn, *http_request,
								 boost::bind(&TCPConnection::finish, tcp_conn)));
	writer->getResponse().setStatusCode(HTTPTypes::RESPONSE_CODE_NOT_FOUND);
	writer->getResponse().setStatusMessage(HTTPTypes::RESPONSE_MESSAGE_NOT_FOUND);
	if (http_request->getMethod() != HTTPTypes::REQUEST_METHOD_HEAD) {
		writer->writeNoCopy(NOT_FOUND_HTML_START);
		writer << http_request->getResource();
		writer->writeNoCopy(NOT_FOUND_HTML_FINISH);
	}
	writer->send();
}

void FileService::start(void)
{
	PION_LOG_DEBUG(m_logger, "Starting up resource (" << getResource() << ')');

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
	PION_LOG_DEBUG(m_logger, "Shutting down resource (" << getResource() << ')');
	// clear cached files (if started again, it will re-scan)
	boost::mutex::scoped_lock cache_lock(m_cache_mutex);
	m_cache_map.clear();
}

void FileService::scanDirectory(const boost::filesystem::path& dir_path)
{
	PION_LOG_DEBUG(m_logger, "Scanning directory (" << getResource() << "): "
				   << dir_path.directory_string());
	
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
			std::string file_path_string( itr->path().file_string() );
			std::string relative_path( file_path_string.substr(m_directory.directory_string().size() + 1) );
			
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
	DiskFile cache_entry(file_path, NULL, 0, 0, findMIMEType(file_path.leaf()));
	if (! placeholder) {
		cache_entry.update();
		// only read the file if its size is <= max_cache_size
		if (m_max_cache_size==0 || cache_entry.getFileSize() <= m_max_cache_size) {
			try { cache_entry.read(); }
			catch (std::exception&) {
				PION_LOG_ERROR(m_logger, "Unable to add file to cache: "
							   << file_path.file_string());
				return std::make_pair(m_cache_map.end(), false);
			}
		}
	}
	
	std::pair<CacheMap::iterator, bool> add_entry_result
		= m_cache_map.insert( std::make_pair(relative_path, cache_entry) );
	
	if (add_entry_result.second) {
		PION_LOG_DEBUG(m_logger, "Added file to cache: "
					   << file_path.file_string());
	} else {
		PION_LOG_ERROR(m_logger, "Unable to insert cache entry for file: "
					   << file_path.file_string());
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
	// ...
	
	// set the static pointer
	m_mime_types_ptr = &mime_types;
}


// DiskFile member functions

void DiskFile::update(void)
{
	// set file_size and last_modified
	m_file_size = boost::filesystem::file_size( m_file_path );
	m_last_modified = boost::filesystem::last_write_time( m_file_path );
	m_last_modified_string = HTTPTypes::get_date_string( m_last_modified );
}

void DiskFile::read(void)
{
	// re-allocate storage buffer for the file's content
	m_file_content.reset(new char[m_file_size]);
	
	// open the file for reading
	boost::filesystem::ifstream file_stream;
	file_stream.open(m_file_path, std::ios::in | std::ios::binary);

	// read the file into memory
	if (!file_stream.is_open() || !file_stream.read(m_file_content.get(), m_file_size))
		throw FileService::FileReadException(m_file_path.file_string());
}

bool DiskFile::checkUpdated(void)
{
	// get current values
	unsigned long cur_size = boost::filesystem::file_size( m_file_path );
	time_t cur_modified = boost::filesystem::last_write_time( m_file_path );

	// check if file has not been updated
	if (cur_modified == m_last_modified && cur_size == m_file_size)
		return false;

	// file has been updated
		
	// update file_size and last_modified timestamp
	m_file_size = cur_size;
	m_last_modified = cur_modified;
	m_last_modified_string = HTTPTypes::get_date_string( m_last_modified );

	// read new contents
	read();
	
	return true;
}


// DiskFileSender member functions

DiskFileSender::DiskFileSender(DiskFile& file, pion::net::HTTPRequestPtr& request,
							   pion::net::TCPConnectionPtr& tcp_conn,
							   unsigned long max_chunk_size)
	: m_logger(PION_GET_LOGGER("pion.FileService.DiskFileSender")), m_disk_file(file),
	m_writer(pion::net::HTTPResponseWriter::create(tcp_conn, *request, boost::bind(&TCPConnection::finish, tcp_conn))),
	m_max_chunk_size(max_chunk_size), m_file_bytes_to_send(0), m_bytes_sent(0)
{
	PION_LOG_DEBUG(m_logger, "Preparing to send file"
				   << (m_disk_file.hasFileContent() ? " (cached): " : ": ")
				   << m_disk_file.getFilePath().file_string());
	
		// set the Content-Type HTTP header using the file's MIME type
	m_writer->getResponse().setContentType(m_disk_file.getMimeType());
	
	// set Last-Modified header to enable client-side caching
	m_writer->getResponse().addHeader(HTTPTypes::HEADER_LAST_MODIFIED,
									  m_disk_file.getLastModifiedString());

	// use "200 OK" HTTP response
	m_writer->getResponse().setStatusCode(HTTPTypes::RESPONSE_CODE_OK);
	m_writer->getResponse().setStatusMessage(HTTPTypes::RESPONSE_MESSAGE_OK);
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
				PION_LOG_ERROR(m_logger, "Unable to open file: "
							   << m_disk_file.getFilePath().file_string());
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
				PION_LOG_ERROR(m_logger, "File size inconsistency: "
							   << m_disk_file.getFilePath().file_string());
			} else {
				PION_LOG_ERROR(m_logger, "Unable to read file: "
							   << m_disk_file.getFilePath().file_string());
			}
			return;
		}
	}

	// send the content
	m_writer->writeNoCopy(file_content_ptr, m_file_bytes_to_send);
	
	if (m_bytes_sent + m_file_bytes_to_send >= m_disk_file.getFileSize()) {
		// this is the last piece of data to send
		if (m_bytes_sent > 0) {
			// send last chunk in a series
			m_writer->sendFinalChunk(boost::bind(&DiskFileSender::handleWrite,
												 shared_from_this(),
												 boost::asio::placeholders::error,
												 boost::asio::placeholders::bytes_transferred));
		} else {
			// sending entire file at once
			m_writer->send(boost::bind(&DiskFileSender::handleWrite,
									   shared_from_this(),
									   boost::asio::placeholders::error,
									   boost::asio::placeholders::bytes_transferred));
		}
	} else {
		// there will be more data -> send a chunk
		m_writer->sendChunk(boost::bind(&DiskFileSender::handleWrite,
										shared_from_this(),
										boost::asio::placeholders::error,										
										boost::asio::placeholders::bytes_transferred));
	}
}

void DiskFileSender::handleWrite(const boost::system::error_code& write_error,
								 std::size_t bytes_written)
{
	bool finished_sending = true;

	if (write_error) {
		// encountered error sending response data
		m_writer->getTCPConnection()->setLifecycle(TCPConnection::LIFECYCLE_CLOSE);	// make sure it will get closed
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
						   << (m_writer->getTCPConnection()->getKeepAlive() ? ", keeping alive)" : ", closing)") );
		} else {
			// NOT finished sending
			PION_LOG_DEBUG(m_logger, "Sent file chunk of " << m_file_bytes_to_send << " bytes");
			finished_sending = false;
			m_writer->clear();
		}
	}

	if (finished_sending) {
		// TCPConnection::finish() calls TCPServer::finishConnection, which will either:
		// a) call HTTPServer::handleConnection again if keep-alive is true; or,
		// b) close the socket and remove it from the server's connection pool
		m_writer->getTCPConnection()->finish();
	} else {
		send();
	}
}


/// creates new FileService objects
extern "C" PION_SERVICE_API FileService *pion_create_FileService(void)
{
	return new FileService();
}


/// destroys FileService objects
extern "C" PION_SERVICE_API void pion_destroy_FileService(FileService *service_ptr)
{
	delete service_ptr;
}
