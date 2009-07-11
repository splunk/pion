// -----------------------------------------------------------------------
// pion-common: a collection of common libraries used by the Pion Platform
// -----------------------------------------------------------------------
// Copyright (C) 2007-2009 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PIONBLOB_HEADER__
#define __PION_PIONBLOB_HEADER__

#include <string>
#include <boost/detail/atomic_count.hpp>
#include <boost/functional/hash.hpp>
#include <pion/PionConfig.hpp>


namespace pion {	// begin namespace pion


///
/// PionBlob: a simple, reference-counting BLOB class 
/// that uses PionPoolAllocator for memory management
///
template <typename CharType, typename AllocType>
class PionBlob {
protected:

	/// structure used to store BLOB metadata; payload starts immediately following this
	struct BlobData {
		/// constructor takes allocator and size (in octets) of BLOB
		BlobData(AllocType& blob_alloc, const std::size_t len) :
			m_alloc_ptr(&blob_alloc), m_len(len), m_copies(0)
		{
			*((CharType*)(this) + sizeof(struct BlobData) + len) = '\0';
		}
		
		/// pointer to the allocator used by the BLOB
		AllocType *	const				m_alloc_ptr;

		/// size of the BLOB, in octets
		const std::size_t				m_len;

		/// number of copies referencing this BLOB
		boost::detail::atomic_count		m_copies;
	};

	/// pointer the the BLOB metadata structure (payload follows the structure)
	BlobData *			m_blob_ptr;


	/**
	 * creates a new BLOB reference object
	 *
	 * @param len size in octets to allocate for the BLOB
	 *
	 * @return BlobData* pointer to the new BLOB data object (with reference incremented)
	 */
	static inline BlobData *create(AllocType& blob_alloc, const std::size_t len) {
		BlobData *blob_ptr = new (blob_alloc.malloc(len+sizeof(struct BlobData)+1))
			BlobData(blob_alloc, len);
		return blob_ptr;
	}

	/**
	 * releases pointer to the BLOB metadata structure, and frees memory if no more references
	 */
	inline void release(void) {
		if (m_blob_ptr) {
			if (m_blob_ptr->m_copies == 0) {
				m_blob_ptr->m_alloc_ptr->free(m_blob_ptr, m_blob_ptr->m_len+sizeof(struct BlobData)+1);
			} else {
				--m_blob_ptr->m_copies;
			}
			m_blob_ptr = NULL;
		}
	}

	/**
	 * grabs & returns reference pointer to this BLOB (increments references)
	 */
	inline BlobData *grab(void) const {
		if (m_blob_ptr) {
			++m_blob_ptr->m_copies;
			return m_blob_ptr;
		} else {
			return NULL;
		}
	}
	
	
public:

	/// data type used to initialize blobs in variants without copy construction
	struct BlobParams {
		/// constructor requires all parameters
		BlobParams(AllocType& blob_alloc, const CharType *ptr, const std::size_t len)
			: m_alloc(blob_alloc), m_ptr(ptr), m_len(len)
		{}
		// data parameters for constructing a PionBlob
		AllocType&			m_alloc;
		const CharType *	m_ptr;
		std::size_t			m_len;
	};

	/// virtual destructor
	virtual ~PionBlob() {
		release();
	}

	/// default constructor
	PionBlob(void) :
		m_blob_ptr(NULL)
	{}

	/**
	 * copy constructor
	 *
	 * @param blob grabs reference from this existing blob
	 */
	PionBlob(const PionBlob& blob) :
		m_blob_ptr(blob.grab())
	{}

	/**
	 * constructs a BLOB using BlobParams
	 *
	 * @param p BlobParams contains all parameters used to initialize the BLOB
	 */
	PionBlob(const BlobParams& p) :
		m_blob_ptr(NULL)
	{
		m_blob_ptr = create(p.m_alloc, p.m_len);
		memcpy(get(), p.m_ptr, p.m_len);
	}

	/**
	 * constructs a BLOB using existing memory buffer
	 *
	 * @param blob_alloc allocator used for memory management
	 * @param ptr pointer to a buffer of memory to copy into the BLOB
	 * @param len size in octets of the memory buffer to copy
	 */
	PionBlob(AllocType& blob_alloc, const CharType* ptr, const std::size_t len) :
		m_blob_ptr(NULL)
	{
		m_blob_ptr = create(blob_alloc, len);
		memcpy(get(), ptr, len);
	}

