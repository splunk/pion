// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/PionConfig.hpp>
#include <pion/PionDateTime.hpp>
#include <boost/test/unit_test.hpp>

using namespace pion;

class NewPionTimeFacet_F : public PionTimeFacet {
public:
	NewPionTimeFacet_F() {
	}
	virtual ~NewPionTimeFacet_F() {
	}

private:
};

BOOST_FIXTURE_TEST_SUITE(NewPionTimeFacet_S, NewPionTimeFacet_F)

BOOST_AUTO_TEST_CASE(checkPionTimeFacetReadDate) {
	std::stringstream tmp_stream("2005-10-11");
	PionDateTime t;
	setFormat("%Y-%m-%d");
	read(tmp_stream, t);
	BOOST_CHECK_EQUAL(t.date().year(), 2005);
	BOOST_CHECK_EQUAL(t.date().month(), 10);
	BOOST_CHECK_EQUAL(t.date().day(), 11);
}

BOOST_AUTO_TEST_CASE(checkPionTimeFacetWriteDate) {
	std::stringstream tmp_stream;
	const PionDateTime t(boost::gregorian::date(2005, 10, 11));
	setFormat("%Y-%m-%d");
	write(tmp_stream, t);
	BOOST_CHECK_EQUAL(tmp_stream.str(), "2005-10-11");
}

BOOST_AUTO_TEST_CASE(checkPionTimeFacetFromString) {
	const std::string str("15:24:31");
	PionDateTime t;
	setFormat("%H:%M:%S");
	fromString(str, t);
	BOOST_CHECK_EQUAL(t.time_of_day().hours(), 15);
	BOOST_CHECK_EQUAL(t.time_of_day().minutes(), 24);
	BOOST_CHECK_EQUAL(t.time_of_day().seconds(), 31);
}

BOOST_AUTO_TEST_CASE(checkPionTimeFacetToString) {
	const PionDateTime t(boost::gregorian::date(2005, 10, 11),
						 boost::posix_time::time_duration(15, 24, 31));
	std::string str;
	setFormat("%H:%M:%S");
	toString(str, t);
	BOOST_CHECK_EQUAL(str, "15:24:31");
}


BOOST_AUTO_TEST_SUITE_END()
