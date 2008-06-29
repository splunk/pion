#ifndef __HTTP_MESSAGE_PARSER_HEADER__
#define __HTTP_MESSAGE_PARSER_HEADER__

#include <boost/logic/tribool.hpp>
#include <pion/net/HTTPRequest.hpp>
#include <pion/net/HTTPResponse.hpp>
#include <pion/net/HTTPParser.hpp>

namespace pion {
namespace net {

/// HTTPMessage parser: a class that parses HTTP packets and produces 
///						HTTPMessage objects (HTTPRequest or HTTPResponse)
class PION_NET_API HTTPMessageParser : public pion::net::HTTPParser
{
public:

	/// constructs HTTPMessageParser object
	HTTPMessageParser(const bool is_request) : HTTPParser(is_request), m_content_len(-1), 
			m_content_len_read(0), m_headers_parsed(false) 
	{
	}

	/// reads the next portion of HTTP traffic data
	boost::tribool readNext(const char *ptr, size_t len);

	/// returns the current HTTPMessage (creates a new one if not already created)
	inline pion::net::HTTPMessage& getMessage();

	/// Initializes the HTTP message to a new HTTPResponse object created from given HTTPRequest
	void setRequest(const HTTPRequest& request_ref)
	{
		PION_ASSERT(!m_is_request);
		m_msg_ptr.reset( new HTTPResponse(request_ref) );
	}

private:
	inline bool hasContent() {  return m_msg_ptr->isChunked() || (m_content_len != 0); }

	boost::tribool processHeader(const char *ptr, size_t len);
	boost::tribool processContent(const char *ptr, size_t len);

	void determineContentLength();

	boost::tribool addToContentBuffer(const char *ptr, size_t len);

	void finishMessage()
	{
		if(m_is_request) {
			finishRequest(dynamic_cast<pion::net::HTTPRequest&>(getMessage()));
		} else {
			finishResponse(dynamic_cast<pion::net::HTTPResponse&>(getMessage()));
		}
	}

	boost::scoped_ptr<pion::net::HTTPMessage>	m_msg_ptr;
	bool										m_headers_parsed;
	size_t										m_content_len;
	size_t										m_content_len_read;
};


inline pion::net::HTTPMessage& HTTPMessageParser::getMessage()
{
	if(m_msg_ptr.get() == NULL) 
	{
		if(m_is_request)
			m_msg_ptr.reset(new pion::net::HTTPRequest);
		else
			m_msg_ptr.reset(new pion::net::HTTPResponse);
	}

	return *m_msg_ptr;
}

} // end namespace net
} // end namespace pion

#endif
