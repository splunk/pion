// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HAS_CREATE_AND_DESTROY_HEADER__
#define __PION_HAS_CREATE_AND_DESTROY_HEADER__


///
/// This class has a corresponding create function (pion_create_hasCreateAndDestroy) and
/// destroy function (pion_destroy_hasCreateAndDestroy), as required for use by PionPlugin.
/// 
class hasCreateAndDestroy
{
public:
	hasCreateAndDestroy(void) {}
	~hasCreateAndDestroy() {}
};

#endif
