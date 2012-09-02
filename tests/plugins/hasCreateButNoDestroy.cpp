// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include "hasCreateButNoDestroy.hpp"
#include <pion/config.hpp>

/// creates new hasCreateButNoDestroy objects
extern "C" PION_SERVICE_API hasCreateButNoDestroy *pion_create_hasCreateButNoDestroy(void)
{
    return new hasCreateButNoDestroy();
}

