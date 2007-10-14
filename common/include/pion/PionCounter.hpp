// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PIONCOUNTER_HEADER__
#define __PION_PIONCOUNTER_HEADER__

#include <pion/PionConfig.hpp>
#ifdef PION_HAVE_APR
	#include <apr-1/apr_atomic.h>
	#include <boost/thread/once.hpp>
#else
	#include <boost/thread/mutex.hpp>
#endif


namespace pion {	// begin namespace pion


///
/// PionCounter: thread-safe 32-bit counter that is faster if APR is available
///
class PionCounter {
protected:
	
	/// increments the value of the counter
	inline void increment(void) {
		#ifdef PION_HAVE_APR
			apr_atomic_inc32(&m_counter);
		#else
			boost::mutex::scoped_lock counter_lock(m_mutex);
			++m_counter;
		#endif
	}
	
	/// decrement the value of the counter
	inline void decrement(void) {
		#ifdef PION_HAVE_APR
			apr_atomic_dec32(&m_counter);
		#else
			boost::mutex::scoped_lock counter_lock(m_mutex);
			--m_counter;
		#endif
	}
	
	/// adds a value to the counter
	template <typename IntegerType>
	inline void add(const IntegerType& n) {
		#ifdef PION_HAVE_APR
			apr_atomic_add32(&m_counter, n);
		#else
			boost::mutex::scoped_lock counter_lock(m_mutex);
			m_counter += n;
		#endif
	}
	
	/// subtracts a value from the counter
	template <typename IntegerType>
	inline void subtract(const IntegerType& n) {
		#ifdef PION_HAVE_APR
			apr_atomic_sub32(&m_counter, n);
		#else
			boost::mutex::scoped_lock counter_lock(m_mutex);
			m_counter -= n;
		#endif
	}

	/// assigns a new value to the counter
	template <typename IntegerType>
	inline void assign(const IntegerType& n) {
		#ifdef PION_HAVE_APR
			apr_atomic_set32(&m_counter, n);
		#else
			boost::mutex::scoped_lock counter_lock(m_mutex);
			m_counter = n;
		#endif
	}
	

public:

	/// default constructor initializes counter to zero (0)
	PionCounter(void) {
		#ifdef PION_HAVE_APR
			boost::call_once(PionCounter::atomicInit, m_init_flag);
		#endif
		assign(0);
	}

	/// virtual destructor: class may be extended
	virtual ~PionCounter() {}

	/// copy constructor
	PionCounter(const PionCounter& c) : m_counter(c.getValue()) {}
	
	/// assignment operator
	inline const PionCounter& operator=(const PionCounter& c) { assign(c.getValue()); return *this; }
	
	/// prefix increment
	inline const PionCounter& operator++(void) { increment(); return *this; }
	
	/// prefix decrement
	inline const PionCounter& operator--(void) { decrement(); return *this; }
	
	/// adds integer value to the counter
	template <typename IntegerType>
	inline const PionCounter& operator+=(const IntegerType& n) { add(n); return *this; }
	
	/// subtracts integer value from the counter
	template <typename IntegerType>
	inline const PionCounter& operator-=(const IntegerType& n) { subtract(n); return *this; }
	
	/// assigns integer value to the counter
	template <typename IntegerType>
	inline const PionCounter& operator=(const IntegerType& n) { assign(n); return *this; }

	/// resets the counter to zero
	inline void reset(void) { assign(0); }

	/// returns the value of the counter
	inline unsigned long getValue(void) const {
		#ifdef PION_HAVE_APR
			return apr_atomic_read32(const_cast<apr_uint32_t*>(&m_counter));
		#else
			return m_counter;
		#endif
	}
	

private:

	/// initializes APR atomic operations library
	static void atomicInit(void);
	

#ifdef PION_HAVE_APR

	/// used to make sure that atomicInit() is called only once
	static boost::once_flag		m_init_flag;

	/// counter used when APR is available
	apr_uint32_t				m_counter;

#else

	/// mutex protects the counter when APR is not available
	boost::mutex				m_mutex;
	
	/// counter used when APR is not available or when thread safety is disabled
	unsigned long				m_counter;

#endif

};


}	// end namespace pion

#endif

