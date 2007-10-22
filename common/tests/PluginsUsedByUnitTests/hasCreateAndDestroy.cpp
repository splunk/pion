// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include "hasCreateAndDestroy.hpp"
#include <pion/PionConfig.hpp>

/// creates new hasCreateAndDestroy objects
extern "C" PION_SERVICE_API hasCreateAndDestroy *pion_create_hasCreateAndDestroy(void)
{
	return new hasCreateAndDestroy();
}

/// destroys hasCreateAndDestroy objects
extern "C" PION_SERVICE_API void pion_destroy_hasCreateAndDestroy(hasCreateAndDestroy *service_ptr)
{
	delete service_ptr;
}
