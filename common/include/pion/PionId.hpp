// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007-2009 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PIONID_HEADER__
#define __PION_PIONID_HEADER__

#include <string>
#include <limits>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <boost/functional/hash.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <pion/PionConfig.hpp>

namespace pion {	// begin namespace pion


///
/// PionId: a random-number based universally unique identifier (UUID v4)
///
class PionId {
public:

	/// data type for iterating PionId byte values
	typedef unsigned char *			iterator;
	
	/// const data type for iterating PionId byte values
	typedef const unsigned char *	const_iterator;
	
	enum {
		PION_ID_DATA_BYTES = 16,			//< total number of data bytes
		PION_ID_HEX_BYTES = 16 * 2 + 4		//< number of bytes in hexadecimal representation
	};

	/// class may be extended (virtual destructor)
	virtual ~PionId() {}

	/// default constructor
	PionId(void) {
		typedef boost::mt19937 gen_type;
		typedef boost::uniform_int<unsigned long> dist_type;
		typedef boost::variate_generator<gen_type,dist_type> die_type;
		gen_type rng_gen(PionId::make_seed());
		dist_type rng_dist((std::numeric_limits<unsigned long>::min)(), (std::numeric_limits<unsigned long>::max)());
		die_type rng_die(rng_gen, rng_dist);
		generate(m_data, rng_die);
	}

	/// construction using a string representation (bb49b9ca-e733-47c0-9a26-0f8f53ea1660)
	explicit PionId(const std::string& str) {
		from_string(str.c_str());
	}
		
	/// construction using a null-terminated c-style string (bb49b9ca-e733-47c0-9a26-0f8f53ea1660)
	explicit PionId(const char *str) {
		from_string(str);
	}

	/// construction using an existing random number generator
	template<typename base_generator_type, typename distribution_type>
	explicit PionId(boost::variate_generator<base_generator_type, distribution_type>& rng) {
		generate(m_data, rng);
	}

	/// copy constructor
	PionId(const PionId& id) {
		memcpy(m_data, id.m_data, PION_ID_DATA_BYTES);
	}
	
	/// assignment operator
	PionId& operator=(const PionId& id) {
		memcpy(m_data, id.m_data, PION_ID_DATA_BYTES);
		return *this;
	}
	
	/// returns id value at byte offset
	inline unsigned char operator[](const std::size_t n) const {
		return m_data[n];
	}
	
	/// returns true if id equals this
	inline bool operator==(const PionId& id) const {
		return (memcmp(m_data, id.m_data, PION_ID_DATA_BYTES) == 0);
	}

	/// returns true if id does not equal this
	inline bool operator!=(const PionId& id) const {
		return (memcmp(m_data, id.m_data, PION_ID_DATA_BYTES) != 0);
	}

	/// returns true if id is less than this
	inline bool operator<(const PionId& id) const {
		return (memcmp(m_data, id.m_data, PION_ID_DATA_BYTES) < 0);
	}
		
	/// returns true if id is greater than this
	inline bool operator>(const PionId& id) const {
		return (memcmp(m_data, id.m_data, PION_ID_DATA_BYTES) > 0);
	}
	
	/// returns the beginning iterator
	inline iterator begin(void) { return m_data; }
	
	/// returns the ending iterator
	inline iterator end(void) { return m_data + PION_ID_DATA_BYTES; }
	
	/// returns the beginning iterator (const)
	inline const_iterator begin(void) const { return m_data; }

	/// returns the ending iterator (const)
	inline const_iterator end(void) const { return m_data + PION_ID_DATA_BYTES; }

	/// returns hexadecimal representation as a string (bb49b9ca-e733-47c0-9a26-0f8f53ea1660)
	inline std::string to_string(void) const {
		std::string hex_str;
		static const char hex[] = "0123456789abcdef";
		for (std::size_t i = 0; i < PION_ID_DATA_BYTES; i++) {
			hex_str += hex[m_data[i] >> 4];
			hex_str += hex[m_data[i] & 0x0f];
			if (i == 3 || i == 5 || i == 7 || i == 9)
				hex_str += '-';
		}
		return hex_str;
	}

