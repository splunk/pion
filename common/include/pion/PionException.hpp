// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PIONEXCEPTION_HEADER__
#define __PION_PIONEXCEPTION_HEADER__

#include <pion/PionConfig.hpp>
#include <exception>
#include <string>
#include <cstdio>


namespace pion {	// begin namespace pion

///
/// PionException: basic exception class that defines a what() function
///
class PionException :
	public std::exception
{
public:
	// virtual destructor does not throw
	virtual ~PionException() throw () {}

	// constructors used for constant messages
	PionException(const char *what_msg) : m_what_msg(what_msg) {}
	PionException(const std::string& what_msg) : m_what_msg(what_msg) {}
	
	// constructors used for messages with a parameter
	PionException(const char *description, const std::string& param)
		: m_what_msg(std::string(description) + param) {}
	PionException(std::string description, const std::string& param)
		: m_what_msg(description + param) {}

	/// returns a desciptive message for the exception
	virtual const char* what() const throw() {
		return m_what_msg.c_str();
	}
	
private:
	
	// message returned by what() function
	const std::string	m_what_msg;
};


///
/// BadAssertException: exception thrown if an assertion (PION_ASSERT) fails
///
class BadAssertException : public PionException {
public:
	BadAssertException(const std::string& file, unsigned long line)
		: PionException(make_string(file, line)) {}
	
private:
	static std::string make_string(const std::string& file, unsigned long line) {
		std::string result("Assertion failed at ");
		result += file;
		char line_buf[50];
		sprintf(line_buf, " line %lu", line);
		result += line_buf;
		return result;
	}
};
	
}	// end namespace pion


// define PION_ASSERT macro to check assertions when debugging mode is enabled
#ifdef NDEBUG
	#define PION_ASSERT(EXPR)	((void)0)
#else
	#define PION_ASSERT(EXPR)	if (!(EXPR)) { throw BadAssertException(__FILE__, __LINE__); }
#endif


#endif
