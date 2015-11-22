// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_TCP_STREAM_HEADER__
#define __PION_TCP_STREAM_HEADER__

#include <cstring>
#include <istream>
#include <streambuf>
#include <pion/config.hpp>
#include <pion/tcp/connection.hpp>
#include <pion/stdx/mutex.hpp>
#include <pion/stdx/condition_variable.hpp>
#include <pion/stdx/functional.hpp>


namespace pion {    // begin namespace pion
namespace tcp {     // begin namespace tcp

    
///
/// stream_buffer: std::basic_streambuf wrapper for TCP network connections.
///                  Based in part on section 13.13.3 of "The Standard C++ Library"
///                  by Nicolai M. Josuttis, published in 1999 by Addison-Wesley
/// 
class stream_buffer
    : public std::basic_streambuf<char, std::char_traits<char> >
{
public:
    
    // data type definitions required for iostream compatability
    typedef char                                char_type;
    typedef std::char_traits<char>::int_type    int_type;
    typedef std::char_traits<char>::off_type    off_type;
    typedef std::char_traits<char>::pos_type    pos_type;
    typedef std::char_traits<char>              traits_type;

    // some integer constants used within stream_buffer
    enum {
        PUT_BACK_MAX = 10,  //< number of bytes that can be put back into the read buffer
        WRITE_BUFFER_SIZE = 8192    //< size of the write buffer
    };
    
    
    /**
     * constructs a TCP stream buffer object for an existing TCP connection
     *
     * @param conn_ptr pointer to the TCP connection to use for reading & writing
     */
    explicit stream_buffer(const tcp::connection_ptr& conn_ptr)
        : m_conn_ptr(conn_ptr), m_bytes_transferred(0), m_read_buf(m_conn_ptr->get_read_buffer().c_array())
    {
        setup_buffers();
    }

    /**
     * constructs a TCP stream buffer object for a new TCP connection
     *
     * @param io_service asio service associated with the connection
     * @param ssl_flag if true then the connection will be encrypted using SSL 
     */
    explicit stream_buffer(stdx::asio::io_service& io_service,
                             const bool ssl_flag = false)
        : m_conn_ptr(new connection(io_service, ssl_flag)),
        m_read_buf(m_conn_ptr->get_read_buffer().c_array())
    {
        setup_buffers();
    }
    
    /**
     * constructs a TCP stream buffer object for a new SSL/TCP connection
     *
     * @param io_service asio service associated with the connection
     * @param ssl_context asio ssl context associated with the connection
     */
    stream_buffer(stdx::asio::io_service& io_service,
                    connection::ssl_context_type& ssl_context)
        : m_conn_ptr(new connection(io_service, ssl_context)),
        m_read_buf(m_conn_ptr->get_read_buffer().c_array())
    {
        setup_buffers();
    }
    
    /// virtual destructor flushes the write buffer
    virtual ~stream_buffer() { sync(); }

    /// returns a reference to the current TCP connection
    connection& get_connection(void) { return *m_conn_ptr; }

    /// returns a const reference to the current TCP connection
    const connection& get_connection(void) const { return *m_conn_ptr; }
    
    
protected:

    /// sets up the read and write buffers for input and output
    inline void setup_buffers(void) {
        // use the TCP connection's read buffer and allow for bytes to be put back
        setg(m_read_buf+PUT_BACK_MAX, m_read_buf+PUT_BACK_MAX, m_read_buf+PUT_BACK_MAX);
        // set write buffer size-1 so that we have an extra char avail for overflow
        setp(m_write_buf, m_write_buf+(WRITE_BUFFER_SIZE-1));
    }
    
    /**
     * writes data in the output buffer to the TCP connection
     *
     * @return int_type the number of bytes sent, or eof() if there was an error
     */
    inline int_type flush_output(void) {
        const std::streamsize bytes_to_send = std::streamsize(pptr() - pbase());
        int_type bytes_sent = 0;
        if (bytes_to_send > 0) {
            stdx::lock_guard<stdx::mutex> async_lock(m_async_mutex);
            m_bytes_transferred = 0;
            m_conn_ptr->async_write(stdx::asio::buffer(pbase(), bytes_to_send),
                                    stdx::bind(&stream_buffer::operation_finished, this,
                                                stdx::asio::placeholders::error,
                                                stdx::asio::placeholders::bytes_transferred));
            m_async_done.wait(async_lock);
            bytes_sent = m_bytes_transferred;
            pbump(-bytes_sent);
            if (m_async_error)
                bytes_sent = traits_type::eof();
        }
        return bytes_sent;
    }
    
    /**
     * this function is called when the read buffer has no more characters available
     *
     * @return int_type the next character available for reading, or eof() if there was an error
     */
    virtual int_type underflow(void) {
        // first check if we still have bytes available in the read buffer
        if (gptr() < egptr())
            return traits_type::to_int_type(*gptr());
        
        // calculate the number of bytes we will allow to be put back
        std::streamsize put_back_num = std::streamsize(gptr() - eback());
        if (put_back_num > PUT_BACK_MAX)
            put_back_num = PUT_BACK_MAX;
        
        // copy the last bytes read to the beginning of the buffer (for put back)
        if (put_back_num > 0)
            memmove(m_read_buf+(PUT_BACK_MAX-put_back_num), gptr()-put_back_num, put_back_num);
        
        // read data from the TCP connection
        // note that this has to be an ansynchronous call; otherwise, it cannot
        // be cancelled by other threads and will block forever (such as during shutdown)
        stdx::lock_guard<stdx::mutex> async_lock(m_async_mutex);
        m_bytes_transferred = 0;
        m_conn_ptr->async_read_some(stdx::asio::buffer(m_read_buf+PUT_BACK_MAX,
                                                        connection::READ_BUFFER_SIZE-PUT_BACK_MAX),
                                    stdx::bind(&stream_buffer::operation_finished, this,
                                                stdx::asio::placeholders::error,
                                                stdx::asio::placeholders::bytes_transferred));
        m_async_done.wait(async_lock);
        if (m_async_error)
            return traits_type::eof();
        
        // reset buffer pointers now that data is available
        setg(m_read_buf+(PUT_BACK_MAX-put_back_num),            //< beginning of putback bytes
             m_read_buf+PUT_BACK_MAX,                           //< read position
             m_read_buf+PUT_BACK_MAX+m_bytes_transferred);      //< end of buffer
        
        // return next character available
        return traits_type::to_int_type(*gptr());
    }

    /**
     * this function is called when the write buffer for the stream is full
     *
     * @param c character that has not been written yet, or eof() if we are flushing
     * @return int_type the last character written, or eof() if there was an error
     */
    virtual int_type overflow(int_type c) {
        if (! traits_type::eq_int_type(c, traits_type::eof())) {
            // character is not eof -> add it to the end of the write buffer
            // we can push this to the back of the write buffer because we set
            // the size of the write buffer to 1 less than the actual size using setp()
            *pptr() = c;
            pbump(1);
        }
        // flush data in the write buffer by sending it to the TCP connection
        return ((flush_output() == traits_type::eof())
                ? traits_type::eof() : traits_type::not_eof(c));
    }

    /**
     * writes a sequence of characters
     *
     * @param s pointer to a sequence of characters
     * @param n number of characters in the sequence to write
     *
     * @return std::streamsize number of character written
     */
    virtual std::streamsize xsputn(const char_type *s, std::streamsize n) {
        const std::streamsize bytes_available = std::streamsize(epptr() - pptr());
        std::streamsize bytes_sent = 0;
        if (bytes_available >= n) {
            // there is enough room in the buffer -> just put it in there
            memcpy(pptr(), s, n);
            pbump(n);
            bytes_sent = n;
        } else {
            // there is not enough room left in the buffer
            if (bytes_available > 0) {
                // fill up the buffer
                memcpy(pptr(), s, bytes_available);
                pbump(bytes_available);
            }
            // flush data in the write buffer by sending it to the TCP connection
            if (flush_output() == traits_type::eof()) 
                return 0;
            if ((n-bytes_available) >= (WRITE_BUFFER_SIZE-1)) {
                // the remaining data to send is larger than the buffer available
                // send it all now rather than buffering
                stdx::lock_guard<stdx::mutex> async_lock(m_async_mutex);
                m_bytes_transferred = 0;
                m_conn_ptr->async_write(stdx::asio::buffer(s+bytes_available,
                                                            n-bytes_available),
                                        stdx::bind(&stream_buffer::operation_finished, this,
                                                    stdx::asio::placeholders::error,
                                                    stdx::asio::placeholders::bytes_transferred));
                m_async_done.wait(async_lock);
                bytes_sent = bytes_available + m_bytes_transferred;
            } else {
                // the buffer is larger than the remaining data
                // put remaining data to the beginning of the output buffer
                memcpy(pbase(), s+bytes_available, n-bytes_available);
                pbump(n-bytes_available);
                bytes_sent = n;
            }
        }
        return bytes_sent;
    }
    
    /**
     * reads a sequence of characters
     *
     * @param s pointer to where the sequence of characters will be stored
     * @param n number of characters in the sequence to read
     *
     * @return std::streamsize number of character read
     */
    virtual std::streamsize xsgetn(char_type *s, std::streamsize n) {
        std::streamsize bytes_remaining = n;
        while (bytes_remaining > 0) {
            const std::streamsize bytes_available = std::streamsize(egptr() - gptr());
            const std::streamsize bytes_next_read = ((bytes_available >= bytes_remaining)
                                                   ? bytes_remaining : bytes_available);
            // copy available input data from buffer
            if (bytes_next_read > 0) {
                memcpy(s, gptr(), bytes_next_read);
                gbump(bytes_next_read);
                bytes_remaining -= bytes_next_read;
                s += bytes_next_read;
            }
            if (bytes_remaining > 0) {
                // call underflow() to read more data
                if (traits_type::eq_int_type(underflow(), traits_type::eof()))
                    break;
            }
        }
        return(n-bytes_remaining);
    }           
        
    /**
     * synchronize buffers with the TCP connection
     *
     * @return 0 if successful, -1 if there was an error
     */
    virtual int_type sync(void) {
        return ((flush_output() == traits_type::eof()) ? -1 : 0);
    }
    
    
private:
    
    /// function called after an asynchronous operation has completed
    inline void operation_finished(const stdx::error_code& error_code,
                                  std::size_t bytes_transferred)
    {
        stdx::lock_guard<stdx::mutex> async_lock(m_async_mutex);
        m_async_error = error_code;
        m_bytes_transferred = bytes_transferred;
        m_async_done.notify_one();
    }
    
    
    /// pointer to the underlying TCP connection used for reading & writing
    tcp::connection_ptr         m_conn_ptr;
    
    /// condition signaled whenever an asynchronous operation has completed
    stdx::mutex                m_async_mutex;
    
    /// condition signaled whenever an asynchronous operation has completed
    stdx::condition_variable            m_async_done;
    
    /// used to keep track of the result from the last asynchronous operation
    stdx::error_code   m_async_error;
    
    /// the number of bytes transferred by the last asynchronous operation
    std::size_t                 m_bytes_transferred;
    
    /// pointer to the start of the TCP connection's read buffer
    char_type *                 m_read_buf;
             
    /// buffer used to write output
    char_type                   m_write_buf[WRITE_BUFFER_SIZE];
};
    
    
///
/// stream: std::basic_iostream wrapper for TCP network connections
/// 
class stream
    : public std::basic_iostream<char, std::char_traits<char> >
{
public:

    // data type definitions required for iostream compatability
    typedef char                                char_type;
    typedef std::char_traits<char>::int_type    int_type;
    typedef std::char_traits<char>::off_type    off_type;
    typedef std::char_traits<char>::pos_type    pos_type;
    typedef std::char_traits<char>              traits_type;
    

    /**
     * constructs a TCP stream object for an existing TCP connection
     *
     * @param conn_ptr pointer to the TCP connection to use for reading & writing
     */
    explicit stream(const tcp::connection_ptr& conn_ptr)
        : std::basic_iostream<char, std::char_traits<char> >(NULL), m_tcp_buf(conn_ptr)
    {
        // initialize basic_iostream with pointer to the stream buffer
        std::basic_ios<char,std::char_traits<char> >::init(&m_tcp_buf);
    }
    
    /**
     * constructs a TCP stream object for a new TCP connection
     *
     * @param io_service asio service associated with the connection
     * @param ssl_flag if true then the connection will be encrypted using SSL 
     */
    explicit stream(stdx::asio::io_service& io_service,
                       const bool ssl_flag = false)
        : std::basic_iostream<char, std::char_traits<char> >(NULL), m_tcp_buf(io_service, ssl_flag)
    {
        // initialize basic_iostream with pointer to the stream buffer
        std::basic_ios<char,std::char_traits<char> >::init(&m_tcp_buf);
    }
    
    /**
     * constructs a TCP stream object for a new SSL/TCP connection
     *
     * @param io_service asio service associated with the connection
     * @param ssl_context asio ssl context associated with the connection
     */
    stream(stdx::asio::io_service& io_service,
              connection::ssl_context_type& ssl_context)
        : std::basic_iostream<char, std::char_traits<char> >(NULL), m_tcp_buf(io_service, ssl_context)
    {
        // initialize basic_iostream with pointer to the stream buffer
        std::basic_ios<char,std::char_traits<char> >::init(&m_tcp_buf);
    }
    
    /**
     * accepts a new tcp connection and performs SSL handshake if necessary
     *
     * @param tcp_acceptor object used to accept new connections
     * @return stdx::error_code contains error code if the connection fails
     *
     * @see stdx::asio::basic_socket_acceptor::accept()
     */
    inline stdx::error_code accept(stdx::asio::ip::tcp::acceptor& tcp_acceptor)
    {
        stdx::error_code ec = m_tcp_buf.get_connection().accept(tcp_acceptor);
        if (! ec && get_ssl_flag()) ec = m_tcp_buf.get_connection().handshake_server();
        return ec;
    }

    /**
     * connects to a remote endpoint and performs SSL handshake if necessary
     *
     * @param tcp_endpoint remote endpoint to connect to
     * @return stdx::error_code contains error code if the connection fails
     *
     * @see stdx::asio::basic_socket_acceptor::connect()
     */
    inline stdx::error_code connect(stdx::asio::ip::tcp::endpoint& tcp_endpoint)
    {
        stdx::error_code ec = m_tcp_buf.get_connection().connect(tcp_endpoint);
        if (! ec && get_ssl_flag()) ec = m_tcp_buf.get_connection().handshake_client();
        return ec;
    }
    
    /**
     * connects to a (IPv4) remote endpoint and performs SSL handshake if necessary
     *
     * @param remote_addr remote IP address (v4) to connect to
     * @param remote_port remote port number to connect to
     * @return stdx::error_code contains error code if the connection fails
     *
     * @see stdx::asio::basic_socket_acceptor::connect()
     */
    inline stdx::error_code connect(const stdx::asio::ip::address& remote_addr,
                                             const unsigned int remote_port)
    {
        stdx::asio::ip::tcp::endpoint tcp_endpoint(remote_addr, remote_port);
        stdx::error_code ec = m_tcp_buf.get_connection().connect(tcp_endpoint);
        if (! ec && get_ssl_flag()) ec = m_tcp_buf.get_connection().handshake_client();
        return ec;
    }

    /// closes the tcp connection
    inline void close(void) { m_tcp_buf.get_connection().close(); }

    /*
    Use close instead; basic_socket::cancel is deprecated for Windows XP.

    /// cancels any asynchronous operations pending on the tcp connection
    inline void cancel(void) { m_tcp_buf.get_connection().cancel(); }
    */

    /// returns true if the connection is currently open
    inline bool is_open(void) const { return m_tcp_buf.get_connection().is_open(); }
    
    /// returns true if the connection is encrypted using SSL
    inline bool get_ssl_flag(void) const { return m_tcp_buf.get_connection().get_ssl_flag(); }

    /// returns the client's IP address
    inline stdx::asio::ip::address get_remote_ip(void) const {
        return m_tcp_buf.get_connection().get_remote_ip();
    }
    
    /// returns a pointer to the stream buffer in use
    stream_buffer *rdbuf(void) { return &m_tcp_buf; }
    
    
private:
    
    /// the underlying TCP stream buffer used for reading & writing
    stream_buffer     m_tcp_buf;
};


}   // end namespace tcp
}   // end namespace pion

#endif
