// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTPWRITER_HEADER__
#define __PION_HTTPWRITER_HEADER__

#include <vector>
#include <string>
#include <exception>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <pion/PionConfig.hpp>
#include <pion/PionLogger.hpp>
#include <pion/net/HTTPMessage.hpp>
#include <pion/net/TCPConnection.hpp>


namespace pion {	// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)

///
/// HTTPWriter: used to asynchronously send HTTP messages
/// 
class PION_NET_API HTTPWriter :
	private boost::noncopyable
{
protected:
	
	/// function called after the HTTP message has been sent
	typedef boost::function0<void>	FinishedHandler;

	/// data type for a function that handles write operations
	typedef boost::function2<void,const boost::system::error_code&,std::size_t>	WriteHandler;
	
	
	/**
	 * protected constructor: only derived classes may create objects
	 * 
	 * @param tcp_conn TCP connection used to send the message
	 * @param handler function called after the request has been sent
	 */
	HTTPWriter(TCPConnectionPtr& tcp_conn, FinishedHandler handler)
		: m_logger(PION_GET_LOGGER("pion.net.HTTPWriter")),
		m_tcp_conn(tcp_conn), m_content_length(0), m_stream_is_empty(true), 
		m_client_supports_chunks(true), m_sending_chunks(false),
		m_sent_headers(false), m_finished(handler)
	{}
	
	/**
	 * called after the message is sent
	 * 
	 * @param write_error error status from the last write operation
	 * @param bytes_written number of bytes sent by the last write operation
	 */
	virtual void handleWrite(const boost::system::error_code& write_error,
					 std::size_t bytes_written) = 0;

	
	/**
	 * initializes a vector of write buffers with the HTTP message information
	 *
	 * @param write_buffers vector of write buffers to initialize
	 */
	virtual void prepareBuffersForSend(HTTPMessage::WriteBuffers& write_buffers) = 0;
									  
	/// returns a function bound to HTTPWriter::handleWrite()
	virtual WriteHandler bindToWriteHandler(void) = 0;
	
	/// called after we have finished sending the HTTP message
	inline void finishedWriting(void) { if (m_finished) m_finished(); }
	
	
public:

	/// exception thrown if the TCP connection is dropped while/before sending
	class LostConnectionException : public std::exception {
	public:
		virtual const char* what() const throw() {
			return "Lost TCP connection while or before sending an HTTP message";
		}
	};
		
	/// default destructor
	virtual ~HTTPWriter() {}

	/// clears out all of the memory buffers used to cache payload content data
	inline void clear(void) {
		m_content_buffers.clear();
		m_binary_cache.clear();
		m_text_cache.clear();
		m_content_stream.str("");
		m_stream_is_empty = true;
		m_content_length = 0;
	}

	/**
	 * write text (non-binary) payload content
	 *
	 * @param data the data to append to the payload content
	 */
	template <typename T>
	inline void write(const T& data) {
		m_content_stream << data;
		if (m_stream_is_empty) m_stream_is_empty = false;
	}

	/**
	 * write binary payload content
	 *
	 * @param data points to the binary data to append to the payload content
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
	 * write text (non-binary) payload content; the data written is not
	 * copied, and therefore must persist until the message has finished
	 * sending
	 *
	 * @param data the data to append to the payload content
	 */
	inline void writeNoCopy(const std::string& data) {
		if (! data.empty()) {
			flushContentStream();
			m_content_buffers.push_back(boost::asio::buffer(data));
			m_content_length += data.size();
		}
	}
	
	/**
	 * write binary payload content;  the data written is not copied, and
	 * therefore must persist until the message has finished sending
	 *
	 * @param data points to the binary data to append to the payload content
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
	 * Sends all data buffered as a single HTTP message (without chunking).
	 * Following a call to this function, it is not thread safe to use your
	 * reference to the HTTPWriter object.
	 */
	inline void send(void) {
		sendMoreData(false, bindToWriteHandler());
	}
	
	/**
	 * Sends all data buffered as a single HTTP message (without chunking).
	 * Following a call to this function, it is not thread safe to use your
	 * reference to the HTTPWriter object until the send_handler has been called.
	 *
	 * @param send_handler function that is called after the message has been
	 *                     sent to the client.  Your callback function must end
	 *                     the connection by calling TCPConnection::finish().
	 */
	template <typename SendHandler>
	inline void send(SendHandler send_handler) {
		sendMoreData(false, send_handler);
	}
	
	/**
	 * Sends all data buffered as a single HTTP chunk.  Following a call to this
	 * function, it is not thread safe to use your reference to the HTTPWriter
	 * object until the send_handler has been called.
	 * 
	 * @param send_handler function that is called after the chunk has been sent
	 *                     to the client.  Your callback function must end by
	 *                     calling one of sendChunk() or sendFinalChunk().  Also,
	 *                     be sure to clear() the writer before writing data to it.
	 */
	template <typename SendHandler>
	inline void sendChunk(SendHandler send_handler) {
		m_sending_chunks = true;
		if (!supportsChunkedMessages()) {
			// sending data in chunks, but the client does not support chunking;
			// make sure that the connection will be closed when we are all done
			m_tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_CLOSE);
		}
		// send more data
		sendMoreData(false, send_handler);
	}

