// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include "hasCreateButNoDestroy.hpp"
#include <pion/PionConfig.hpp>

/// creates new hasCreateButNoDestroy objects
extern "C" PION_SERVICE_API hasCreateButNoDestroy *pion_create_hasCreateButNoDestroy(void)
{
	return new hasCreateButNoDestroy();
}

