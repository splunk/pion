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

#include <boost/functional/hash.hpp>
#include <pion/PionConfig.hpp>


#if defined(PION_HAVE_UNORDERED_MAP)
	#include <tr1/unordered_map>
	#define PION_HASH_MAP std::tr1::unordered_map
	#define PION_HASH_MULTIMAP std::tr1::unordered_multimap
	#define PION_HASH_STRING boost::hash<std::string>
	#define PION_HASH(TYPE) boost::hash<TYPE>
#elif defined(PION_HAVE_EXT_HASH_MAP)
	#include <ext/hash_map>
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
	#include <hash_map>
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


#endif
