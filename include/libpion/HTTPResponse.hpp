// -----------------------------------------------------------------
// libpion: a C++ framework for building lightweight HTTP interfaces
// -----------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTPRESPONSE_HEADER__
#define __PION_HTTPRESPONSE_HEADER__

#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <libpion/PionConfig.hpp>
#include <libpion/PionLogger.hpp>
#include <libpion/TCPConnection.hpp>
#include <libpion/HTTPTypes.hpp>
#include <vector>
#include <string>


namespace pion {	// begin namespace pion

///
/// HTTPResponse: container for HTTP response information
/// 
class PION_LIBRARY_API HTTPResponse :
	public boost::enable_shared_from_this<HTTPResponse>,
	private boost::noncopyable
{
	
public:

	/// creates new HTTPResponse objects
	static inline boost::shared_ptr<HTTPResponse> create(void) {
		return boost::shared_ptr<HTTPResponse>(new HTTPResponse);
	}
	
	/// default destructor
	virtual ~HTTPResponse() {}

	/**
	 * write text (non-binary) response content
	 *
	 * @param data the data to append to the response content
	 */
	template <typename T>
	inline void write(const T& data) {
		m_content_stream << data;
		if (m_stream_is_empty) m_stream_is_empty = false;
	}

	/**
	 * write binary response content
	 *
	 * @param data points to the binary data to append to the response content
	 * @param length the length, in bytes, of the binary data
	 */
	inline void write(const void *data, size_t length) {
		if (length != 0) {
			flushContentStream();
			m_content_buffers.push_back(m_binary_cache.add(data, length));
			m_content_length += length;
		}
	}
	

	/**
	 * write text (non-binary) response content; the data written is not
	 * copied, and therefore must persist until the response has finished
	 * sending; this specialization avoids lexical_cast for strings
	 *
	 * @param data the data to append to the response content
	 */
	inline void writeNoCopy(const std::string& data) {
		if (! data.empty()) {
			flushContentStream();
			m_content_buffers.push_back(boost::asio::buffer(data));
			m_content_length += data.size();
		}
	}
	
	/**
	 * write binary response content;  the data written is not copied, and
	 * therefore must persist until the response has finished sending
	 *
	 * @param data points to the binary data to append to the response content
	 * @param length the length, in bytes, of the binary data
	 */
	inline void writeNoCopy(void *data, size_t length) {
		if (length > 0) {
			flushContentStream();
			m_content_buffers.push_back(boost::asio::buffer(data, length));
			m_content_length += length;
		}
	}

	/// adds an HTTP response header
	inline void addHeader(const std::string& key, const std::string& value) {
		m_response_headers.insert(std::make_pair(key, value));
	}

	/**
	 * sets a cookie by adding a Set-Cookie header (see RFC 2109)
	 * the cookie will be discarded by the user-agent when it closes
	 *
	 * @param name the name of the cookie
	 * @param value the value of the cookie
	 */
	inline void setCookie(const std::string& name, const std::string& value) {
		std::string set_cookie_header(makeSetCookieHeader(name, value, ""));
		addHeader(HTTPTypes::HEADER_SET_COOKIE, set_cookie_header);
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
		std::string set_cookie_header(makeSetCookieHeader(name, value, path));
		addHeader(HTTPTypes::HEADER_SET_COOKIE, set_cookie_header);
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
		std::string set_cookie_header(makeSetCookieHeader(name, value, path, true, max_age));
		addHeader(HTTPTypes::HEADER_SET_COOKIE, set_cookie_header);
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
		std::string set_cookie_header(makeSetCookieHeader(name, value, "", true, max_age));
		addHeader(HTTPTypes::HEADER_SET_COOKIE, set_cookie_header);
	}
	
	/// deletes cookie called name by adding a Set-Cookie header (cookie has no path)
	inline void deleteCookie(const std::string& name) {
		std::string set_cookie_header(makeSetCookieHeader(name, "", "", true, 0));
		addHeader(HTTPTypes::HEADER_SET_COOKIE, set_cookie_header);
	}
	
	/// deletes cookie called name by adding a Set-Cookie header (cookie has a path)
	inline void deleteCookie(const std::string& name, const std::string& path) {
		std::string set_cookie_header(makeSetCookieHeader(name, "", path, true, 0));
		addHeader(HTTPTypes::HEADER_SET_COOKIE, set_cookie_header);
	}

	/// sets the response or status code to send
	inline void setResponseCode(const unsigned int n) {
		// add space character to beginning and end
		m_response_code = ' ';
		m_response_code += boost::lexical_cast<std::string>(n);
		m_response_code += ' ';
	}
	
	/// sets the time that the response was last modified (Last-Modified)
	inline void setLastModified(const unsigned long t) {
		addHeader(HTTPTypes::HEADER_LAST_MODIFIED, HTTPTypes::get_date_string(t));
	}
	
	/// sets the response or status message to send
	inline void setResponseMessage(const std::string& m) { m_response_message = m; }

	/// sets the type of response content to be sent (Content-Type)
	inline void setContentType(const std::string& t) { m_content_type = t; }

	/// sets the logger to be used
	inline void setLogger(PionLogger log_ptr) { m_logger = log_ptr; }
	
	/// returns the logger currently in use
	inline PionLogger getLogger(void) { return m_logger; }

	
	/**
	 * sends the response
	 *
	 * @param tcp_conn TCP connection used to send the response
	 */
	void send(TCPConnectionPtr& tcp_conn);

	
protected:
		
	/// protected constructor restricts creation of objects (use create())
	HTTPResponse(void)
		: m_logger(PION_GET_LOGGER("Pion.HTTPResponse")),
		m_stream_is_empty(true), 
		m_response_message(HTTPTypes::RESPONSE_MESSAGE_OK),
		m_content_type(HTTPTypes::CONTENT_TYPE_HTML),
		m_content_length(0)
	{
			setResponseCode(HTTPTypes::RESPONSE_CODE_OK);
	}

	/**
	 * creates a "Set-Cookie" header
	 *
	 * @param name the name of the cookie
	 * @param value the value of the cookie
	 * @param path the path of the cookie
	 * @param has_max_age true if the max_age value should be set
	 * @param max_age the life of the cookie, in seconds (0 = discard)
	 *
	 * @return the new "Set-Cookie" header
	 */
	 std::string makeSetCookieHeader(const std::string& name, const std::string& value,
									 const std::string& path, const bool has_max_age = false,
									 const unsigned long max_age = 0);
	
private:
	
	/**
	 * called after the response is sent
	 * 
	 * @param tcp_conn TCP connection used to send the response
	 * @param write_error error status from the last write operation
	 * @param bytes_written number of bytes sent by the last write operation
	 */
	void handleWrite(TCPConnectionPtr tcp_conn,
					 const boost::system::error_code& write_error,
					 std::size_t bytes_written);

	/// flushes any text data in the content stream after caching it in the TextCache
	inline void flushContentStream(void) {
		if (! m_stream_is_empty) {
			std::string string_to_add(m_content_stream.str());
			if (! string_to_add.empty()) {
				m_content_stream.str("");
				m_content_length += string_to_add.size();
				m_text_cache.push_back(string_to_add);
				m_content_buffers.push_back(boost::asio::buffer(m_text_cache.back()));
			}
			m_stream_is_empty = true;
		}
	}
	
	/// used to cache binary data included within the response
	class BinaryCache : public std::vector<std::pair<const char *, size_t> > {
	public:
		~BinaryCache() {
			for (iterator i=begin(); i!=end(); ++i) {
				delete[] i->first;
			}
		}
		inline boost::asio::const_buffer add(const void *ptr, const size_t size) {
			char *data_ptr = new char[size];
			memcpy(data_ptr, ptr, size);
			push_back( std::make_pair(data_ptr, size) );
			return boost::asio::buffer(data_ptr, size);
		}
	};
	
	/// used to cache text (non-binary) data included within the response
	typedef std::vector<std::string>				TextCache;

	/// data type for I/O write buffers (these wrap existing data to be sent)
	typedef std::vector<boost::asio::const_buffer>	WriteBuffers;
	
	
	/// primary logging interface used by this class
	PionLogger								m_logger;

	/// I/O write buffers that wrap the response content to be written
	WriteBuffers							m_content_buffers;
	
	/// caches binary data included within the response
	BinaryCache								m_binary_cache;

	/// caches text (non-binary) data included within the response
	TextCache								m_text_cache;
	
	/// incrementally creates strings of text data for the TextCache
	std::ostringstream						m_content_stream;
	
	/// true if the content_stream is empty (avoids unnecessary string copies)
	bool									m_stream_is_empty;
	
	/// The HTTP response headers to send
	HTTPTypes::StringDictionary				m_response_headers;
	
	/// The HTTP response or status code to send (as a string wrapped with spaces)
	std::string								m_response_code;

	/// The HTTP response or status message to send
	std::string								m_response_message;
	
	/// The type of response content to be sent (Content-Type)
	std::string								m_content_type;
	
	/// The length (in bytes) of the response content to be sent (Content-Length)
	size_t									m_content_length;
};


/// data type for a HTTPResponse pointer
typedef boost::shared_ptr<HTTPResponse>		HTTPResponsePtr;


/// override operator<< for convenience
template <typename T>
HTTPResponsePtr& operator<<(HTTPResponsePtr& response, const T& data) {
	response->write(data);
	return response;
}


}	// end namespace pion

#endif
