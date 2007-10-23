// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_HAS_NO_CREATE_HEADER__
#define __PION_HAS_NO_CREATE_HEADER__

///
/// This class has no corresponding create function or destroy function.
/// 
class hasNoCreate
{
public:
	hasNoCreate(void) {}
	~hasNoCreate() {}
};

#endif
