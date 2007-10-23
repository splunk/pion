// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/PionConfig.hpp>
#include <pion/PionCounter.hpp>


namespace pion {	// begin namespace pion
	

// static members of PionCounter
	
#ifdef PION_HAVE_APR
	boost::once_flag		PionCounter::m_init_flag = BOOST_ONCE_INIT;
#endif
	
	
// PionCounter member functions
	
void PionCounter::atomicInit(void)
{
#ifdef PION_HAVE_APR
	apr_pool_t *p;
	apr_initialize();
	apr_pool_create(&p, NULL);
	apr_atomic_init(p);
#endif
}

void PionCounter::atomicTerminate(void)
{
#ifdef PION_HAVE_APR
	apr_terminate();
#endif
}

}	// end namespace pion
