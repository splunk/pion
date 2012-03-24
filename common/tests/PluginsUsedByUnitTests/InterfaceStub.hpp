// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007-2011 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_INTERFACE_STUB_HEADER__
#define __PION_INTERFACE_STUB_HEADER__


///
/// Simple interface stub class for other unit test plugins
/// 
class InterfaceStub
{
public:
	InterfaceStub(void) {}
	virtual ~InterfaceStub() {}
	virtual void method(void) {}
	virtual void const_method(void) const {}
};

#endif
