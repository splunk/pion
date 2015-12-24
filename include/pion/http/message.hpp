// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HTTP_MESSAGE_HEADER__
#define __PION_HTTP_MESSAGE_HEADER__

#include <iosfwd>
#include <vector>
#include <cstring>
#include <cstdint>
#include <regex>
#include <asio.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <pion/config.hpp>
#include <pion/http/types.hpp>
#include <boost/system/error_code.hpp>

#ifndef BOOST_SYSTEM_NOEXCEPT
    // if 'BOOST_NOEXCEPT' is not defined, as with some older versions of
    // boost, setting it to nothing should be harmless.
    #ifndef BOOST_NOEXCEPT
        #define BOOST_SYSTEM_NOEXCEPT
    #else
        #define BOOST_SYSTEM_NOEXCEPT BOOST_NOEXCEPT
    #endif
#endif


namespace pion {    // begin namespace pion
	
	
namespace tcp {
    // forward declaration for class used by send() and receive()
    class connection;
}


namespace http {    // begin namespace http


// forward declaration of parser class
class parser;

    
///
/// message: base container for HTTP messages
/// 
class PION_API message
    : public http::types
{
public:

    /// data type for I/O write buffers (these wrap existing data to be sent)
    typedef std::vector<asio::const_buffer>  write_buffers_t;

    /// used to cache chunked data
    typedef std::vector<char>   chunk_cache_t;

    /// data type for library errors returned during receive() operations
    struct receive_error_t
        : public boost::system::error_category
    {
        virtual ~receive_error_t() {}
        virtual inline const char *name() const BOOST_SYSTEM_NOEXCEPT { return "receive_error_t"; }
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
    enum data_status_t
    {
        STATUS_NONE,        // no data received (i.e. all lost)
        STATUS_TRUNCATED,   // one or more missing packets at the end 
        STATUS_PARTIAL,     // one or more missing packets but NOT at the end 
        STATUS_OK           // no missing packets
    };

    /// constructs a new HTTP message object
    message(void)
        : m_is_valid(false), m_is_chunked(false), m_chunks_supported(false),
        m_do_not_send_content_length(false),
        m_version_major(1), m_version_minor(1), m_content_length(0), m_content_buf(),
        m_status(STATUS_NONE), m_has_missing_packets(false), m_has_data_after_missing(false)
    {}

    /// copy constructor
    message(const message& http_msg)
        : http::types(),
        m_first_line(http_msg.m_first_line),
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
        m_cookie_params(http_msg.m_cookie_params),
        m_status(http_msg.m_status),
        m_has_missing_packets(http_msg.m_has_missing_packets),
        m_has_data_after_missing(http_msg.m_has_data_after_missing)
    {}

    /// assignment operator
    inline message& operator=(const message& http_msg) {
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
        m_cookie_params = http_msg.m_cookie_params;
        m_status = http_msg.m_status;
        m_has_missing_packets = http_msg.m_has_missing_packets;
        m_has_data_after_missing = http_msg.m_has_data_after_missing;
        return *this;
    }

    /// virtual destructor
    virtual ~message() {}

    /// clears all message data
    virtual void clear(void) {
        clear_first_line();
        m_is_valid = m_is_chunked = m_chunks_supported
            = m_do_not_send_content_length = false;
        m_remote_ip = asio::ip::address_v4(0);
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
    virtual bool is_content_length_implied(void) const = 0;

    /// returns true if the message is valid
    inline bool is_valid(void) const { return m_is_valid; }

    /// returns true if chunked transfer encodings are supported
    inline bool get_chunks_supported(void) const { return m_chunks_supported; }

    /// returns IP address of the remote endpoint
    inline asio::ip::address& get_remote_ip(void) {
        return m_remote_ip;
    }

    /// returns the major HTTP version number
    inline std::uint16_t get_version_major(void) const { return m_version_major; }

    /// returns the minor HTTP version number
    inline std::uint16_t get_version_minor(void) const { return m_version_minor; }

    /// returns a string representation of the HTTP version (i.e. "HTTP/1.1")
    inline std::string get_version_string(void) const {
        std::string http_version(STRING_HTTP_VERSION);
        http_version += std::to_string(get_version_major());
        http_version += '.';
        http_version += std::to_string(get_version_minor());
        return http_version;
    }

    /// returns the length of the payload content (in bytes)
    inline size_t get_content_length(void) const { return m_content_length; }

    /// returns true if the message content is chunked
    inline bool is_chunked(void) const { return m_is_chunked; }

    /// returns true if buffer for content is allocated
    bool is_content_buffer_allocated() const { return !m_content_buf.is_empty(); }
    
    /// returns size of allocated buffer
    inline std::size_t get_content_buffer_size() const { return m_content_buf.size(); }
    
    /// returns a pointer to the payload content, or empty string if there is none
    inline char *get_content(void) { return m_content_buf.get(); }

    /// returns a const pointer to the payload content, or empty string if there is none
    inline const char *get_content(void) const { return m_content_buf.get(); }

    /// returns a reference to the chunk cache
    inline chunk_cache_t& get_chunk_cache(void) { return m_chunk_cache; }

    /// returns a value for the header if any are defined; otherwise, an empty string
    inline const std::string& get_header(const std::string& key) const {
        return get_value(m_headers, key);
    }

    /// returns a reference to the HTTP headers
    inline ihash_multimap& get_headers(void) {
        return m_headers;
    }

    /// returns true if at least one value for the header is defined
    inline bool has_header(const std::string& key) const {
        return(m_headers.find(key) != m_headers.end());
    }

    /// returns a value for the cookie if any are defined; otherwise, an empty string
    /// since cookie names are insensitive, key should use lowercase alpha chars
    inline const std::string& get_cookie(const std::string& key) const {
        return get_value(m_cookie_params, key);
    }
    
    /// returns the cookie parameters
    inline ihash_multimap& get_cookies(void) {
        return m_cookie_params;
    }

    /// returns true if at least one value for the cookie is defined
    /// since cookie names are insensitive, key should use lowercase alpha chars
    inline bool has_cookie(const std::string& key) const {
        return(m_cookie_params.find(key) != m_cookie_params.end());
    }
    
    /// adds a value for the cookie
    /// since cookie names are insensitive, key should use lowercase alpha chars
    inline void add_cookie(const std::string& key, const std::string& value) {
        m_cookie_params.insert(std::make_pair(key, value));
    }

    /// changes the value of a cookie
    /// since cookie names are insensitive, key should use lowercase alpha chars
    inline void change_cookie(const std::string& key, const std::string& value) {
        change_value(m_cookie_params, key, value);
    }

    /// removes all values for a cookie
    /// since cookie names are insensitive, key should use lowercase alpha chars
    inline void delete_cookie(const std::string& key) {
        delete_value(m_cookie_params, key);
    }
    
    /// returns a string containing the first line for the HTTP message
    inline const std::string& get_first_line(void) const {
        if (m_first_line.empty())
            update_first_line();
        return m_first_line;
    }

    /// true if there were missing packets 
    inline bool has_missing_packets() const { return m_has_missing_packets; }
    
    /// set to true when missing packets detected
    inline void set_missing_packets(bool newVal) { m_has_missing_packets = newVal; }

    /// true if more data seen after the missing packets
    inline bool has_data_after_missing_packets() const { return m_has_data_after_missing; }

    inline void set_data_after_missing_packet(bool newVal) { m_has_data_after_missing = newVal; }

    /// sets whether or not the message is valid
    inline void set_is_valid(bool b = true) { m_is_valid = b; }

    /// set to true if chunked transfer encodings are supported
    inline void set_chunks_supported(bool b) { m_chunks_supported = b; }

    /// sets IP address of the remote endpoint
    inline void set_remote_ip(const asio::ip::address& ip) { m_remote_ip = ip; }

    /// sets the major HTTP version number
    inline void set_version_major(const std::uint16_t n) {
        m_version_major = n;
        clear_first_line();
    }

    /// sets the minor HTTP version number
    inline void set_version_minor(const std::uint16_t n) {
        m_version_minor = n;
        clear_first_line();
    }

    /// sets the length of the payload content (in bytes)
    inline void set_content_length(size_t n) { m_content_length = n; }

    /// if called, the content-length will not be sent in the HTTP headers
    inline void set_do_not_send_content_length(void) { m_do_not_send_content_length = true; }

    /// return the data receival status
    inline data_status_t get_status() const { return m_status; }

    /// 
    inline void set_status(data_status_t newVal) { m_status = newVal; }

    /// sets the length of the payload content using the Content-Length header
    inline void update_content_length_using_header(void) {
        ihash_multimap::const_iterator i = m_headers.find(HEADER_CONTENT_LENGTH);
        if (i == m_headers.end()) {
            m_content_length = 0;
        } else {
            std::string trimmed_length(i->second);
            boost::algorithm::trim(trimmed_length);
            m_content_length = std::stoul(trimmed_length);
        }
    }

    /// sets the transfer coding using the Transfer-Encoding header
    inline void update_transfer_encoding_using_header(void) {
        m_is_chunked = false;
        ihash_multimap::const_iterator i = m_headers.find(HEADER_TRANSFER_ENCODING);
        if (i != m_headers.end()) {
            // From RFC 2616, sec 3.6: All transfer-coding values are case-insensitive.
            m_is_chunked = std::regex_match(i->second, REGEX_ICASE_CHUNKED);
            // ignoring other possible values for now
        }
    }

    ///creates a payload content buffer of size m_content_length and returns
    /// a pointer to the new buffer (memory is managed by message class)
    inline char *create_content_buffer(void) {
        m_content_buf.resize(m_content_length);
        return m_content_buf.get();
    }
    
    /// resets payload content to match the value of a string
    inline void set_content(const std::string& content) {
        set_content_length(content.size());
        create_content_buffer();
        memcpy(m_content_buf.get(), content.c_str(), content.size());
    }

    /// clears payload content buffer
    inline void clear_content(void) {
        set_content_length(0);
        create_content_buffer();
        delete_value(m_headers, HEADER_CONTENT_TYPE);
    }

    /// sets the content type for the message payload
    inline void set_content_type(const std::string& type) {
        change_value(m_headers, HEADER_CONTENT_TYPE, type);
    }

    /// adds a value for the HTTP header named key
    inline void add_header(const std::string& key, const std::string& value) {
        m_headers.insert(std::make_pair(key, value));
    }

    /// changes the value for the HTTP header named key
    inline void change_header(const std::string& key, const std::string& value) {
        change_value(m_headers, key, value);
    }

    /// removes all values for the HTTP header named key
    inline void delete_header(const std::string& key) {
        delete_value(m_headers, key);
    }

    /// returns true if the HTTP connection may be kept alive
    inline bool check_keep_alive(void) const {
        return (get_header(HEADER_CONNECTION) != "close"
                && (get_version_major() > 1
                    || (get_version_major() >= 1 && get_version_minor() >= 1)) );
    }

    /**
     * initializes a vector of write buffers with the HTTP message information
     *
     * @param write_buffers vector of write buffers to initialize
     * @param keep_alive true if the connection should be kept alive
     * @param using_chunks true if the payload content will be sent in chunks
     */
    inline void prepare_buffers_for_send(write_buffers_t& write_buffers,
                                      const bool keep_alive,
                                      const bool using_chunks)
    {
        // update message headers
        prepare_headers_for_send(keep_alive, using_chunks);
        // add first message line
        write_buffers.push_back(asio::buffer(get_first_line()));
        write_buffers.push_back(asio::buffer(STRING_CRLF));
        // append cookie headers (if any)
        append_cookie_headers();
        // append HTTP headers
        append_headers(write_buffers);
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
    std::size_t send(tcp::connection& tcp_conn,
                     asio::error_code& ec,
                     bool headers_only = false);

    /**
     * receives a new message from a TCP connection (blocks until finished)
     *
     * @param tcp_conn TCP connection to use
     * @param ec contains error code if the receive fails
     * @param http_parser http parser object to use
     *
     * @return std::size_t number of bytes read from the connection
     */
    std::size_t receive(tcp::connection& tcp_conn,
                        asio::error_code& ec,
                        parser& http_parser);
    
    /**
     * receives a new message from a TCP connection (blocks until finished)
     *
     * @param tcp_conn TCP connection to use
     * @param ec contains error code if the receive fails
     * @param headers_only if true then only HTTP headers are received
     * @param max_content_length maximum number of content bytes received
     *
     * @return std::size_t number of bytes read from the connection
     */
    std::size_t receive(tcp::connection& tcp_conn,
                        asio::error_code& ec,
                        bool headers_only = false,
                        std::size_t max_content_length = static_cast<size_t>(-1));

    /**
     * writes the message to a std::ostream (blocks until finished)
     *
     * @param out std::ostream to use
     * @param ec contains error code if the write fails
     * @param headers_only if true then only HTTP headers are written
     *
     * @return std::size_t number of bytes written to the connection
     */
    std::size_t write(std::ostream& out,
                      asio::error_code& ec,
                      bool headers_only = false);

    /**
     * reads a new message from a std::istream (blocks until finished)
     *
     * @param in std::istream to use
     * @param ec contains error code if the read fails
     * @param http_parser http parser object to use
     *
     * @return std::size_t number of bytes read from the connection
     */
    std::size_t read(std::istream& in,
                     asio::error_code& ec,
                     parser& http_parser);
    
    /**
     * reads a new message from a std::istream (blocks until finished)
     *
     * @param in std::istream to use
     * @param ec contains error code if the read fails
     * @param headers_only if true then only HTTP headers are read
     * @param max_content_length maximum number of content bytes received
     *
     * @return std::size_t number of bytes read from the connection
     */
    std::size_t read(std::istream& in,
                     asio::error_code& ec,
                     bool headers_only = false,
                     std::size_t max_content_length = static_cast<size_t>(-1));

    /**
     * pieces together all the received chunks
     */
    void concatenate_chunks(void);


protected:
    
    /// a simple helper class used to manage a fixed-size payload content buffer
    class content_buffer_t {
    public:
        /// simple destructor
        ~content_buffer_t() {}

        /// default constructor
        content_buffer_t() : m_buf(), m_len(0), m_empty(0), m_ptr(&m_empty) {}

        /// copy constructor
        content_buffer_t(const content_buffer_t& buf)
            : m_buf(), m_len(0), m_empty(0), m_ptr(&m_empty)
        {
            if (buf.size()) {
                resize(buf.size());
                memcpy(get(), buf.get(), buf.size());
            }
        }

        /// assignment operator
        content_buffer_t& operator=(const content_buffer_t& buf) {
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
        std::unique_ptr<char[]>     m_buf;
        std::size_t                 m_len;
        char                        m_empty;
        char                        *m_ptr;
    };

    /**
     * prepares HTTP headers for a send operation
     *
     * @param keep_alive true if the connection should be kept alive
     * @param using_chunks true if the payload content will be sent in chunks
     */
    inline void prepare_headers_for_send(const bool keep_alive,
                                      const bool using_chunks)
    {
        change_header(HEADER_CONNECTION, (keep_alive ? "Keep-Alive" : "close") );
        if (using_chunks) {
            if (get_chunks_supported())
                change_header(HEADER_TRANSFER_ENCODING, "chunked");
        } else if (! m_do_not_send_content_length) {
            change_header(HEADER_CONTENT_LENGTH, std::to_string(get_content_length()));
        }
    }

    /**
     * appends the message's HTTP headers to a vector of write buffers
     *
     * @param write_buffers the buffers to append HTTP headers into
     */
    inline void append_headers(write_buffers_t& write_buffers) {
        // add HTTP headers
        for (ihash_multimap::const_iterator i = m_headers.begin(); i != m_headers.end(); ++i) {
            write_buffers.push_back(asio::buffer(i->first));
            write_buffers.push_back(asio::buffer(HEADER_NAME_VALUE_DELIMITER));
            write_buffers.push_back(asio::buffer(i->second));
            write_buffers.push_back(asio::buffer(STRING_CRLF));
        }
        // add an extra CRLF to end HTTP headers
        write_buffers.push_back(asio::buffer(STRING_CRLF));
    }

    /// appends HTTP headers for any cookies defined by the http::message
    virtual void append_cookie_headers(void) {}

    /**
     * Returns the first value in a dictionary if key is found; or an empty
     * string if no values are found
     *
     * @param dict the dictionary to search for key
     * @param key the key to search for
     * @return value if found; empty string if not
     */
    template <typename DictionaryType>
    inline static const std::string& get_value(const DictionaryType& dict,
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
    inline static void change_value(DictionaryType& dict,
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
    inline static void delete_value(DictionaryType& dict,
                                   const std::string& key)
    {
        std::pair<typename DictionaryType::iterator, typename DictionaryType::iterator>
            result_pair = dict.equal_range(key);
        if (result_pair.first != dict.end())
            dict.erase(result_pair.first, result_pair.second);
    }

    /// erases the string containing the first line for the HTTP message
    /// (it will be updated the next time get_first_line() is called)
    inline void clear_first_line(void) const {
        if (! m_first_line.empty())
            m_first_line.clear();
    }

    /// updates the string containing the first line for the HTTP message
    virtual void update_first_line(void) const = 0;

    /// first line sent in an HTTP message
    /// (i.e. "GET / HTTP/1.1" for request, or "HTTP/1.1 200 OK" for response)
    mutable std::string             m_first_line;


private:

    /// Regex used to check for the "chunked" transfer encoding header
    static const std::regex       REGEX_ICASE_CHUNKED;

    /// True if the HTTP message is valid
    bool                            m_is_valid;

    /// whether the message body is chunked
    bool                            m_is_chunked;

    /// true if chunked transfer encodings are supported
    bool                            m_chunks_supported;

    /// if true, the content length will not be sent in the HTTP headers
    bool                            m_do_not_send_content_length;

    /// IP address of the remote endpoint
    asio::ip::address        m_remote_ip;

    /// HTTP major version number
    std::uint16_t                 m_version_major;

    /// HTTP major version number
    std::uint16_t                 m_version_minor;

    /// the length of the payload content (in bytes)
    size_t                          m_content_length;

    /// the payload content, if any was sent with the message
    content_buffer_t                m_content_buf;

    /// buffers for holding chunked data
    chunk_cache_t                   m_chunk_cache;

    /// HTTP message headers
    ihash_multimap                  m_headers;

    /// HTTP cookie parameters parsed from the headers
    ihash_multimap                  m_cookie_params;

    /// message data integrity status
    data_status_t                   m_status;

    /// missing packet indicator
    bool                            m_has_missing_packets;

    /// indicates missing packets in the middle of the data stream
    bool                            m_has_data_after_missing;
};


}   // end namespace http
}   // end namespace pion

#endif
