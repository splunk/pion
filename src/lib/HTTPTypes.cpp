// -----------------------------------------------------------------
// libpion: a C++ framework for building lightweight HTTP interfaces
// -----------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.
// 
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//

#include "HTTPTypes.hpp"


namespace pion {		// begin namespace pion


// static members of HTTPTypes
	
const std::string	HTTPTypes::STRING_EMPTY;
const std::string	HTTPTypes::STRING_CRLF("\r\n");
const std::string	HTTPTypes::STRING_HTTP_VERSION("HTTP/1.1");
const std::string	HTTPTypes::HEADER_NAME_VALUE_DELIMINATOR(": ");

const std::string	HTTPTypes::HEADER_HOST("Host");
const std::string	HTTPTypes::HEADER_COOKIE("Cookie");
const std::string	HTTPTypes::HEADER_CONNECTION("Connection");
const std::string	HTTPTypes::HEADER_CONTENT_TYPE("Content-Type");
const std::string	HTTPTypes::HEADER_CONTENT_LENGTH("Content-Length");

const std::string	HTTPTypes::CONTENT_TYPE_HTML("text/html");
const std::string	HTTPTypes::CONTENT_TYPE_TEXT("text/plain");
const std::string	HTTPTypes::CONTENT_TYPE_XML("text/xml");

const std::string	HTTPTypes::RESPONSE_MESSAGE_OK("OK");
const std::string	HTTPTypes::RESPONSE_MESSAGE_NOT_FOUND("Request Not Found");
const std::string	HTTPTypes::RESPONSE_MESSAGE_BAD_REQUEST("Bad Request");

const unsigned int	HTTPTypes::RESPONSE_CODE_OK = 200;
const unsigned int	HTTPTypes::RESPONSE_CODE_NOT_FOUND = 404;
const unsigned int	HTTPTypes::RESPONSE_CODE_BAD_REQUEST = 400;


}	// end namespace pion

