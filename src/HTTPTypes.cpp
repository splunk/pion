// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/lexical_cast.hpp>
#include <boost/thread/mutex.hpp>
#include <pion/net/HTTPTypes.hpp>
#include <cstdio>
#include <ctime>


namespace pion {		// begin namespace pion
namespace net {		// begin namespace net (Pion Network Library)


// generic strings used by HTTP
const std::string	HTTPTypes::STRING_EMPTY;
const std::string	HTTPTypes::STRING_CRLF("\x0D\x0A");
const std::string	HTTPTypes::STRING_HTTP_VERSION("HTTP/");
const std::string	HTTPTypes::HEADER_NAME_VALUE_DELIMITER(": ");

// common HTTP header names
const std::string	HTTPTypes::HEADER_HOST("Host");
const std::string	HTTPTypes::HEADER_COOKIE("Cookie");
const std::string	HTTPTypes::HEADER_SET_COOKIE("Set-Cookie");
const std::string	HTTPTypes::HEADER_CONNECTION("Connection");
const std::string	HTTPTypes::HEADER_CONTENT_TYPE("Content-Type");
const std::string	HTTPTypes::HEADER_CONTENT_LENGTH("Content-Length");
const std::string	HTTPTypes::HEADER_CONTENT_LOCATION("Content-Location");
const std::string	HTTPTypes::HEADER_CONTENT_ENCODING("Content-Encoding");
const std::string	HTTPTypes::HEADER_LAST_MODIFIED("Last-Modified");
const std::string	HTTPTypes::HEADER_IF_MODIFIED_SINCE("If-Modified-Since");
const std::string	HTTPTypes::HEADER_TRANSFER_ENCODING("Transfer-Encoding");
const std::string	HTTPTypes::HEADER_LOCATION("Location");
const std::string	HTTPTypes::HEADER_AUTHORIZATION("Authorization");
const std::string	HTTPTypes::HEADER_REFERER("Referer");
const std::string	HTTPTypes::HEADER_USER_AGENT("User-Agent");
const std::string	HTTPTypes::HEADER_X_FORWARDED_FOR("X-Forwarded-For");

// common HTTP content types
const std::string	HTTPTypes::CONTENT_TYPE_HTML("text/html");
const std::string	HTTPTypes::CONTENT_TYPE_TEXT("text/plain");
const std::string	HTTPTypes::CONTENT_TYPE_XML("text/xml");
const std::string	HTTPTypes::CONTENT_TYPE_URLENCODED("application/x-www-form-urlencoded");

// common HTTP request methods
const std::string	HTTPTypes::REQUEST_METHOD_HEAD("HEAD");
const std::string	HTTPTypes::REQUEST_METHOD_GET("GET");
const std::string	HTTPTypes::REQUEST_METHOD_PUT("PUT");
const std::string	HTTPTypes::REQUEST_METHOD_POST("POST");
const std::string	HTTPTypes::REQUEST_METHOD_DELETE("DELETE");

// common HTTP response messages
const std::string	HTTPTypes::RESPONSE_MESSAGE_OK("OK");
const std::string	HTTPTypes::RESPONSE_MESSAGE_CREATED("Created");
const std::string	HTTPTypes::RESPONSE_MESSAGE_NO_CONTENT("No Content");
const std::string	HTTPTypes::RESPONSE_MESSAGE_FOUND("Found");
const std::string	HTTPTypes::RESPONSE_MESSAGE_UNAUTHORIZED("Unauthorized");
const std::string	HTTPTypes::RESPONSE_MESSAGE_FORBIDDEN("Forbidden");
const std::string	HTTPTypes::RESPONSE_MESSAGE_NOT_FOUND("Not Found");
const std::string	HTTPTypes::RESPONSE_MESSAGE_METHOD_NOT_ALLOWED("Method Not Allowed");
const std::string	HTTPTypes::RESPONSE_MESSAGE_NOT_MODIFIED("Not Modified");
const std::string	HTTPTypes::RESPONSE_MESSAGE_BAD_REQUEST("Bad Request");
const std::string	HTTPTypes::RESPONSE_MESSAGE_SERVER_ERROR("Server Error");
const std::string	HTTPTypes::RESPONSE_MESSAGE_NOT_IMPLEMENTED("Not Implemented");
const std::string	HTTPTypes::RESPONSE_MESSAGE_CONTINUE("Continue");

// common HTTP response codes
const unsigned int	HTTPTypes::RESPONSE_CODE_OK = 200;
const unsigned int	HTTPTypes::RESPONSE_CODE_CREATED = 201;
const unsigned int	HTTPTypes::RESPONSE_CODE_NO_CONTENT = 204;
const unsigned int	HTTPTypes::RESPONSE_CODE_FOUND = 302;
const unsigned int	HTTPTypes::RESPONSE_CODE_UNAUTHORIZED = 401;
const unsigned int	HTTPTypes::RESPONSE_CODE_FORBIDDEN = 403;
const unsigned int	HTTPTypes::RESPONSE_CODE_NOT_FOUND = 404;
const unsigned int	HTTPTypes::RESPONSE_CODE_METHOD_NOT_ALLOWED = 405;
const unsigned int	HTTPTypes::RESPONSE_CODE_NOT_MODIFIED = 304;
const unsigned int	HTTPTypes::RESPONSE_CODE_BAD_REQUEST = 400;
const unsigned int	HTTPTypes::RESPONSE_CODE_SERVER_ERROR = 500;
const unsigned int	HTTPTypes::RESPONSE_CODE_NOT_IMPLEMENTED = 501;
const unsigned int	HTTPTypes::RESPONSE_CODE_CONTINUE = 100;


// static member functions
	
bool HTTPTypes::base64_decode(const std::string &input, std::string &output)
{
	static const char nop = -1; 
	static const char decoding_data[] = {
		nop,nop,nop,nop, nop,nop,nop,nop, nop,nop,nop,nop, nop,nop,nop,nop,
		nop,nop,nop,nop, nop,nop,nop,nop, nop,nop,nop,nop, nop,nop,nop,nop,
		nop,nop,nop,nop, nop,nop,nop,nop, nop,nop,nop, 62, nop,nop,nop, 63,
		52, 53, 54,  55,  56, 57, 58, 59,  60, 61,nop,nop, nop,nop,nop,nop,
		nop, 0,  1,   2,   3,  4,  5,  6,   7,  8,  9, 10,  11, 12, 13, 14,
		15, 16, 17,  18,  19, 20, 21, 22,  23, 24, 25,nop, nop,nop,nop,nop,
		nop,26, 27,  28,  29, 30, 31, 32,  33, 34, 35, 36,  37, 38, 39, 40,
		41, 42, 43,  44,  45, 46, 47, 48,  49, 50, 51,nop, nop,nop,nop,nop,
		nop,nop,nop,nop, nop,nop,nop,nop, nop,nop,nop,nop, nop,nop,nop,nop,
		nop,nop,nop,nop, nop,nop,nop,nop, nop,nop,nop,nop, nop,nop,nop,nop,
		nop,nop,nop,nop, nop,nop,nop,nop, nop,nop,nop,nop, nop,nop,nop,nop,
		nop,nop,nop,nop, nop,nop,nop,nop, nop,nop,nop,nop, nop,nop,nop,nop,
		nop,nop,nop,nop, nop,nop,nop,nop, nop,nop,nop,nop, nop,nop,nop,nop,
		nop,nop,nop,nop, nop,nop,nop,nop, nop,nop,nop,nop, nop,nop,nop,nop,
		nop,nop,nop,nop, nop,nop,nop,nop, nop,nop,nop,nop, nop,nop,nop,nop,
		nop,nop,nop,nop, nop,nop,nop,nop, nop,nop,nop,nop, nop,nop,nop,nop
		};

	unsigned int input_length=input.size();
	const char * input_ptr = input.data();

	// allocate space for output string
	output.clear();
	output.reserve(((input_length+2)/3)*4);

	// for each 4-bytes sequence from the input, extract 4 6-bits sequences by droping first two bits
	// and regenerate into 3 8-bits sequence

	for (unsigned int i=0; i<input_length;i++) {
		char base64code0;
		char base64code1;
		char base64code2 = 0;   // initialized to 0 to suppress warnings
		char base64code3;

		base64code0 = decoding_data[static_cast<int>(input_ptr[i])];
		if(base64code0==nop)			// non base64 character
			return false;
		if(!(++i<input_length)) // we need at least two input bytes for first byte output
			return false;
		base64code1 = decoding_data[static_cast<int>(input_ptr[i])];
		if(base64code1==nop)			// non base64 character
			return false;

		output += ((base64code0 << 2) | ((base64code1 >> 4) & 0x3));

		if(++i<input_length) {
			char c = input_ptr[i];
			if(c =='=') { // padding , end of input
				BOOST_ASSERT( (base64code1 & 0x0f)==0);
				return true;
			}
			base64code2 = decoding_data[static_cast<int>(input_ptr[i])];
			if(base64code2==nop)			// non base64 character
				return false;

			output += ((base64code1 << 4) & 0xf0) | ((base64code2 >> 2) & 0x0f);
		}

		if(++i<input_length) {
			char c = input_ptr[i];
			if(c =='=') { // padding , end of input
				BOOST_ASSERT( (base64code2 & 0x03)==0);
				return true;
			}
			base64code3 = decoding_data[static_cast<int>(input_ptr[i])];
			if(base64code3==nop)			// non base64 character
				return false;

			output += (((base64code2 << 6) & 0xc0) | base64code3 );
		}

	}

	return true;
}

bool HTTPTypes::base64_encode(const std::string &input, std::string &output)
{
	static const char encoding_data[] = 
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	unsigned int input_length=input.size();
	const char * input_ptr = input.data();

	// allocate space for output string
	output.clear();
	output.reserve(((input_length+2)/3)*4);

	// for each 3-bytes sequence from the input, extract 4 6-bits sequences and encode using 
	// encoding_data lookup table.
	// if input do not contains enough chars to complete 3-byte sequence,use pad char '=' 
	for (unsigned int i=0; i<input_length;i++) {
		int base64code0=0;
		int base64code1=0;
		int base64code2=0;
		int base64code3=0;

		base64code0 = (input_ptr[i] >> 2)  & 0x3f;	// 1-byte 6 bits
		output += encoding_data[base64code0];
		base64code1 = (input_ptr[i] << 4 ) & 0x3f;	// 1-byte 2 bits +

		if (++i < input_length) {
			base64code1 |= (input_ptr[i] >> 4) & 0x0f; // 2-byte 4 bits
			output += encoding_data[base64code1];
			base64code2 = (input_ptr[i] << 2) & 0x3f;  // 2-byte 4 bits + 

			if (++i < input_length) {
				base64code2 |= (input_ptr[i] >> 6) & 0x03; // 3-byte 2 bits
				base64code3  = input_ptr[i] & 0x3f;		  // 3-byte 6 bits
				output += encoding_data[base64code2];
				output += encoding_data[base64code3];
			} else {
				output += encoding_data[base64code2];
				output += '=';
			}
		} else {
			output += encoding_data[base64code1];
			output += '=';
			output += '=';
		}
	}

	return true;
}

std::string HTTPTypes::url_decode(const std::string& str)
{
	char decode_buf[3];
	std::string result;
	result.reserve(str.size());
	
	for (std::string::size_type pos = 0; pos < str.size(); ++pos) {
		switch(str[pos]) {
		case '+':
			// convert to space character
			result += ' ';
			break;
		case '%':
			// decode hexidecimal value
			if (pos + 2 < str.size()) {
				decode_buf[0] = str[++pos];
				decode_buf[1] = str[++pos];
				decode_buf[2] = '\0';
				result += static_cast<char>( strtol(decode_buf, 0, 16) );
			} else {
				// recover from error by not decoding character
				result += '%';
			}
			break;
		default:
			// character does not need to be escaped
			result += str[pos];
		}
	};
	
	return result;
}
	
std::string HTTPTypes::url_encode(const std::string& str)
{
	char encode_buf[4];
	std::string result;
	encode_buf[0] = '%';
	result.reserve(str.size());

	// character selection for this algorithm is based on the following url:
	// http://www.blooberry.com/indexdot/html/topics/urlencoding.htm
	
	for (std::string::size_type pos = 0; pos < str.size(); ++pos) {
		switch(str[pos]) {
		default:
			if (str[pos] > 32 && str[pos] < 127) {
				// character does not need to be escaped
				result += str[pos];
				break;
			}
			// else pass through to next case
		case ' ':	
		case '$': case '&': case '+': case ',': case '/': case ':':
		case ';': case '=': case '?': case '@': case '"': case '<':
		case '>': case '#': case '%': case '{': case '}': case '|':
		case '\\': case '^': case '~': case '[': case ']': case '`':
			// the character needs to be encoded
			sprintf(encode_buf+1, "%.2X", str[pos]);
			result += encode_buf;
			break;
		}
	};
	
	return result;
}	

std::string HTTPTypes::get_date_string(const time_t t)
{
	// use mutex since time functions are normally not thread-safe
	static boost::mutex	time_mutex;
	static const char *TIME_FORMAT = "%a, %d %b %Y %H:%M:%S GMT";
	static const unsigned int TIME_BUF_SIZE = 100;
	char time_buf[TIME_BUF_SIZE+1];

	boost::mutex::scoped_lock time_lock(time_mutex);
	if (strftime(time_buf, TIME_BUF_SIZE, TIME_FORMAT, gmtime(&t)) == 0)
		time_buf[0] = '\0';	// failed; resulting buffer is indeterminate
	time_lock.unlock();

	return std::string(time_buf);
}

std::string HTTPTypes::make_query_string(const QueryParams& query_params)
{
	std::string query_string;
	for (QueryParams::const_iterator i = query_params.begin(); i != query_params.end(); ++i) {
		if (i != query_params.begin())
			query_string += '&';
		query_string += url_encode(i->first);
		query_string += '=';
		query_string += url_encode(i->second);
	}
	return query_string;
}

std::string HTTPTypes::make_set_cookie_header(const std::string& name,
											  const std::string& value,
											  const std::string& path,
											  const bool has_max_age,
											  const unsigned long max_age)
{
	std::string set_cookie_header(name);
	set_cookie_header += "=\"";
	set_cookie_header += value;
	set_cookie_header += "\"; Version=\"1\"";
	if (! path.empty()) {
		set_cookie_header += "; Path=\"";
		set_cookie_header += path;
		set_cookie_header += '\"';
	}
	if (has_max_age) {
		set_cookie_header += "; Max-Age=\"";
		set_cookie_header += boost::lexical_cast<std::string>(max_age);
		set_cookie_header += '\"';
	}
	return set_cookie_header;
}

	
}	// end namespace net
}	// end namespace pion

