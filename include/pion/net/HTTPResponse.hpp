// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
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
#include <boost/bind.hpp>
#include <pion/PionConfig.hpp>
#include <pion/PionLogger.hpp>
#include <pion/net/TCPConnection.hpp>
#include <pion/net/HTTPTypes.hpp>
#include <pion/net/HTTPRequest.hpp>
#include <vector>
#include <string>
#include <exception>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)

///
/// HTTPResponse: container for HTTP response information
/// 
class PION_NET_API HTTPResponse :
	public boost::enable_shared_from_this<HTTPResponse>,
	private boost::noncopyable
{
	
protected:
	
	/// data type for I/O write buffers (these wrap existing data to be sent)
	typedef std::vector<boost::asio::const_buffer>	WriteBuffers;
	
	
	/**
	 * protected constructor restricts creation of objects (use create())
     * 
     * @param http_request the request to which we are responding
	 * @param tcp_conn TCP connection used to send the response
	 */
	HTTPResponse(HTTPRequestPtr& http_request, TCPConnectionPtr& tcp_conn)
		: m_logger(PION_GET_LOGGER("pion.net.HTTPResponse")),
		m_tcp_conn(tcp_conn),
		m_stream_is_empty(true), 
		m_response_message(HTTPTypes::RESPONSE_MESSAGE_OK),
		m_content_type(HTTPTypes::CONTENT_TYPE_HTML),
		m_content_length(0),
		m_client_supports_chunks(http_request->getVersionMajor() == 1
								 && http_request->getVersionMinor() >= 1),
		m_sending_chunks(false), m_sent_headers(false)
	{
		// set default response code of "200 OK"
		setResponseCode(HTTPTypes::RESPONSE_CODE_OK);
	}

	/**
	 * sends all of the buffered data to the client
	 *
	 * @param send_final_chunk true if the final 0-byte chunk should be included
	 * @param send_handler function called after the data has been sent
	 */
	template <typename SendHandler>
	inline void sendMoreData(const bool send_final_chunk, SendHandler send_handler)
	{
		// make sure that we did not loose the TCP connection
		if (! m_tcp_conn->is_open()) throw LostConnectionException();
		// make sure that the content-length is up-to-date
		flushContentStream();
		// prepare the write buffers to be sent
		WriteBuffers write_buffers;
		prepareWriteBuffers(write_buffers, send_final_chunk);
		// send data in the write buffers
		m_tcp_conn->async_write(write_buffers, send_handler);
	}
	

public:

	/// exception thrown if the TCP connection is dropped while/before sending
	class LostConnectionException : public std::exception {
	public:
		virtual const char* what() const throw() {
			return "Lost TCP connection while or before sending an HTTP response";
		}
	};
	

	/**
     * creates new HTTPResponse objects
     * 
     * @param http_request the request to which we are responding
	 * @param tcp_conn TCP connection used to send the response
	 * 
     * @return boost::shared_ptr<HTTPResponse> shared pointer to the
     *         new response object that was created
	 */
	static inline boost::shared_ptr<HTTPResponse> create(HTTPRequestPtr& http_request,
														 TCPConnectionPtr& tcp_conn)
	{
		return boost::shared_ptr<HTTPResponse>(new HTTPResponse(http_request, tcp_conn));
	}
	
	/// default destructor
	virtual ~HTTPResponse() {}

	/// clears out all of the memory buffers used to cache response data
	inline void clear(void) {
		m_content_buffers.clear();
		m_binary_cache.clear();
		m_text_cache.clear();
		m_content_stream.str("");
		m_stream_is_empty = true;
		m_content_length = 0;
	}

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

	
	/**
	 * Sends all data buffered as a single HTTP response (without chunking).
	 * Following a call to this function, it is not thread safe to use your
	 * reference to the HTTPResponse object.
	 */
	inline void send(void) {
		sendMoreData(false, boost::bind(&HTTPResponse::handleWrite,
										shared_from_this(),
										boost::asio::placeholders::error,
										boost::asio::placeholders::bytes_transferred) );
	}
	
	/**
	 * Sends all data buffered as a single HTTP response (without chunking).
	 * Following a call to this function, it is not thread safe to use your
	 * reference to the HTTPResponse object until the send_handler has been called.
	 *
	 * @param send_handler function that is called after the response has been
	 *                     sent to the client.  Your callback function must end
	 *                     the connection by calling TCPConnection::finish().
	 */
	template <typename SendHandler>
	inline void send(SendHandler send_handler) {
		sendMoreData(false, send_handler);
	}
	
	/**
	 * Sends all data buffered as a single HTTP chunk.  Following a call to this
	 * function, it is not thread safe to use your reference to the HTTPResponse
	 * object until the send_handler has been called.
	 * 
	 * @param send_handler function that is called after the chunk has been sent
	 *                     to the client.  Your callback function must end by
	 *                     calling one of sendChunk() or sendFinalChunk().  Also,
	 *                     be sure to clear() the response before writing to it.
	 */
	template <typename SendHandler>
	inline void sendChunk(SendHandler send_handler) {
		m_sending_chunks = true;
		if (!supportsChunkedResponses()) {
			// sending data in chunks, but the client does not support chunking;
			// make sure that the connection will be closed when we are all done
			m_tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_CLOSE);
		}
		// send more data
		sendMoreData(false, send_handler);
	}
	
	/**
	 * Sends all data buffered (if any) and also sends the final HTTP chunk.
	 * This function must be called following any calls to sendChunk().
	 * Following a call to this function, it is not thread safe to use your
	 * reference to the HTTPResponse object until the send_handler has been called.
	 *
	 * @param send_handler function that is called after the response has been
	 *                     sent to the client.  Your callback function must end
	 *                     the connection by calling TCPConnection::finish().
	 */ 
	template <typename SendHandler>
	inline void sendFinalChunk(SendHandler send_handler) {
		sendMoreData(true, send_handler);
	}
	
	/**
	 * Sends all data buffered (if any) and also sends the final HTTP chunk.
	 * This function must be called following any calls to sendChunk().
	 * Following a call to this function, it is not thread safe to use your
	 * reference to the HTTPResponse object.
	 */ 
	inline void sendFinalChunk(void) {
		sendMoreData(true, boost::bind(&HTTPResponse::handleWrite,
									   shared_from_this(),
									   boost::asio::placeholders::error,
									   boost::asio::placeholders::bytes_transferred) );
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
	
	/// returns a shared pointer to the response's TCP connection
	inline TCPConnectionPtr& getTCPConnection(void) { return m_tcp_conn; }

	/// returns true if the client supports chunked responses
	inline bool supportsChunkedResponses() const { return m_client_supports_chunks; }

	/// returns true if we are sending a chunked response to the client
	inline bool sendingChunkedResponse() const { return m_sending_chunks; }
	
	
private:
	
	/**
	 * prepares write_buffers for next send operation
	 *
	 * @param write_buffers buffers to which data will be appended
	 * @param send_final_chunk true if the final 0-byte chunk should be included
	 */
	void prepareWriteBuffers(WriteBuffers &write_buffers,
							 const bool send_final_chunk);
	
	/**
	 * called after the response is sent
	 * 
	 * @param write_error error status from the last write operation
	 * @param bytes_written number of bytes sent by the last write operation
	 */
	void handleWrite(const boost::system::error_code& write_error,
					 std::size_t bytes_written);

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

	
	/// primary logging interface used by this class
	PionLogger								m_logger;

	/// The HTTP connection that we are writing the response to
	TCPConnectionPtr						m_tcp_conn;
	
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

	/// true if the HTTP client supports chunked transfer encodings
	bool									m_client_supports_chunks;
	
	/// true if data is being sent to the client using multiple chunks
	bool									m_sending_chunks;
	
	/// true if the HTTP response headers have already been sent
	bool									m_sent_headers;
};


/// data type for a HTTPResponse pointer
typedef boost::shared_ptr<HTTPResponse>		HTTPResponsePtr;


/// override operator<< for convenience
template <typename T>
HTTPResponsePtr& operator<<(HTTPResponsePtr& response, const T& data) {
	response->write(data);
	return response;
}


}	// end namespace net
}	// end namespace pion

#endif
