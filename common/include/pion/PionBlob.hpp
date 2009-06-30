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

#include <pion/PionConfig.hpp>
#include <boost/detail/atomic_count.hpp>


namespace pion {	// begin namespace pion


///
/// PionBlob: simple reference-counting BLOB class that uses PionAllocator for memory management
///
template <typename CharType, typename AllocType>
class PionBlob {
protected:

	/// structure used to store BLOB metadata; payload starts immediately following this
	struct BlobData {
		/// constructor takes size (in octets) of BLOB
		BlobData(const std::size_t len) : m_len(len), m_ref(0) {}

		/// size of the BLOB, in octets
		std::size_t						m_len;

		/// number of references to this BLOB
		boost::detail::atomic_count		m_ref;
	};

	/// pointer the the BLOB metadata structure (payload follows the structure)
	BlobData *			m_blob_ptr;

	/// pointer to the allocator used by the BLOB
	AllocType *			m_alloc_ptr;


	/**
	 * creates a new BLOB reference object
	 *
	 * @param len size in octets to allocate for the BLOB
	 */
	inline void create(const std::size_t len) {
		release();
		m_blob_ptr = new (m_alloc_ptr->malloc(len+sizeof(struct BlobData)+1)) BlobData(len);
		++m_blob_ptr->m_ref;
		get()[len] = '\0';
	}

	/**
	 * releases pointer to the BLOB metadata structure, and frees memory if no more references
	 */
	inline void release(void) {
		if (m_blob_ptr) {
			if (--m_blob_ptr->m_ref == 0) {
				m_alloc_ptr->free(m_blob_ptr, m_blob_ptr->m_len+sizeof(struct BlobData)+1);
			}
			m_blob_ptr = NULL;
		}
	}

	/**
	 * grabs & returns reference pointer to this BLOB (increments references)
	 */
	inline BlobData *grab(void) const {
		if (m_blob_ptr) {
			++m_blob_ptr->m_ref;
			return m_blob_ptr;
		} else {
			return NULL;
		}
	}
	
	
public:

	/// virtual destructor
	virtual ~PionBlob() {
		release();
	}

	/// default constructor (do not call create!)
	PionBlob(void) :
		m_blob_ptr(NULL), m_alloc_ptr(NULL)
	{}

	/**
	 * copy constructor
	 *
	 * @param blob grabs reference from this existing blob
	 */
	PionBlob(const PionBlob& blob) :
		m_blob_ptr(blob.grab()), m_alloc_ptr(blob.m_alloc_ptr)
	{}

	/**
	 * constructs a BLOB using existing memory buffer
	 *
	 * @param blob_alloc allocator used for memory management
	 * @param ptr pointer to a buffer of memory to copy into the BLOB
	 * @param len size in octets of the memory buffer to copy
	 */
	PionBlob(AllocType& blob_alloc, const CharType* ptr, const std::size_t len) :
		m_blob_ptr(NULL), m_alloc_ptr(&blob_alloc)
	{
		create(len);
		memcpy(get(), ptr, len);
	}

	/**
	 * assigns BLOB to use an existing memory buffer
	 *
	 * @param blob_alloc allocator used for memory management
	 * @param ptr pointer to a buffer of memory to copy into the BLOB
	 * @param len size in octets of the memory buffer to copy
	 */
	void set(AllocType& blob_alloc, const CharType* ptr, const std::size_t len) {
		release();
		m_alloc_ptr = &blob_alloc;
		create(len);
		memcpy(get(), ptr, len);
	}

	/**
	 * assignment operator
	 *
	 * @param blob grabs reference from this existing blob
	 */
	PionBlob& operator=(const PionBlob& blob) {
		release();
		m_blob_ptr = blob.grab();
		m_alloc_ptr = blob.m_alloc_ptr;
		return *this;
	}
	
	/**
	 * comparison operator (equals)
	 *
	 * @param blob object to compare this to
	 *
	 * @return true if the BLOB matches this one
	 */
	inline bool operator==(const PionBlob& blob) const {
		if (m_blob_ptr == blob.m_blob_ptr)
			return true;
		if (m_blob_ptr == NULL || m_blob_ptr->m_len == 0)
			return (blob.m_blob_ptr == NULL || blob.m_blob_ptr->m_len == 0);
		if (blob.m_blob_ptr == NULL || blob.m_blob_ptr->m_len == 0)
			return false;
		if (m_blob_ptr->m_len != blob.m_blob_ptr->m_len)
			return false;
		return memcmp(get(), blob.get(), m_blob_ptr->m_len) == 0;
	}

	/**
	 * comparison operator (not equals)
	 *
	 * @param blob object to compare this to
	 *
	 * @return true if the BLOB does not match this one
	 */
	inline bool operator!=(const PionBlob& blob) const {
		return ! (this->operator==(blob));
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
};

}	// end namespace pion

#endif
