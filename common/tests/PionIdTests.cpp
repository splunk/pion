// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007-2009 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <cctype>
#include <pion/PionConfig.hpp>
#include <pion/PionUnitTestDefs.hpp>
#include <pion/PionId.hpp>
#include <pion/PionHashMap.hpp>
#include <boost/test/unit_test.hpp>

using namespace pion;

class PionIdTests_F {
public:
	PionIdTests_F() {}
	virtual ~PionIdTests_F() {}
	
	void checkPionId(const PionId& id) {
		std::string id_str(id.to_string());
		
		// check size of id string
		BOOST_CHECK_EQUAL(id_str.size(), static_cast<std::size_t>(PionId::PION_ID_HEX_BYTES));
		
		// check hex digits for all non-dash positions in string
		// also check dash positions
		for (std::size_t i = 0; i < PionId::PION_ID_HEX_BYTES; ++i) {
			if (i != 8 && i != 13 && i != 18 && i != 23) {
				BOOST_CHECK(isxdigit(id_str[i]));
			} else {
				BOOST_CHECK_EQUAL(id_str[i], '-');
			}
		}
		
		// check version in id string
		BOOST_CHECK_EQUAL(id_str[14], '4'); // v4
		
		// check y position (8, 9, a or b)
		BOOST_CHECK(id_str[19] == '8' || id_str[19] == '9' || id_str[19] == 'a' || id_str[19] == 'b');
	}
	
	void checkEqual(const PionId& id1, const PionId& id2) {
		BOOST_CHECK(id1 == id2);
		BOOST_CHECK( !(id1 != id2) );
		BOOST_CHECK( !(id1 < id2) );
		BOOST_CHECK( !(id1 > id2) );
	}

	void checkNotEqual(const PionId& id1, const PionId& id2) {
		BOOST_CHECK(id1 != id2);
		BOOST_CHECK( !(id1 == id2) );
	}
};
	
BOOST_FIXTURE_TEST_SUITE(PionIdTests_S, PionIdTests_F)

BOOST_AUTO_TEST_CASE(checkDefaultConstructor) {
	PionId id;
	checkPionId(id);
}
	
BOOST_AUTO_TEST_CASE(checkCopyConstructor) {
	PionId id1;
	PionId id2(id1);
	checkPionId(id1);
	checkPionId(id2);
	checkEqual(id1, id2);
}
	
BOOST_AUTO_TEST_CASE(checkAssignmentOperator) {
	PionId id1;
	PionId id2;
	id2 = id1;
	checkPionId(id1);
	checkPionId(id2);
	checkEqual(id1, id2);
}

BOOST_AUTO_TEST_CASE(checkCreateMultipleIds) {
	PionId id1;
	PionId id2;
	PionId id3;
	checkPionId(id1);
	checkPionId(id2);
	checkPionId(id3);
	checkNotEqual(id1, id2);
	checkNotEqual(id1, id3);
	checkNotEqual(id2, id3);
}
	
BOOST_AUTO_TEST_CASE(checkCreateMultipleIdsWithGenerator) {
	PionIdGenerator id_gen;
	PionId id1 = id_gen();
	PionId id2(id_gen());
	PionId id3(id_gen());
	checkPionId(id1);
	checkPionId(id2);
	checkPionId(id3);
	checkNotEqual(id1, id2);
	checkNotEqual(id1, id3);
	checkNotEqual(id2, id3);
}
	
BOOST_AUTO_TEST_CASE(checkCreateFromString) {
	std::string str1("bb49b9ca-e733-47c0-9a26-0f8f53ea1660");
	std::string str2("c4b486f3-d13f-4cb9-9b24-5a1050a51dbf");
	PionId id1(str1);
	PionId id2(str2.c_str());
	checkPionId(id1);
	checkPionId(id2);
	checkNotEqual(id1, id2);
	BOOST_CHECK_EQUAL(id1.to_string(), str1);
	BOOST_CHECK_EQUAL(id2.to_string(), str2);
}

BOOST_AUTO_TEST_CASE(checkHashValues) {
	PionId id1;
	PionId id2;
	PionId id3;
	checkPionId(id1);
	checkPionId(id2);
	checkPionId(id3);
	checkNotEqual(id1, id2);
	checkNotEqual(id1, id3);
	checkNotEqual(id2, id3);
	std::size_t seed1 = hash_value(id1);
	std::size_t seed2 = hash_value(id2);
	std::size_t seed3 = hash_value(id3);
	BOOST_CHECK_NE(seed1, seed2);
	BOOST_CHECK_NE(seed1, seed3);
	BOOST_CHECK_NE(seed2, seed3);
}
	
BOOST_AUTO_TEST_CASE(checkPionIdHashMap) {
	typedef PION_HASH_MAP<PionId, int, PION_HASH(PionId) >	PionIdHashMap;
	PionIdHashMap id_map;
	PionId id1;
	PionId id2;
	PionId id3;
	id_map.insert(std::make_pair(id1, 1));
	id_map.insert(std::make_pair(id2, 2));
	id_map.insert(std::make_pair(id3, 3));
	BOOST_CHECK_EQUAL(id_map[id1], 1);
	BOOST_CHECK_EQUAL(id_map[id2], 2);
	BOOST_CHECK_EQUAL(id_map[id3], 3);
}
	
BOOST_AUTO_TEST_SUITE_END()
