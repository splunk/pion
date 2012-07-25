// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HAS_CREATE_BUT_NO_DESTROY_HEADER__
#define __PION_HAS_CREATE_BUT_NO_DESTROY_HEADER__

#include "InterfaceStub.hpp"

///
/// This class has a corresponding create function (pion_create_hasCreateButNoDestroy) but no corresponding destroy function.
/// 
class hasCreateButNoDestroy : public InterfaceStub
{
public:
    hasCreateButNoDestroy(void) {}
    ~hasCreateButNoDestroy() {}
};

#endif