	/**
	 * Sends all data buffered (if any) and also sends the final HTTP chunk.
	 * This function (either overloaded version) must be called following any 
	 * calls to sendChunk().
	 * Following a call to this function, it is not thread safe to use your
	 * reference to the HTTPWriter object until the send_handler has been called.
	 *
	 * @param send_handler function that is called after the message has been
	 *                     sent to the client.  Your callback function must end
	 *                     the connection by calling TCPConnection::finish().
	 */ 
	template <typename SendHandler>
	inline void sendFinalChunk(SendHandler send_handler) {
		m_sending_chunks = true;
		sendMoreData(true, send_handler);
	}
	
	/**
	 * Sends all data buffered (if any) and also sends the final HTTP chunk.
	 * This function (either overloaded version) must be called following any 
	 * calls to sendChunk().
	 * Following a call to this function, it is not thread safe to use your
	 * reference to the HTTPWriter object.
	 */ 
	inline void sendFinalChunk(void) {
		m_sending_chunks = true;
		sendMoreData(true, bindToWriteHandler());
	}
	
	
	/// returns a shared pointer to the TCP connection
	inline TCPConnectionPtr& getTCPConnection(void) { return m_tcp_conn; }

	/// returns the length of the payload content (in bytes)
	inline size_t getContentLength(void) const { return m_content_length; }

	/// sets whether or not the client supports chunked messages
	inline void supportsChunkedMessages(bool b) { m_client_supports_chunks = b; }
	
	/// returns true if the client supports chunked messages
	inline bool supportsChunkedMessages() const { return m_client_supports_chunks; }

	/// returns true if we are sending a chunked message to the client
	inline bool sendingChunkedMessage() const { return m_sending_chunks; }
	
	/// sets the logger to be used
	inline void setLogger(PionLogger log_ptr) { m_logger = log_ptr; }
	
	/// returns the logger currently in use
	inline PionLogger getLogger(void) { return m_logger; }

	
private:

	/**
	 * sends all of the buffered data to the client
	 *
	 * @param send_final_chunk true if the final 0-byte chunk should be included
	 * @param send_handler function called after the data has been sent
	 */
	template <typename SendHandler>
	inline void sendMoreData(const bool send_final_chunk, SendHandler send_handler)
	{
		// make sure that we did not lose the TCP connection
		if (! m_tcp_conn->is_open()) throw LostConnectionException();
		// make sure that the content-length is up-to-date
		flushContentStream();
		// prepare the write buffers to be sent
		HTTPMessage::WriteBuffers write_buffers;
		prepareWriteBuffers(write_buffers, send_final_chunk);
		// send data in the write buffers
		m_tcp_conn->async_write(write_buffers, send_handler);
	}
	
	/**
	 * prepares write_buffers for next send operation
	 *
	 * @param write_buffers buffers to which data will be appended
	 * @param send_final_chunk true if the final 0-byte chunk should be included
	 */
	void prepareWriteBuffers(HTTPMessage::WriteBuffers &write_buffers,
							 const bool send_final_chunk);
	
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
	
	
	/// used to cache binary data included within the payload content
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
	
	/// used to cache text (non-binary) data included within the payload content
	typedef std::list<std::string>				TextCache;

	
	/// primary logging interface used by this class
	PionLogger								m_logger;

	/// The HTTP connection that we are writing the message to
	TCPConnectionPtr						m_tcp_conn;
	
	/// I/O write buffers that wrap the payload content to be written
	HTTPMessage::WriteBuffers				m_content_buffers;
	
	/// caches binary data included within the payload content
	BinaryCache								m_binary_cache;

	/// caches text (non-binary) data included within the payload content
	TextCache								m_text_cache;
	
	/// incrementally creates strings of text data for the TextCache
	std::ostringstream						m_content_stream;
	
	/// The length (in bytes) of the response content to be sent (Content-Length)
	size_t									m_content_length;

	/// true if the content_stream is empty (avoids unnecessary string copies)
	bool									m_stream_is_empty;
	
	/// true if the HTTP client supports chunked transfer encodings
	bool									m_client_supports_chunks;
	
	/// true if data is being sent to the client using multiple chunks
	bool									m_sending_chunks;
	
	/// true if the HTTP message headers have already been sent
	bool									m_sent_headers;

	/// function called after the HTTP message has been sent
	FinishedHandler							m_finished;
};


/// data type for a HTTPWriter pointer
typedef boost::shared_ptr<HTTPWriter>	HTTPWriterPtr;


/// override operator<< for convenience
template <typename T>
HTTPWriterPtr& operator<<(HTTPWriterPtr& writer, const T& data) {
	writer->write(data);
	return writer;
}


}	// end namespace net
}	// end namespace pion

#endif
