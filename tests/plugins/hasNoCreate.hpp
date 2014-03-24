// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HAS_NO_CREATE_HEADER__
#define __PION_HAS_NO_CREATE_HEADER__

#include "InterfaceStub.hpp"

///
/// This class has no corresponding create function or destroy function.
/// 
class hasNoCreate : public InterfaceStub
{
public:
    hasNoCreate(void) {}
    ~hasNoCreate() {}
};

#endif
