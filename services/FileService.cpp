// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/scoped_array.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>
#include <algorithm>

#include "FileService.hpp"
#include <pion/PionPlugin.hpp>
#include <pion/net/HTTPResponse.hpp>

using namespace pion;
using namespace pion::net;


// static members of FileService

const std::string			FileService::DEFAULT_MIME_TYPE("application/octet-stream");
const unsigned int			FileService::DEFAULT_CACHE_SETTING = 1;
const unsigned int			FileService::DEFAULT_SCAN_SETTING = 0;
boost::once_flag			FileService::m_mime_types_init_flag = BOOST_ONCE_INIT;
FileService::MIMETypeMap	*FileService::m_mime_types_ptr = NULL;


// FileService member functions

FileService::FileService(void)
	: m_logger(PION_GET_LOGGER("FileService")),
	m_cache_setting(DEFAULT_CACHE_SETTING),
	m_scan_setting(DEFAULT_SCAN_SETTING)
{
	PION_LOG_SETLEVEL_WARN(m_logger);
}

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
	} else {
		throw UnknownOptionException(name);
	}
}

bool FileService::handleRequest(HTTPRequestPtr& request, TCPConnectionPtr& tcp_conn)
{
	// the type of response we will send
	enum ResponseType {
		RESPONSE_UNDEFINED,		// initial state until we know how to respond
		RESPONSE_OK,			// normal response that includes the file's content
		RESPONSE_HEAD_OK,		// response to HEAD request (would send file's content)
		RESPONSE_NOT_MODIFIED	// Not Modified (304) response to If-Modified-Since
	} response_type = RESPONSE_UNDEFINED;

	// used to hold our response information
	DiskFile response_file;
	
	// get the If-Modified-Since request header
	const std::string if_modified_since(request->getHeader(HTTPTypes::HEADER_IF_MODIFIED_SINCE));
	
	// get the relative resource path for the request
	const std::string relative_path(getRelativeResource(request->getResource()));
	
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
				return false;
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
				response_file.file_path = cache_itr->second.file_path;
				response_file.mime_type = cache_itr->second.mime_type;
				
				// get the file_size and last_modified timestamp
				response_file.update();

				// just compare strings for simplicity (parsing this date format sucks!)
				if (response_file.last_modified_string == if_modified_since) {
					// no need to read the file; the modified times match!
					response_type = RESPONSE_NOT_MODIFIED;
				} else {
					if (request->getMethod() == HTTPTypes::REQUEST_METHOD_HEAD) {
						response_type = RESPONSE_HEAD_OK;
					} else {
						response_type = RESPONSE_OK;

						// read the file (may throw exception)
						response_file.read();

						PION_LOG_DEBUG(m_logger, "Cache disabled, reading file ("
									   << getResource() << "): " << relative_path);
					}
				}
					
			} else {
				// cache is enabled
				
				// true if the entry was updated (used for log message)
				bool cache_was_updated = false;

				if (cache_itr->second.last_modified == 0) {

					// cache file for the first time
					cache_was_updated = true;
					cache_itr->second.update();
					// read the file (may throw exception)
					cache_itr->second.read();
					
				} else if (m_cache_setting == 1) {

					// check if file has been updated (may throw exception)
					cache_was_updated = cache_itr->second.checkUpdated();
					
				} // else cache_setting == 2 (use existing values)

				// get the response type
				if (cache_itr->second.last_modified_string == if_modified_since) {
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
		// determine the path to the file
		
		if (relative_path.empty()) {
			// request matches resource exactly

			// return false if no file defined
			// web service's directory is not valid
			if (m_file.empty()) {
				PION_LOG_WARN(m_logger, "No file option defined ("
							  << getResource() << "): " << relative_path);
				return false;
			}
			
			// use file to service directory request
			response_file.file_path = m_file;

		} else if (! m_directory.empty()) {
			// request does not match resource, and a directory is defined

			// calculate the location of the file being requested
			response_file.file_path = m_directory;
			response_file.file_path /= relative_path;

			// make sure that the file exists
			if (! boost::filesystem::exists(response_file.file_path) ) {
				PION_LOG_WARN(m_logger, "File not found ("
							  << getResource() << "): " << relative_path);
				return false;
			}
			
			// make sure that it is not a directory
			if ( boost::filesystem::is_directory(response_file.file_path) ) {
				PION_LOG_WARN(m_logger, "Request for directory ("
							  << getResource() << "): " << relative_path);
				return false;
			}
			
			// make sure that the file is within the configured directory
			std::string file_string = response_file.file_path.file_string();
			if (file_string.find(m_directory.directory_string()) != 0) {
				PION_LOG_WARN(m_logger, "Request for file outside of directory ("
							  << getResource() << "): " << relative_path);
				return false;
			}
			
		} else {
			// request does not match resource, and no directory is defined
			return false;
		}

		PION_LOG_DEBUG(m_logger, "Found file for request ("
					   << getResource() << "): " << relative_path);

		// determine the MIME type
		response_file.mime_type = findMIMEType( response_file.file_path.leaf() );

		// get the file_size and last_modified timestamp
		response_file.update();

		// just compare strings for simplicity (parsing this date format sucks!)
		if (response_file.last_modified_string == if_modified_since) {
			// no need to read the file; the modified times match!
			response_type = RESPONSE_NOT_MODIFIED;
		} else if (request->getMethod() == HTTPTypes::REQUEST_METHOD_HEAD) {
			response_type = RESPONSE_HEAD_OK;
		} else {
			response_type = RESPONSE_OK;
			// read the file (may throw exception)
			response_file.read();
			if (m_cache_setting != 0) {
				// add new entry to the cache
				PION_LOG_DEBUG(m_logger, "Adding cache entry for request ("
							   << getResource() << "): " << relative_path);
				boost::mutex::scoped_lock cache_lock(m_cache_mutex);
				m_cache_map.insert( std::make_pair(relative_path, response_file) );
			}
		}
	}
		
	// prepare a response and set the Content-Type
	HTTPResponsePtr response(HTTPResponse::create());
	response->setContentType(response_file.mime_type);
	
	// set Last-Modified header to enable client-side caching
	response->addHeader(HTTPTypes::HEADER_LAST_MODIFIED, response_file.last_modified_string);

	switch(response_type) {
		case RESPONSE_UNDEFINED:
			// this should never happen
			throw UndefinedResponseException(request->getResource());
			break;
		case RESPONSE_NOT_MODIFIED:
			// set "Not Modified" response
			response->setResponseCode(HTTPTypes::RESPONSE_CODE_NOT_MODIFIED);
			response->setResponseMessage(HTTPTypes::RESPONSE_MESSAGE_NOT_MODIFIED);
			break;
		case RESPONSE_OK:
			// write the file's contents to the response stream
			if (response_file.file_size != 0)
				response->write(response_file.file_content.get(), response_file.file_size);
			// fall through to RESPONSE_HEAD_OK
		case RESPONSE_HEAD_OK:
			// set "OK" response (not really necessary since this is the default)
			response->setResponseCode(HTTPTypes::RESPONSE_CODE_OK);
			response->setResponseMessage(HTTPTypes::RESPONSE_MESSAGE_OK);
			break;
	}
	
	// send the response
	response->send(tcp_conn);

	return true;
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
		try { cache_entry.read(); }
		catch (std::exception&) {
			PION_LOG_ERROR(m_logger, "Unable to add file to cache: "
						   << file_path.file_string());
			return std::make_pair(m_cache_map.end(), false);
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
	std::transform(extension.begin(), extension.end(),
				   extension.begin(), tolower);
	
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


// FileService::DiskFile member functions

void FileService::DiskFile::update(void)
{
	// set file_size and last_modified
	file_size = boost::filesystem::file_size( file_path );
	last_modified = boost::filesystem::last_write_time( file_path );
	last_modified_string = HTTPTypes::get_date_string( last_modified );
}

void FileService::DiskFile::read(void)
{
	// re-allocate storage buffer for the file's content
	file_content.reset(new char[file_size]);
	
	// open the file for reading
	boost::filesystem::ifstream file_stream;
	file_stream.open(file_path, std::ios::in | std::ios::binary);

	// read the file into memory
	if (!file_stream.is_open() || !file_stream.read(file_content.get(), file_size))
		throw FileService::FileReadException(file_path.file_string());
}

bool FileService::DiskFile::checkUpdated(void)
{
	// get current values
	unsigned long cur_size = boost::filesystem::file_size( file_path );
	time_t cur_modified = boost::filesystem::last_write_time( file_path );

	// check if file has not been updated
	if (cur_modified == last_modified && cur_size == file_size)
		return false;

	// file has been updated
		
	// update file_size and last_modified timestamp
	file_size = cur_size;
	last_modified = cur_modified;
	last_modified_string = HTTPTypes::get_date_string( last_modified );

	// read new contents
	read();
	
	return true;
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
