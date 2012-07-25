// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HAS_CREATE_AND_DESTROY_HEADER__
#define __PION_HAS_CREATE_AND_DESTROY_HEADER__

#include "InterfaceStub.hpp"

///
/// This class has a corresponding create function (pion_create_hasCreateAndDestroy) and
/// destroy function (pion_destroy_hasCreateAndDestroy), as required for use by PionPlugin.
/// 
class hasCreateAndDestroy : public InterfaceStub
{
public:
    hasCreateAndDestroy(void) {}
    ~hasCreateAndDestroy() {}
};

#endif
