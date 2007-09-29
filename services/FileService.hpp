// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_FILESERVICE_HEADER__
#define __PION_FILESERVICE_HEADER__

#include <boost/functional/hash.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/thread/once.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/shared_array.hpp>
#include <pion/PionLogger.hpp>
#include <pion/PionException.hpp>
#include <pion/PionHashMap.hpp>
#include <pion/net/WebService.hpp>
#include <string>
#include <map>


///
/// FileService: web service that serves regular files
/// 
class FileService :
	public pion::net::WebService
{
public:
	
	/// exception thrown if the directory configured is not found
	class DirectoryNotFoundException : public pion::PionException {
	public:
		DirectoryNotFoundException(const std::string& dir)
			: pion::PionException("FileService directory not found: ", dir) {}
	};

	/// exception thrown if the directory configuration option is not a directory
	class NotADirectoryException : public pion::PionException {
	public:
		NotADirectoryException(const std::string& dir)
			: pion::PionException("FileService option is not a directory: ", dir) {}
	};

	/// exception thrown if the file configured is not found
	class FileNotFoundException : public pion::PionException {
	public:
		FileNotFoundException(const std::string& file)
			: pion::PionException("FileService file not found: ", file) {}
	};
	
	/// exception thrown if the file configuration option is not a file
	class NotAFileException : public pion::PionException {
	public:
		NotAFileException(const std::string& file)
			: pion::PionException("FileService option is not a file: ", file) {}
	};

	/// exception thrown if the cache option is set to an invalid value
	class InvalidCacheException : public pion::PionException {
	public:
		InvalidCacheException(const std::string& value)
			: pion::PionException("FileService invalid value for cache option: ", value) {}
	};

	/// exception thrown if the cache option is set to an invalid value
	class InvalidScanException : public pion::PionException {
	public:
		InvalidScanException(const std::string& value)
			: pion::PionException("FileService invalid value for scan option: ", value) {}
	};

	/// exception thrown if we are unable to read a file from disk
	class FileReadException : public pion::PionException {
	public:
		FileReadException(const std::string& value)
			: pion::PionException("FileService unable to read file: ", value) {}
	};

	/// exception thrown if we do not know how to response (should never happen)
	class UndefinedResponseException : public pion::PionException {
	public:
		UndefinedResponseException(const std::string& value)
			: pion::PionException("FileService has an undefined response: ", value) {}
	};
	
	
	// default constructor and destructor
	FileService(void);
	virtual ~FileService() {}
	
	/**
	 * configuration options supported by FileService:
	 *
	 * directory: all files within the directory will be made available
	 */
	virtual void setOption(const std::string& name, const std::string& value);
	
	/// handles requests for FileService
	virtual bool handleRequest(pion::net::HTTPRequestPtr& request,
							   pion::net::TCPConnectionPtr& tcp_conn);	
	
	/// called when the web service's server is starting
	virtual void start(void);
	
	/// called when the web service's server is stopping
	virtual void stop(void);
	
	/// sets the logger to be used
	inline void setLogger(pion::PionLogger log_ptr) { m_logger = log_ptr; }
	
	/// returns the logger currently in use
	inline pion::PionLogger getLogger(void) { return m_logger; }

	
protected:

	/// data type representing files on disk
	struct DiskFile {
		/// constructors for convenience
		DiskFile(void)
			: file_size(0), last_modified(0) {}
		DiskFile(const boost::filesystem::path& path,
				 char *content, unsigned long size,
				 std::time_t modified, const std::string& mime)
			: file_path(path), file_content(content), file_size(size),
			last_modified(modified), mime_type(mime) {}
		/// updates the file_size and last_modified timestamp to disk
		void update(void);
		/// reads content from disk into file_content buffer
		void read(void);
		/**
			* checks if the file has been updated and updates vars if it has
		 *
		 * @return true if the file was updated
		 */
		bool checkUpdated(void);
		/// path to the cached file
		boost::filesystem::path		file_path;
		/// content of the cached file
		boost::shared_array<char>	file_content;
		/// size of the file's content
		unsigned long				file_size;
		/// timestamp that the cached file was last modified (0 = cache disabled)
		std::time_t					last_modified;
		/// timestamp that the cached file was last modified (string format)
		std::string					last_modified_string;
		/// mime type for the cached file
		std::string					mime_type;
	};
	
	/// data type for map of file names to cache entries
	typedef PION_HASH_MAP<std::string, DiskFile, PION_HASH_STRING >		CacheMap;
	
	/// data type for map of file extensions to MIME types
	typedef PION_HASH_MAP<std::string, std::string, PION_HASH_STRING >	MIMETypeMap;
	
	/**
	 * adds all files within a directory to the cache
	 *
	 * @param dir_path the directory to scan (sub-directories are included)
	 */
	void scanDirectory(const boost::filesystem::path& dir_path);
	
	/**
	 * adds a single file to the cache
	 *
	 * @param relative_path path for the file relative to the root directory
	 * @param file_path actual path to the file on disk
	 * @param placeholder if true, the file's contents are not cached
	 *
	 * @return std::pair<CacheMap::iterator, bool> if an entry is added to the
	 *         cache, second will be true and first will point to the new entry
	 */
	std::pair<CacheMap::iterator, bool>
		addCacheEntry(const std::string& relative_path,
					  const boost::filesystem::path& file_path,
					  const bool placeholder);

	/**
	 * searches for a MIME type that matches a file
	 *
	 * @param file_name name of the file to search for
	 * @return MIME type corresponding with the file, or DEFAULT_MIME_TYPE if none found
	 */
	static std::string findMIMEType(const std::string& file_name);

	
	/// primary logging interface used by this class
	pion::PionLogger			m_logger;
	
	
private:
	
	/// function called once to initialize the map of MIME types
	static void createMIMETypes(void);
	
	
	/// mime type used if no others are found for the file's extension
	static const std::string	DEFAULT_MIME_TYPE;
	
	/// default setting for cache configuration option
	static const unsigned int	DEFAULT_CACHE_SETTING;

	/// default setting for scan configuration option
	static const unsigned int	DEFAULT_SCAN_SETTING;

	/// flag used to make sure that createMIMETypes() is called only once
	static boost::once_flag		m_mime_types_init_flag;
	
	/// map of file extensions to MIME types
	static MIMETypeMap *		m_mime_types_ptr;
	
	
	/// directory containing files that will be made available
	boost::filesystem::path		m_directory;
	
	/// single file served by the web service
	boost::filesystem::path		m_file;
	
	/// used to cache file contents and metadata in memory
	CacheMap					m_cache_map;
	
	/// mutex used to make the file cache thread-safe
	boost::mutex				m_cache_mutex;
	
	/**
	 * cache configuration setting:
	 * 0 = do not cache files in memory
	 * 1 = cache files in memory when requested, check for any updates
	 * 2 = cache files in memory when requested, ignore any updates
	 */
	unsigned int				m_cache_setting;

	/**
	 * scan configuration setting (only applies to directories):
	 * 0 = do not scan the directory; allow files to be added at any time
	 * 1 = scan directory when started, and do not allow files to be added
	 * 2 = scan directory and pre-populate cache; allow new files
	 * 3 = scan directory and pre-populate cache; ignore new files
	 */
	unsigned int				m_scan_setting;
};

#endif
