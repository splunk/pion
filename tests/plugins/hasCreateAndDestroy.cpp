// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2012 Cloudmeter, Inc.  (http://www.cloudmeter.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include "hasCreateAndDestroy.hpp"
#include <pion/config.hpp>

/// creates new hasCreateAndDestroy objects
extern "C" PION_API hasCreateAndDestroy *pion_create_hasCreateAndDestroy(void)
{
    return new hasCreateAndDestroy();
}

/// destroys hasCreateAndDestroy objects
extern "C" PION_API void pion_destroy_hasCreateAndDestroy(hasCreateAndDestroy *service_ptr)
{
    delete service_ptr;
}
