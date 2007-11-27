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
#include <boost/thread/mutex.hpp>


namespace pion {	// begin namespace pion


///
/// PionCounter: thread-safe 64-bit integer counter
///
class PionCounter {
protected:
	
	/// increments the value of the counter
	inline void increment(void) {
		boost::mutex::scoped_lock counter_lock(m_mutex);
		++m_value;
	}
	
	/// decrement the value of the counter
	inline void decrement(void) {
		boost::mutex::scoped_lock counter_lock(m_mutex);
		--m_value;
	}
	
	/// adds a value to the counter
	template <typename IntegerType>
	inline void add(const IntegerType& n) {
		boost::mutex::scoped_lock counter_lock(m_mutex);
		m_value += n;
	}
	
	/// subtracts a value from the counter
	template <typename IntegerType>
	inline void subtract(const IntegerType& n) {
		boost::mutex::scoped_lock counter_lock(m_mutex);
		m_value -= n;
	}

	/// assigns a new value to the counter
	template <typename IntegerType>
	inline void assign(const IntegerType& n) {
		boost::mutex::scoped_lock counter_lock(m_mutex);
		m_value = n;
	}
	

public:

	/// default constructor initializes counter
	explicit PionCounter(unsigned long n = 0) {
		assign(n);
	}

	/// virtual destructor: class may be extended
	virtual ~PionCounter() {}

	/// copy constructor
	PionCounter(const PionCounter& c) : m_value(c.getValue()) {}
	
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

	/// compares an integer value to the counter
	template <typename IntegerType>
	inline bool operator==(const IntegerType& n) const { return getValue() == n; }
	
	/// compares an integer value to the counter
	template <typename IntegerType>
	inline bool operator>(const IntegerType& n) const { return getValue() > n; }
	
	/// compares an integer value to the counter
	template <typename IntegerType>
	inline bool operator<(const IntegerType& n) const { return getValue() < n; }
	
	/// compares an integer value to the counter
	template <typename IntegerType>
	inline bool operator>=(const IntegerType& n) const { return getValue() >= n; }
	
	/// compares an integer value to the counter
	template <typename IntegerType>
	inline bool operator<=(const IntegerType& n) const { return getValue() <= n; }
	
	/// resets the counter to zero
	inline void reset(void) { assign(0); }

	/// returns the value of the counter
	inline unsigned long long getValue(void) const {
		return m_value;
	}
	

private:

	/// mutex used to protect the counter's value
	boost::mutex				m_mutex;
	
	/// used to keep track of the counter's value
	unsigned long long			m_value;
};


}	// end namespace pion

#endif

