// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTPMESSAGE_HEADER__
#define __PION_HTTPMESSAGE_HEADER__

#include <iosfwd>
#include <vector>
#include <cstring>
#include <boost/cstdint.hpp>
#include <boost/asio.hpp>
#include <boost/scoped_array.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/regex.hpp>
#include <pion/config.hpp>
#include <pion/http/types.hpp>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)


// forward declaration for class used by send() and receive()
class TCPConnection;


///
/// HTTPMessage: base container for HTTP messages
/// 
class PION_NET_API HTTPMessage
	: public HTTPTypes
{
public:

	/// data type for I/O write buffers (these wrap existing data to be sent)
	typedef std::vector<boost::asio::const_buffer>	WriteBuffers;

	/// used to cache chunked data
	typedef std::vector<char>	ChunkCache;

	/// data type for library errors returned during receive() operations
	struct ReceiveError
		: public boost::system::error_category
	{
		virtual ~ReceiveError() {}
		virtual inline const char *name() const { return "ReceiveError"; }
		virtual inline std::string message(int ev) const {
			std::string result;
			switch(ev) {
				case 1:
					result = "HTTP message parsing error";
					break;
				default:
					result = "Unknown receive error";
					break;
			}
			return result;
		}
	};

	/// defines message data integrity status codes
	enum DataStatus
	{
		STATUS_NONE,		// no data received (i.e. all lost)
		STATUS_TRUNCATED,	// one or more missing packets at the end 
		STATUS_PARTIAL,		// one or more missing packets but NOT at the end 
		STATUS_OK			// no missing packets
	};

	/// constructs a new HTTP message object
	HTTPMessage(void)
		: m_is_valid(false), m_is_chunked(false), m_chunks_supported(false),
		m_do_not_send_content_length(false),
		m_version_major(1), m_version_minor(1), m_content_length(0), m_content_buf(),
		m_status(STATUS_NONE), m_has_missing_packets(false), m_has_data_after_missing(false)
	{}

	/// copy constructor
	HTTPMessage(const HTTPMessage& http_msg)
		: m_first_line(http_msg.m_first_line),
		m_is_valid(http_msg.m_is_valid),
		m_is_chunked(http_msg.m_is_chunked),
		m_chunks_supported(http_msg.m_chunks_supported),
		m_do_not_send_content_length(http_msg.m_do_not_send_content_length),
		m_remote_ip(http_msg.m_remote_ip),
		m_version_major(http_msg.m_version_major),
		m_version_minor(http_msg.m_version_minor),
		m_content_length(http_msg.m_content_length),
		m_content_buf(http_msg.m_content_buf),
		m_chunk_cache(http_msg.m_chunk_cache),
		m_headers(http_msg.m_headers),
		m_status(http_msg.m_status),
		m_has_missing_packets(http_msg.m_has_missing_packets),
		m_has_data_after_missing(http_msg.m_has_data_after_missing)
	{}

	/// assignment operator
	inline HTTPMessage& operator=(const HTTPMessage& http_msg) {
		m_first_line = http_msg.m_first_line;
		m_is_valid = http_msg.m_is_valid;
		m_is_chunked = http_msg.m_is_chunked;
		m_chunks_supported = http_msg.m_chunks_supported;
		m_do_not_send_content_length = http_msg.m_do_not_send_content_length;
		m_remote_ip = http_msg.m_remote_ip;
		m_version_major = http_msg.m_version_major;
		m_version_minor = http_msg.m_version_minor;
		m_content_length = http_msg.m_content_length;
		m_content_buf = http_msg.m_content_buf;
		m_chunk_cache = http_msg.m_chunk_cache;
		m_headers = http_msg.m_headers;
		m_status = http_msg.m_status;
		m_has_missing_packets = http_msg.m_has_missing_packets;
		m_has_data_after_missing = http_msg.m_has_data_after_missing;
		return *this;
	}

	/// virtual destructor
	virtual ~HTTPMessage() {}

	/// clears all message data
	virtual void clear(void) {
		clearFirstLine();
		m_is_valid = m_is_chunked = m_chunks_supported
			= m_do_not_send_content_length = false;
		m_remote_ip = boost::asio::ip::address_v4(0);
		m_version_major = m_version_minor = 1;
		m_content_length = 0;
		m_content_buf.clear();
		m_chunk_cache.clear();
		m_headers.clear();
		m_cookie_params.clear();
		m_status = STATUS_NONE;
		m_has_missing_packets = false;
		m_has_data_after_missing = false;
	}

	/// should return true if the content length can be implied without headers
	virtual bool isContentLengthImplied(void) const = 0;

	/// returns true if the message is valid
	inline bool isValid(void) const { return m_is_valid; }

	/// returns true if chunked transfer encodings are supported
	inline bool getChunksSupported(void) const { return m_chunks_supported; }

	/// returns IP address of the remote endpoint
	inline boost::asio::ip::address& getRemoteIp(void) {
		return m_remote_ip;
	}

	/// returns the major HTTP version number
	inline boost::uint16_t getVersionMajor(void) const { return m_version_major; }

	/// returns the minor HTTP version number
	inline boost::uint16_t getVersionMinor(void) const { return m_version_minor; }

	/// returns a string representation of the HTTP version (i.e. "HTTP/1.1")
	inline std::string getVersionString(void) const {
		std::string http_version(STRING_HTTP_VERSION);
		http_version += boost::lexical_cast<std::string>(getVersionMajor());
		http_version += '.';
		http_version += boost::lexical_cast<std::string>(getVersionMinor());
		return http_version;
	}

	/// returns the length of the payload content (in bytes)
	inline boost::uint64_t getContentLength(void) const { return m_content_length; }

	/// returns true if the message content is chunked
	inline bool isChunked(void) const { return m_is_chunked; }

	/// returns true if buffer for content is allocated
	bool isContentBufferAllocated() const { return !m_content_buf.is_empty(); }
	
	/// returns size of allocated buffer
	inline std::size_t getContentBufferSize() const { return m_content_buf.size(); }
	
	/// returns a pointer to the payload content, or empty string if there is none
	inline char *getContent(void) { return m_content_buf.get(); }

	/// returns a const pointer to the payload content, or empty string if there is none
	inline const char *getContent(void) const { return m_content_buf.get(); }

	/// returns a reference to the chunk cache
	inline ChunkCache& getChunkCache(void) { return m_chunk_cache; }

	/// returns a value for the header if any are defined; otherwise, an empty string
	inline const std::string& getHeader(const std::string& key) const {
		return getValue(m_headers, key);
	}

	/// returns a reference to the HTTP headers
	inline Headers& getHeaders(void) {
		return m_headers;
	}

	/// returns true if at least one value for the header is defined
	inline bool hasHeader(const std::string& key) const {
		return(m_headers.find(key) != m_headers.end());
	}

	/// returns a value for the cookie if any are defined; otherwise, an empty string
	/// since cookie names are insensitive, key should use lowercase alpha chars
	inline const std::string& getCookie(const std::string& key) const {
		return getValue(m_cookie_params, key);
	}
	
	/// returns the cookie parameters
	inline CookieParams& getCookieParams(void) {
		return m_cookie_params;
	}

	/// returns true if at least one value for the cookie is defined
	/// since cookie names are insensitive, key should use lowercase alpha chars
	inline bool hasCookie(const std::string& key) const {
		return(m_cookie_params.find(key) != m_cookie_params.end());
	}
	
	/// adds a value for the cookie
	/// since cookie names are insensitive, key should use lowercase alpha chars
	inline void addCookie(const std::string& key, const std::string& value) {
		m_cookie_params.insert(std::make_pair(key, value));
	}

	/// changes the value of a cookie
	/// since cookie names are insensitive, key should use lowercase alpha chars
	inline void changeCookie(const std::string& key, const std::string& value) {
		changeValue(m_cookie_params, key, value);
	}

	/// removes all values for a cookie
	/// since cookie names are insensitive, key should use lowercase alpha chars
	inline void deleteCookie(const std::string& key) {
		deleteValue(m_cookie_params, key);
	}
	
	/// returns a string containing the first line for the HTTP message
	inline const std::string& getFirstLine(void) const {
		if (m_first_line.empty())
			updateFirstLine();
		return m_first_line;
	}

	/// true if there were missing packets 
	inline bool hasMissingPackets() const { return m_has_missing_packets; }
	
	/// set to true when missing packets detected
	inline void setMissingPackets(bool newVal) { m_has_missing_packets = newVal; }

	/// true if more data seen after the missing packets
	inline bool hasDataAfterMissingPackets() const { return m_has_data_after_missing; }

	inline void setDataAfterMissingPacket(bool newVal) { m_has_data_after_missing = newVal; }

	/// sets whether or not the message is valid
	inline void setIsValid(bool b = true) { m_is_valid = b; }

	/// set to true if chunked transfer encodings are supported
	inline void setChunksSupported(bool b) { m_chunks_supported = b; }

	/// sets IP address of the remote endpoint
	inline void setRemoteIp(const boost::asio::ip::address& ip) { m_remote_ip = ip; }

	/// sets the major HTTP version number
	inline void setVersionMajor(const boost::uint16_t n) {
		m_version_major = n;
		clearFirstLine();
	}

	/// sets the minor HTTP version number
	inline void setVersionMinor(const boost::uint16_t n) {
		m_version_minor = n;
		clearFirstLine();
	}

	/// sets the length of the payload content (in bytes)
	inline void setContentLength(const boost::uint64_t n) { m_content_length = n; }

	/// if called, the content-length will not be sent in the HTTP headers
	inline void setDoNotSendContentLength(void) { m_do_not_send_content_length = true; }

	/// return the data receival status
	inline DataStatus getStatus() const { return m_status; }

	/// 
	inline void setStatus(DataStatus newVal) { m_status = newVal; }

	/// sets the length of the payload content using the Content-Length header
	inline void updateContentLengthUsingHeader(void) {
		Headers::const_iterator i = m_headers.find(HEADER_CONTENT_LENGTH);
		if (i == m_headers.end()) {
			m_content_length = 0;
		} else {
			std::string trimmed_length(i->second);
			boost::algorithm::trim(trimmed_length);
			m_content_length = boost::lexical_cast<boost::uint64_t>(trimmed_length);
		}
	}

	/// sets the transfer coding using the Transfer-Encoding header
	inline void updateTransferCodingUsingHeader(void) {
		m_is_chunked = false;
		Headers::const_iterator i = m_headers.find(HEADER_TRANSFER_ENCODING);
		if (i != m_headers.end()) {
			// From RFC 2616, sec 3.6: All transfer-coding values are case-insensitive.
			m_is_chunked = boost::regex_match(i->second, REGEX_ICASE_CHUNKED);
			// ignoring other possible values for now
		}
	}

	///creates a payload content buffer of size m_content_length and returns
	/// a pointer to the new buffer (memory is managed by HTTPMessage class)
	inline char *createContentBuffer(void) {
		m_content_buf.resize(m_content_length);
		return m_content_buf.get();
	}
	
	/// resets payload content to match the value of a string
	inline void setContent(const std::string& content) {
		setContentLength(content.size());
		createContentBuffer();
		memcpy(m_content_buf.get(), content.c_str(), content.size());
	}

	/// clears payload content buffer
	inline void clearContent(void) {
		setContentLength(0);
		createContentBuffer();
		deleteValue(m_headers, HEADER_CONTENT_TYPE);
	}

	/// sets the content type for the message payload
	inline void setContentType(const std::string& type) {
		changeValue(m_headers, HEADER_CONTENT_TYPE, type);
	}

	/// adds a value for the HTTP header named key
	inline void addHeader(const std::string& key, const std::string& value) {
		m_headers.insert(std::make_pair(key, value));
	}

	/// changes the value for the HTTP header named key
	inline void changeHeader(const std::string& key, const std::string& value) {
		changeValue(m_headers, key, value);
	}

	/// removes all values for the HTTP header named key
	inline void deleteHeader(const std::string& key) {
		deleteValue(m_headers, key);
	}

	/// returns true if the HTTP connection may be kept alive
	inline bool checkKeepAlive(void) const {
		return (getHeader(HEADER_CONNECTION) != "close"
				&& (getVersionMajor() > 1
					|| (getVersionMajor() >= 1 && getVersionMinor() >= 1)) );
	}

	/**
	 * initializes a vector of write buffers with the HTTP message information
	 *
	 * @param write_buffers vector of write buffers to initialize
	 * @param keep_alive true if the connection should be kept alive
	 * @param using_chunks true if the payload content will be sent in chunks
	 */
	inline void prepareBuffersForSend(WriteBuffers& write_buffers,
									  const bool keep_alive,
									  const bool using_chunks)
	{
		// update message headers
		prepareHeadersForSend(keep_alive, using_chunks);
		// add first message line
		write_buffers.push_back(boost::asio::buffer(getFirstLine()));
		write_buffers.push_back(boost::asio::buffer(STRING_CRLF));
		// append HTTP headers
		appendHeaders(write_buffers);
	}


	/**
	 * sends the message over a TCP connection (blocks until finished)
	 *
	 * @param tcp_conn TCP connection to use
	 * @param ec contains error code if the send fails
	 * @param headers_only if true then only HTTP headers are sent
	 *
	 * @return std::size_t number of bytes written to the connection
	 */
	std::size_t send(TCPConnection& tcp_conn, boost::system::error_code& ec,
		bool headers_only = false);

	/**
	 * receives a new message from a TCP connection (blocks until finished)
	 *
	 * @param tcp_conn TCP connection to use
	 * @param ec contains error code if the receive fails
	 * @param headers_only if true then only HTTP headers are received
	 *
	 * @return std::size_t number of bytes read from the connection
	 */
	std::size_t receive(TCPConnection& tcp_conn, boost::system::error_code& ec,
		bool headers_only = false);

	/**
	 * writes the message to a std::ostream (blocks until finished)
	 *
	 * @param out std::ostream to use
	 * @param ec contains error code if the write fails
	 * @param headers_only if true then only HTTP headers are written
	 *
	 * @return std::size_t number of bytes written to the connection
	 */
	std::size_t write(std::ostream& out, boost::system::error_code& ec,
		bool headers_only = false);

	/**
	 * reads a new message from a std::istream (blocks until finished)
	 *
	 * @param in std::istream to use
	 * @param ec contains error code if the read fails
	 * @param headers_only if true then only HTTP headers are read
	 *
	 * @return std::size_t number of bytes read from the connection
	 */
	std::size_t read(std::istream& in, boost::system::error_code& ec,
		bool headers_only = false);

	/**
	 * pieces together all the received chunks
	 */
	void concatenateChunks(void);


protected:
	
	/// a simple helper class used to manage a fixed-size payload content buffer
	class ContentBuffer {
	public:
		/// simple destructor
		~ContentBuffer() {}

		/// default constructor
		ContentBuffer() : m_buf(), m_len(0), m_empty(0), m_ptr(&m_empty) {}

		/// copy constructor
		ContentBuffer(const ContentBuffer& buf)
			: m_buf(), m_len(0), m_empty(0), m_ptr(&m_empty)
		{
			if (buf.size()) {
				resize(buf.size());
				memcpy(get(), buf.get(), buf.size());
			}
		}

		/// assignment operator
		ContentBuffer& operator=(const ContentBuffer& buf) {
			if (buf.size()) {
				resize(buf.size());
				memcpy(get(), buf.get(), buf.size());
			} else {
				clear();
			}
			return *this;
		}
		
		/// returns true if buffer is empty
		inline bool is_empty() const { return m_len == 0; }
		
		/// returns size in bytes
		inline std::size_t size() const { return m_len; }
		
		/// returns const pointer to data
		inline const char *get() const { return m_ptr; }
		
		/// returns mutable pointer to data
		inline char *get() { return m_ptr; }
		
		/// changes the size of the content buffer
		inline void resize(std::size_t len) {
			m_len = len;
			if (len == 0) {
				m_buf.reset();
				m_ptr = &m_empty;
			} else {
				m_buf.reset(new char[len+1]);
				m_buf[len] = '\0';
				m_ptr = m_buf.get();
			}
		}
		
		/// clears the content buffer
		inline void clear() { resize(0); }
		
	private:
		boost::scoped_array<char>	m_buf;
		std::size_t					m_len;
		char						m_empty;
		char						*m_ptr;
	};

	/**
	 * prepares HTTP headers for a send operation
	 *
	 * @param keep_alive true if the connection should be kept alive
	 * @param using_chunks true if the payload content will be sent in chunks
	 */
	inline void prepareHeadersForSend(const bool keep_alive,
									  const bool using_chunks)
	{
		changeHeader(HEADER_CONNECTION, (keep_alive ? "Keep-Alive" : "close") );
		if (using_chunks) {
			if (getChunksSupported())
				changeHeader(HEADER_TRANSFER_ENCODING, "chunked");
		} else if (! m_do_not_send_content_length) {
			changeHeader(HEADER_CONTENT_LENGTH, boost::lexical_cast<std::string>(getContentLength()));
		}
	}

	/**
	 * appends the message's HTTP headers to a vector of write buffers
	 *
	 * @param write_buffers the buffers to append HTTP headers into
	 */
	inline void appendHeaders(WriteBuffers& write_buffers) {
		// add HTTP headers
		for (Headers::const_iterator i = m_headers.begin(); i != m_headers.end(); ++i) {
			write_buffers.push_back(boost::asio::buffer(i->first));
			write_buffers.push_back(boost::asio::buffer(HEADER_NAME_VALUE_DELIMITER));
			write_buffers.push_back(boost::asio::buffer(i->second));
			write_buffers.push_back(boost::asio::buffer(STRING_CRLF));
		}
		// add an extra CRLF to end HTTP headers
		write_buffers.push_back(boost::asio::buffer(STRING_CRLF));
	}

	/**
	 * Returns the first value in a dictionary if key is found; or an empty
	 * string if no values are found
	 *
	 * @param dict the dictionary to search for key
	 * @param key the key to search for
	 * @return value if found; empty string if not
	 */
	template <typename DictionaryType>
	inline static const std::string& getValue(const DictionaryType& dict,
											  const std::string& key)
	{
		typename DictionaryType::const_iterator i = dict.find(key);
		return ( (i==dict.end()) ? STRING_EMPTY : i->second );
	}

	/**
	 * Changes the value for a dictionary key.  Adds the key if it does not
	 * already exist.  If multiple values exist for the key, they will be
	 * removed and only the new value will remain.
	 *
	 * @param dict the dictionary object to update
	 * @param key the key to change the value for
	 * @param value the value to assign to the key
	 */
	template <typename DictionaryType>
	inline static void changeValue(DictionaryType& dict,
								   const std::string& key, const std::string& value)

	{
		// retrieve all current values for key
		std::pair<typename DictionaryType::iterator, typename DictionaryType::iterator>
			result_pair = dict.equal_range(key);
		if (result_pair.first == dict.end()) {
			// no values exist -> add a new key
			dict.insert(std::make_pair(key, value));
		} else {
			// set the first value found for the key to the new one
			result_pair.first->second = value;
			// remove any remaining values
			typename DictionaryType::iterator i;
			++(result_pair.first);
			while (result_pair.first != result_pair.second) {
				i = result_pair.first;
				++(result_pair.first);
				dict.erase(i);
			}
		}
	}

	/**
	 * Deletes all values for a key
	 *
	 * @param dict the dictionary object to update
	 * @param key the key to delete
	 */
	template <typename DictionaryType>
	inline static void deleteValue(DictionaryType& dict,
								   const std::string& key)
	{
		std::pair<typename DictionaryType::iterator, typename DictionaryType::iterator>
			result_pair = dict.equal_range(key);
		if (result_pair.first != dict.end())
			dict.erase(result_pair.first, result_pair.second);
	}

	/// erases the string containing the first line for the HTTP message
	/// (it will be updated the next time getFirstLine() is called)
	inline void clearFirstLine(void) const {
		if (! m_first_line.empty())
			m_first_line.clear();
	}

	/// updates the string containing the first line for the HTTP message
	virtual void updateFirstLine(void) const = 0;


	/// first line sent in an HTTP message
	/// (i.e. "GET / HTTP/1.1" for request, or "HTTP/1.1 200 OK" for response)
	mutable std::string				m_first_line;


private:

	/// Regex used to check for the "chunked" transfer encoding header
	static const boost::regex		REGEX_ICASE_CHUNKED;

	/// True if the HTTP message is valid
	bool							m_is_valid;

	/// whether the message body is chunked
	bool							m_is_chunked;

	/// true if chunked transfer encodings are supported
	bool							m_chunks_supported;

	/// if true, the content length will not be sent in the HTTP headers
	bool							m_do_not_send_content_length;

	/// IP address of the remote endpoint
	boost::asio::ip::address		m_remote_ip;

	/// HTTP major version number
	boost::uint16_t					m_version_major;

	/// HTTP major version number
	boost::uint16_t					m_version_minor;

	/// the length of the payload content (in bytes)
	boost::uint64_t					m_content_length;

	/// the payload content, if any was sent with the message
	ContentBuffer					m_content_buf;

	/// buffers for holding chunked data
	ChunkCache						m_chunk_cache;

	/// HTTP message headers
	Headers							m_headers;

	/// HTTP cookie parameters parsed from the headers
	CookieParams					m_cookie_params;

	/// message data integrity status
	DataStatus						m_status;

	/// missing packet indicator
	bool							m_has_missing_packets;

	/// indicates missing packets in the middle of the data stream
	bool							m_has_data_after_missing;
};


}	// end namespace net
}	// end namespace pion

#endif
