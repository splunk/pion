// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include "hasNoCreate.hpp"
#include <pion/PionConfig.hpp>

/// arbitrary function for the library to export
extern "C" PION_SERVICE_API int f(void)
{
	return 4;
}
