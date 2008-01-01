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
#include <pion/tests/PionUnitTestDefs.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>

using namespace pion;

class NewPionCounter_F : public PionCounter {
public:
	NewPionCounter_F() {
	}
	~NewPionCounter_F() {
	}

private:
};

BOOST_AUTO_TEST_SUITE_FIXTURE_TEMPLATE(NewPionCounter_S, boost::mpl::list<NewPionCounter_F>)

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetValueReturnsZero) {
	BOOST_CHECK_EQUAL(getValue(), static_cast<unsigned long long>(0));
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetValueAfterIncrement) {
	increment();
	BOOST_CHECK_EQUAL(getValue(), static_cast<unsigned long long>(1));
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetValueAfterDecrement) {
	decrement();
	BOOST_CHECK_EQUAL(getValue(), static_cast<unsigned long long>(-1));
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetValueAfterReset) {
	reset();
	BOOST_CHECK_EQUAL(getValue(), static_cast<unsigned long long>(0));
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

BOOST_AUTO_TEST_SUITE_FIXTURE_TEMPLATE(PionCounterSetTo5_S, boost::mpl::list<PionCounterSetTo5_F>)

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetValueReturns5) {
	BOOST_CHECK_EQUAL(getValue(), static_cast<unsigned long long>(5));
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetValueAfterIncrement) {
	increment();
	BOOST_CHECK_EQUAL(getValue(), static_cast<unsigned long long>(6));
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetValueAfterDecrement) {
	decrement();
	BOOST_CHECK_EQUAL(getValue(), static_cast<unsigned long long>(4));
}

BOOST_AUTO_TEST_CASE_FIXTURE_TEMPLATE(checkGetValueAfterReset) {
	reset();
	BOOST_CHECK_EQUAL(getValue(), static_cast<unsigned long long>(0));
}

BOOST_AUTO_TEST_SUITE_END()