	/**
	 * constructs a BLOB using existing string
	 *
	 * @param blob_alloc allocator used for memory management
	 * @param str existing std::string object to copy
	 */
	PionBlob(AllocType& blob_alloc, const std::string& str) :
		m_blob_ptr(NULL)
	{
		m_blob_ptr = create(blob_alloc, str.size());
		memcpy(get(), str.c_str(), str.size());
	}

	/**
	 * assignment operator
	 *
	 * @param blob grabs reference from this existing blob
	 *
	 * @return reference to this BLOB
	 */
	inline PionBlob& operator=(const PionBlob& blob) {
		release();
		m_blob_ptr = blob.grab();
		return *this;
	}
	
	/**
	 * assigns BLOB to existing memory buffer via BlobParams
	 *
	 * @param p BlobParams contains all parameters used to initialize the BLOB
	 */
	inline void set(const BlobParams& p) {
		release();
		m_blob_ptr = create(p.m_alloc, p.m_len);
		memcpy(get(), p.m_ptr, p.m_len);
	}

	/**
	 * assigns BLOB to use an existing memory buffer
	 *
	 * @param blob_alloc allocator used for memory management
	 * @param ptr pointer to a buffer of memory to copy into the BLOB
	 * @param len size in octets of the memory buffer to copy
	 */
	inline void set(AllocType& blob_alloc, const CharType* ptr, const std::size_t len) {
		release();
		m_blob_ptr = create(blob_alloc, len);
		memcpy(get(), ptr, len);
	}

	/**
	 * assigns BLOB to use an existing string
	 *
	 * @param blob_alloc allocator used for memory management
	 * @param str existing std::string object to copy
	 */
	inline void set(AllocType& blob_alloc, const std::string& str) {
		release();
		m_blob_ptr = create(blob_alloc, str.size());
		memcpy(get(), str.c_str(), str.size());
	}

	/// returns (non-const) reference to the BLOB payload
	inline CharType *get(void) {
		return (m_blob_ptr ? ((CharType*)m_blob_ptr+sizeof(struct BlobData)) : NULL);
	}

	/// returns (const) reference to the BLOB payload
	inline const CharType *get(void) const {
		return (m_blob_ptr ? ((CharType*)m_blob_ptr+sizeof(struct BlobData)) : NULL);
	}

	/// returns size of the BLOB in octets
	inline std::size_t size(void) const {
		return (m_blob_ptr ? (m_blob_ptr->m_len) : 0);
	}

	/// returns length of the BLOB in octets (alias for size())
	inline std::size_t length(void) const {
		return size();
	}
	
	/// returns true if the BLOB is empty (undefined or size == 0)
	inline bool empty(void) const {
		return (m_blob_ptr == NULL || m_blob_ptr->m_len == 0);
	}
	
	/// returns the number of reference to this BLOB (or 0 if this is null)
	inline long use_count(void) const {
		return (m_blob_ptr == NULL ? 0 : m_blob_ptr->m_copies + 1);
	}

	/// returns true if this is a unique instance or if this is null
	inline bool unique(void) const {
		return (m_blob_ptr == NULL || m_blob_ptr->m_copies == 0);
	}
	
	/// alias for release() -> switch to empty state
	inline void clear(void) { release(); }

	/// alias for release() -> switch to empty state
	inline void reset(void) { release(); }

	/// returns true if str is equal to this (BLOB matches string)
	inline bool operator==(const PionBlob& blob) const {
		if (size() != blob.size())
			return false;
		return (empty() || m_blob_ptr==blob.m_blob_ptr || memcmp(get(), blob.get(), m_blob_ptr->m_len)==0);
	}

	/// returns true if str is equal to this (BLOB matches string)
	inline bool operator==(const std::string& str) const {
		if (size() != str.size())
			return false;
		return (empty() || memcmp(get(), str.c_str(), m_blob_ptr->m_len)==0);
	}

	/// returns true if blob is not equal to this (two BLOBs do not match)
	inline bool operator!=(const PionBlob& blob) const {
		return ! (this->operator==(blob));
	}

	/// returns true if str is not equal to this (BLOB does not match string)
	inline bool operator!=(const std::string& str) const {
		return ! (this->operator==(str));
	}

