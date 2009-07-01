// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007-2009 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <pion/PionConfig.hpp>
#include <pion/PionPoolAllocator.hpp>
#include <pion/PionBlob.hpp>
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <boost/thread/thread.hpp>
#include <boost/test/unit_test.hpp>

using namespace pion;

class PionBlobTests_F {
public:

	typedef PionBlob<char,PionPoolAllocator<> >	BlobType;

	PionPoolAllocator<> m_alloc;

	PionBlobTests_F() {
	}
	
	virtual ~PionBlobTests_F() {
	}
	
	static void createCopies(BlobType b, std::size_t num_copies) {
		boost::scoped_array<BlobType> blobs (new BlobType[num_copies]);
		for (std::size_t n = 0; n < num_copies; ++n) {
			blobs[n] = b;
		}
	}
};

BOOST_FIXTURE_TEST_SUITE(PionBlobTests_S, PionBlobTests_F)

BOOST_AUTO_TEST_CASE(checkSetAndCompareStringValue) {
	BlobType b;
	BOOST_CHECK(b.empty());
	BOOST_CHECK(b.unique());
	BOOST_CHECK_EQUAL(b.use_count(), 0);
	BOOST_CHECK_EQUAL(b.size(), 0U);

	std::string hello_str("hello");
	std::string goodbye_str("goodbye");

	b.set(m_alloc, hello_str);
	BOOST_CHECK(! b.empty());
	BOOST_CHECK(b.unique());
	BOOST_CHECK_EQUAL(b.use_count(), 1);
	BOOST_CHECK_EQUAL(b.size(), 5U);

	BOOST_CHECK(b == hello_str);
	BOOST_CHECK_EQUAL(hello_str, b.get());
	BOOST_CHECK_EQUAL(memcmp(b.get(), hello_str.c_str(), 5), 0);
	BOOST_CHECK_EQUAL(b.get()[5], '\0');
	BOOST_CHECK(b != goodbye_str);
	BOOST_CHECK(! b.empty());
	BOOST_CHECK_EQUAL(b.size(), hello_str.size());
}

BOOST_AUTO_TEST_CASE(checkSetAndCompareBlobParams) {
	std::string hello_str("hello");
	BlobType::BlobParams p1(m_alloc, hello_str.c_str(), hello_str.size());
	BlobType b1(p1);
	BOOST_CHECK(! b1.empty());
	BOOST_CHECK(b1.unique());
	BOOST_CHECK_EQUAL(b1.use_count(), 1);
	BOOST_CHECK_EQUAL(b1.size(), 5U);
	BOOST_CHECK(b1 == hello_str);

	std::string goodbye_str("goodbye");
	BlobType::BlobParams p2(m_alloc, goodbye_str.c_str(), goodbye_str.size());
	b1.set(p2);
	BOOST_CHECK(! b1.empty());
	BOOST_CHECK(b1.unique());
	BOOST_CHECK_EQUAL(b1.use_count(), 1);
	BOOST_CHECK_EQUAL(b1.size(), 7U);
	BOOST_CHECK(b1 == goodbye_str);
}

BOOST_AUTO_TEST_CASE(checkSetAndCompareTwoBlobs) {
	std::string hello_str("hello");
	std::string goodbye_str("goodbye");
	BlobType b1, b2;

	b1.set(m_alloc, hello_str.c_str(), hello_str.size());
	b2.set(m_alloc, hello_str.c_str(), hello_str.size());
	BOOST_CHECK(b1 == hello_str);
	BOOST_CHECK(b1 != goodbye_str);
	BOOST_CHECK(b2 == hello_str);
	BOOST_CHECK(b2 != goodbye_str);
	BOOST_CHECK(b1 == b2);

	b2.set(m_alloc, goodbye_str.c_str(), goodbye_str.size());
	BOOST_CHECK(b2 == goodbye_str);
	BOOST_CHECK(b2 != hello_str);
	BOOST_CHECK(b1 != b2);
	
	b2 = b1;
	BOOST_CHECK(! b1.unique());
	BOOST_CHECK(! b2.unique());
	BOOST_CHECK_EQUAL(b1.use_count(), 2);
	BOOST_CHECK_EQUAL(b2.use_count(), 2);
	BOOST_CHECK(b2 == hello_str);
	BOOST_CHECK(b2 != goodbye_str);
	BOOST_CHECK(b1 == b2);
}

BOOST_AUTO_TEST_CASE(checkCreateLotsOfCopies) {
	static const std::size_t BLOB_ARRAY_SIZE = 1000;
	BlobType blobs[BLOB_ARRAY_SIZE];

	BlobType b1, b2;
	std::string hello_str("hello");
	std::string goodbye_str("goodbye");
	b1.set(m_alloc, hello_str);
	b2.set(m_alloc, goodbye_str);
	
	for (std::size_t n = 0; n < BLOB_ARRAY_SIZE; ++n) {
		blobs[n] = b1;
	}

	for (std::size_t n = 0; n < BLOB_ARRAY_SIZE; ++n) {
		BOOST_CHECK(blobs[n] == b1);
		blobs[n] = b2;
		BOOST_CHECK(blobs[n] == b2);
	}
}

BOOST_AUTO_TEST_CASE(checkCreateLotsOfCopiesInMultipleThreads) {
	static const std::size_t NUM_THREADS = 10;
	static const std::size_t BLOB_COPIES = 10000;
	
	typedef boost::scoped_ptr<boost::thread>	ThreadPtr;
	boost::scoped_array<ThreadPtr>	threads(new ThreadPtr[NUM_THREADS]);

	BlobType b;
	b.set(m_alloc, "hello");
	BOOST_CHECK(b.unique());

	for (std::size_t n = 0; n < NUM_THREADS; ++n) {
		threads[n].reset(new boost::thread(boost::bind(&PionBlobTests_F::createCopies, b, BLOB_COPIES)));
	}

	BOOST_CHECK(! b.unique());

	// wait for threads to finish to ensure that the allocator doesn't destruct first
	for (std::size_t n = 0; n < NUM_THREADS; ++n) {
		threads[n]->join();
	}

	BOOST_CHECK(b.unique());
}

BOOST_AUTO_TEST_SUITE_END()