	/// sets the data value based upon a null-terminated string representation (bb49b9ca-e733-47c0-9a26-0f8f53ea1660)
	void from_string(const char *str) {
		std::size_t data_pos = 0;
		char buf[3];
		buf[2] = '\0';
		while (*str != '\0' && data_pos < PION_ID_DATA_BYTES) {
			if (isxdigit(*str)) {
				buf[0] = *str;
				if (*(++str) == '\0' || !isxdigit(*str))	// sanity check
					break;
				buf[1] = *str;
				m_data[data_pos++] = strtoul(buf, NULL, 16);
			}
			++str;
		}
	}

	/// return a seed value for random number generators
	static inline boost::uint64_t make_seed(void) {
		// this could be much better...  and is probably far too trivial...
		boost::uint64_t seed = boost::posix_time::microsec_clock::local_time().time_of_day().total_microseconds();
		seed += (time(NULL) * 1000000);
		return seed;
	}


protected:

	/**
	 * generates a new data value using an existing random number generator
	 *
	 * @param data pointer to a data buffer that is PION_ID_DATA_BYTES in size
	 * @param rng initialized random number generator
	 */
	template<typename base_generator_type, typename distribution_type>
	static inline void generate(unsigned char *data, boost::variate_generator<base_generator_type, distribution_type>& rng) {
		// Note: this code is adapted from the Boost UUID library, (c) 2006 Andy Tompkins
		for (std::size_t i = 0; i < PION_ID_DATA_BYTES; i += sizeof(unsigned long)) {
            *reinterpret_cast<unsigned long*>(&data[i]) = rng();
		}

        // set variant
        // should be 0b10xxxxxx
        data[8] &= 0xBF;
        data[8] |= 0x80;

        // set version
        // should be 0b0100xxxx
        data[6] &= 0x4F; //0b01001111
        data[6] |= 0x40; //0b01000000
	}
	
	/// sequence of bytes representing the unique identifier
	unsigned char	m_data[PION_ID_DATA_BYTES];
};


/// returns hash value for a PionId object (used by boost::hash_map)
static inline std::size_t hash_value(const PionId& id) {
	std::size_t seed = 0;
	const unsigned char * data = id.begin();
	const unsigned char * const end = id.end();
	while (data < end) {
		boost::hash_combine(seed, *reinterpret_cast<const unsigned long*>(data));
		data += sizeof(unsigned long);
	}
	return seed;
}


///
/// PionIdGeneratorBase: class used to generated new PionId's
///
template <typename BaseGeneratorType>
class PionIdGeneratorBase {
public:

	/// make dynamic type for base generator available
	typedef BaseGeneratorType	base_generator_type;

	/// random number distribution type
	typedef boost::uniform_int<unsigned long>	distribution_type;

	/// random number generator type
	typedef boost::variate_generator<base_generator_type, distribution_type>	gen_type;


	/// class may be extended (virtual destructor)
	virtual ~PionIdGeneratorBase() {}

	/// default constructor
	PionIdGeneratorBase(void)
		: m_random_gen(PionId::make_seed()),
		m_random_dist((std::numeric_limits<unsigned long>::min)(), (std::numeric_limits<unsigned long>::max)()),
		m_random_die(m_random_gen, m_random_dist)
	{}

	/// returns a newly generated PionId object
	inline PionId operator()(void) { return PionId(m_random_die); }
	
	/// return random number generator
	inline gen_type& getRNG(void) { return m_random_die; }

	/// return random number generator
	inline unsigned long getNumber(void) { return m_random_die(); }


protected:

	/// random number generator
	base_generator_type				m_random_gen;

	/// random number distribution
	distribution_type				m_random_dist;

	/// random number die
	gen_type						m_random_die;
};


/// data type for the default PionId generator class
typedef PionIdGeneratorBase<boost::mt19937>	PionIdGenerator;


}	// end namespace pion

#endif
