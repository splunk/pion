// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file COPYING or copy at http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/PionConfig.hpp>
#include <pion/PionCounter.hpp>
#include <boost/test/unit_test.hpp>

using namespace pion;

class NewPionCounter_F : public PionCounter {
public:
	NewPionCounter_F() {
	}
	~NewPionCounter_F() {
	}

private:
};

BOOST_FIXTURE_TEST_SUITE(NewPionCounter_S, NewPionCounter_F)

BOOST_AUTO_TEST_CASE(checkGetValueReturnsZero) {
	BOOST_CHECK_EQUAL(getValue(), 0);
}

BOOST_AUTO_TEST_CASE(checkGetValueAfterIncrement) {
	increment();
	BOOST_CHECK_EQUAL(getValue(), 1);
}

BOOST_AUTO_TEST_CASE(checkGetValueAfterDecrement) {
	decrement();
	BOOST_CHECK_EQUAL(getValue(), -1);
}

BOOST_AUTO_TEST_CASE(checkGetValueAfterReset) {
	reset();
	BOOST_CHECK_EQUAL(getValue(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

class PionCounterSetTo5_F : public PionCounter {
public:
	PionCounterSetTo5_F() {
		assign(5);
	}
	~PionCounterSetTo5_F() {
	}

private:
};

BOOST_FIXTURE_TEST_SUITE(PionCounterSetTo5_S, PionCounterSetTo5_F)

BOOST_AUTO_TEST_CASE(checkGetValueReturns5) {
	BOOST_CHECK_EQUAL(getValue(), 5);
}

BOOST_AUTO_TEST_CASE(checkGetValueAfterIncrement) {
	increment();
	BOOST_CHECK_EQUAL(getValue(), 6);
}

BOOST_AUTO_TEST_CASE(checkGetValueAfterDecrement) {
	decrement();
	BOOST_CHECK_EQUAL(getValue(), 4);
}

BOOST_AUTO_TEST_CASE(checkGetValueAfterReset) {
	reset();
	BOOST_CHECK_EQUAL(getValue(), 0);
}

BOOST_AUTO_TEST_SUITE_END()
