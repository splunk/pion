// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HASH_MAP_HEADER__
#define __PION_HASH_MAP_HEADER__

#include <string>
#include <locale>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/functional/hash.hpp>
#include <pion/config.hpp>

#if defined(PION_HAVE_UNORDERED_MAP)
    #include <tr1/unordered_map>
#elif defined(PION_HAVE_EXT_HASH_MAP)
    #include <ext/hash_map>
#elif defined(PION_HAVE_HASH_MAP)
    #include <hash_map>
#endif


namespace pion {    // begin namespace pion


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

    /// case insensitive string equality predicate
    /// copied from boost.unordered hash_equality documentation
    /// http://www.boost.org/doc/libs/1_50_0/doc/html/unordered/hash_equality.html
    struct iequal_to
        : std::binary_function<std::string, std::string, bool>
    {
        bool operator()(std::string const& x,
                        std::string const& y) const
        {
            return boost::algorithm::iequals(x, y, std::locale());
        }
    };
    
    /// case insensitive hash generic function
    /// copied from boost.unordered hash_equality documentation
    /// http://www.boost.org/doc/libs/1_50_0/doc/html/unordered/hash_equality.html
    struct ihash
        : std::unary_function<std::string, std::size_t>
    {
        std::size_t operator()(std::string const& x) const
        {
            std::size_t seed = 0;
            std::locale locale;
            
            for(std::string::const_iterator it = x.begin();
                it != x.end(); ++it)
            {
                boost::hash_combine(seed, std::toupper(*it, locale));
            }
            
            return seed;
        }
    };
    
#ifdef _MSC_VER
    /// case insensitive extension of stdext::hash_compare for std::string
    struct ihash_windows : public stdext::hash_compare<std::string, boost::algorithm::is_iless > {
        // makes operator() with two arguments visible, otherwise it would be hidden by the operator() defined here
        using stdext::hash_compare<std::string, boost::algorithm::is_iless>::operator();
        
        inline size_t operator()(const std::string& str) const {
            return ihash()(str);
        }
    };

    /// data type for case-insensitive dictionary of strings
    typedef PION_HASH_MULTIMAP<std::string, std::string, algorithm::ihash_windows >    ihash_multimap;
#else
    /// data type for case-insensitive dictionary of strings
    typedef PION_HASH_MULTIMAP<std::string, std::string, ihash, iequal_to >    ihash_multimap;
#endif


}   // end namespace pion

#endif