	/// returns true if blob is less than this
	inline bool operator<(const PionBlob& blob) const {
		const std::size_t len = (size() < blob.size() ? size() : blob.size());
		if (len > 0) {
			const int val = memcmp(get(), blob.get(), len);
			if (val < 0)
				return true;
			if (val > 0)
				return false;
		}
		return (size() < blob.size());
	}
		
	/// returns true if blob is greater than this
	inline bool operator>(const PionBlob& blob) const {
		const std::size_t len = (size() < blob.size() ? size() : blob.size());
		if (len > 0) {
			const int val = memcmp(get(), blob.get(), len);
			if (val > 0)
				return true;
			if (val < 0)
				return false;
		}
		return (size() > blob.size());
	}

	/// returns true if str is less than this
	inline bool operator<(const std::string& str) const {
		const std::size_t len = (size() < str.size() ? size() : str.size());
		if (len > 0) {
			const int val = memcmp(get(), str.c_str(), len);
			if (val < 0)
				return true;
			if (val > 0)
				return false;
		}
		return (size() < str.size());
	}

	/// returns true if str is greater than this
	inline bool operator>(const std::string& str) const {
		const std::size_t len = (size() < str.size() ? size() : str.size());
		if (len > 0) {
			const int val = memcmp(get(), str.c_str(), len);
			if (val > 0)
				return true;
			if (val < 0)
				return false;
		}
		return (size() > str.size());
	}
};


/// returns hash value for a PionBlob object (used by boost::hash_map)
template <typename CharType, typename AllocType>
static inline std::size_t hash_value(const PionBlob<CharType,AllocType>& blob) {
	return (blob.empty() ? 0 : boost::hash_range(blob.get(), blob.get() + blob.size()));
}


/// optimized hash function object for PionBlob objects which contain PionId string representations (bb49b9ca-e733-47c0-9a26-0f8f53ea1660)
struct HashPionIdBlob {
	/// helper for hex->int conversion
	inline unsigned long getValue(unsigned char c) const {
		unsigned long result;
		switch(c) {
		case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
			result = (c - 48);
			break;
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
			result = (c - 87);
			break;
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
			result = (c - 55);
			break;
		default:
			result = 0;
			break;
		}
		return result;
	}

	/// returns hash value for the blob provided
	template <typename CharType, typename AllocType>
	inline std::size_t operator()(const PionBlob<CharType,AllocType>& blob) const {
		if (blob.size() != 36)	// sanity check
			return hash_value(blob);

		const char * const data = blob.get();
		unsigned long n;
		std::size_t seed = 0;

		// calculate first ulong value
		n = (getValue(data[0]) << 28);
		n |= (getValue(data[1]) << 24);
		n |= (getValue(data[2]) << 20);
		n |= (getValue(data[3]) << 16);
		n |= (getValue(data[4]) << 12);
		n |= (getValue(data[5]) << 8);
		n |= (getValue(data[6]) << 4);
		n |= getValue(data[7]);
		boost::hash_combine(seed, n);

		// calculate second ulong value
		n = (getValue(data[9]) << 28);
		n |= (getValue(data[10]) << 24);
		n |= (getValue(data[11]) << 20);
		n |= (getValue(data[12]) << 16);
		n |= (getValue(data[14]) << 12);
		n |= (getValue(data[15]) << 8);
		n |= (getValue(data[16]) << 4);
		n |= getValue(data[17]);
		boost::hash_combine(seed, n);
		
		// calculate third ulong value
		n = (getValue(data[19]) << 28);
		n |= (getValue(data[20]) << 24);
		n |= (getValue(data[21]) << 20);
		n |= (getValue(data[22]) << 16);
		n |= (getValue(data[24]) << 12);
		n |= (getValue(data[25]) << 8);
		n |= (getValue(data[26]) << 4);
		n |= getValue(data[27]);
		boost::hash_combine(seed, n);

		// calculate third ulong value
		n = (getValue(data[28]) << 28);
		n |= (getValue(data[29]) << 24);
		n |= (getValue(data[30]) << 20);
		n |= (getValue(data[31]) << 16);
		n |= (getValue(data[32]) << 12);
		n |= (getValue(data[33]) << 8);
		n |= (getValue(data[34]) << 4);
		n |= getValue(data[35]);
		boost::hash_combine(seed, n);

		return seed;
	}
};


}	// end namespace pion

#endif
