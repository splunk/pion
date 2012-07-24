// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PIONHASHMAP_HEADER__
#define __PION_PIONHASHMAP_HEADER__

#include <string>
#include <cctype>
#include <boost/functional/hash.hpp>
#include <pion/config.hpp>

#if defined(PION_HAVE_UNORDERED_MAP)
	#include <tr1/unordered_map>
#elif defined(PION_HAVE_EXT_HASH_MAP)
	#include <ext/hash_map>
#elif defined(PION_HAVE_HASH_MAP)
	#include <hash_map>
#endif


namespace pion {	// begin namespace pion


#if defined(PION_HAVE_UNORDERED_MAP)
	#define PION_HASH_MAP std::tr1::unordered_map
	#define PION_HASH_MULTIMAP std::tr1::unordered_multimap
	#define PION_HASH_STRING boost::hash<std::string>
	#define PION_HASH(TYPE) boost::hash<TYPE>
#elif defined(PION_HAVE_EXT_HASH_MAP)
	#if __GNUC__ >= 3
		#define PION_HASH_MAP __gnu_cxx::hash_map
		#define PION_HASH_MULTIMAP __gnu_cxx::hash_multimap
	#else
		#define PION_HASH_MAP hash_map
		#define PION_HASH_MULTIMAP hash_multimap
	#endif
	#define PION_HASH_STRING boost::hash<std::string>
	#define PION_HASH(TYPE) boost::hash<TYPE>
#elif defined(PION_HAVE_HASH_MAP)
	#ifdef _MSC_VER
		#define PION_HASH_MAP stdext::hash_map
		#define PION_HASH_MULTIMAP stdext::hash_multimap
		#define PION_HASH_STRING stdext::hash_compare<std::string, std::less<std::string> >
		#define PION_HASH(TYPE) stdext::hash_compare<TYPE, std::less<TYPE> >
	#else
		#define PION_HASH_MAP hash_map
		#define PION_HASH_MULTIMAP hash_multimap
		#define PION_HASH_STRING boost::hash<std::string>
		#define PION_HASH(TYPE) boost::hash<TYPE>
	#endif
#endif


/// returns true if two strings are equal (ignoring case)
struct CaseInsensitiveEqual {
	inline bool operator()(const std::string& str1, const std::string& str2) const {
		if (str1.size() != str2.size())
			return false;
		std::string::const_iterator it1 = str1.begin();
		std::string::const_iterator it2 = str2.begin();
		while ( (it1!=str1.end()) && (it2!=str2.end()) ) {
			if (tolower(*it1) != tolower(*it2))
				return false;
			++it1;
			++it2;
		}
		return true;
	}
};


/// case insensitive hash function for std::string
struct CaseInsensitiveHash {
	inline unsigned long operator()(const std::string& str) const {
		unsigned long value = 0;
		for (std::string::const_iterator i = str.begin(); i!= str.end(); ++i)
			value = static_cast<unsigned char>(tolower(*i)) + (value << 6) + (value << 16) - value;
		return value;
	}
};


/// returns true if str1 < str2 (ignoring case)
struct CaseInsensitiveLess {
	inline bool operator()(const std::string& str1, const std::string& str2) const {
		std::string::const_iterator it1 = str1.begin();
		std::string::const_iterator it2 = str2.begin();
		while ( (it1 != str1.end()) && (it2 != str2.end()) ) {
			if (tolower(*it1) != tolower(*it2))
				return (tolower(*it1) < tolower(*it2));
			++it1;
			++it2;
		}
		return (str1.size() < str2.size());
	}
};


#ifdef _MSC_VER
	/// case insensitive extension of stdext::hash_compare for std::string
	struct CaseInsensitiveHashCompare : public stdext::hash_compare<std::string, CaseInsensitiveLess> {
		// makes operator() with two arguments visible, otherwise it would be hidden by the operator() defined here
		using stdext::hash_compare<std::string, CaseInsensitiveLess>::operator();
	
		inline size_t operator()(const std::string& str) const {
			return CaseInsensitiveHash()(str);
		}
	};
#endif


/// data type for case-insensitive dictionary of strings
#ifdef _MSC_VER
	typedef PION_HASH_MULTIMAP<std::string, std::string, CaseInsensitiveHashCompare>	StringDictionary;
#else
	typedef PION_HASH_MULTIMAP<std::string, std::string, CaseInsensitiveHash, CaseInsensitiveEqual >	StringDictionary;
#endif

/// data type for a dictionary of strings
//typedef PION_HASH_MULTIMAP<std::string, std::string, PION_HASH_STRING >	StringDictionary;


}	// end namespace pion

#endif
