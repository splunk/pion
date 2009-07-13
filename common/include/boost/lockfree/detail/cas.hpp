//  Copyright (C) 2007, 2008, 2009 Tim Blechmann & Thomas Grill
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  Disclaimer: Not a Boost library.

#ifndef BOOST_LOCKFREE_CAS_HPP_INCLUDED
#define BOOST_LOCKFREE_CAS_HPP_INCLUDED

#include <boost/lockfree/detail/prefix.hpp>
#include <boost/detail/lightweight_mutex.hpp>
#include <boost/static_assert.hpp>

#include <boost/cstdint.hpp>

namespace boost
{
namespace lockfree
{

inline void memory_barrier()
{
#if defined(__GNUC__) && ( (__GNUC__ > 4) || ((__GNUC__ >= 4) && (__GNUC_MINOR__ >= 1)) ) || defined(__INTEL_COMPILER)
    __sync_synchronize();
#elif defined(_MSC_VER) && (_MSC_VER >= 1300)
    _ReadWriteBarrier();
#elif defined(__APPLE__)
    OSMemoryBarrier();
#elif defined(AO_HAVE_nop_full)
    AO_nop_full();
#else
#   warning "no memory barrier implemented for this platform"
#endif
}

template <typename C>
inline bool atomic_cas_emulation(volatile C * addr, C old, C nw)
{
    static boost::detail::lightweight_mutex guard;
    boost::detail::lightweight_mutex::scoped_lock lock(guard);

    if (*addr == old)
    {
        *addr = nw;
        return true;
    }
    else
        return false;
}

using boost::uint32_t;
using boost::uint64_t;

inline bool atomic_cas32(volatile uint32_t *  addr, uint32_t old, uint32_t nw)
{
#if defined(__GNUC__) && ( (__GNUC__ > 4) || ((__GNUC__ >= 4) && (__GNUC_MINOR__ >= 1)) ) || defined(__INTEL_COMPILER)
    return __sync_bool_compare_and_swap(addr, old, nw);
#else
    return boost::interprocess::detail::atomic_cas32(addr, old, nw) == old;
#endif
}

inline bool atomic_cas64(volatile uint64_t * addr, uint64_t old, uint64_t nw)
{
#if defined(__GNUC__) && ( (__GNUC__ > 4) || ((__GNUC__ >= 4) && (__GNUC_MINOR__ >= 1)) ) || defined(__INTEL_COMPILER)
    return __sync_bool_compare_and_swap(addr, old, nw);
#elif defined(_M_IX86)
    return InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(addr),
                                      reinterpret_cast<LONG>(nw),
                                      reinterpret_cast<LONG>(old)) == old;
#elif defined(_M_X64)
    return InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(addr),
                                      reinterpret_cast<LONG>(nw),
                                      reinterpret_cast<LONG>(old)) == old;
#else
#warning ("blocking CAS emulation")
    return atomic_cas_emulation(addr, old, nw);
#endif
}

template <class C>
inline bool atomic_cas(volatile C * addr, C old, C nw)
{
    if (sizeof(C) == 4)
        return atomic_cas32((volatile uint32_t *)addr, (uint32_t)old, (uint32_t)nw);
    else if (sizeof(C) == 8)
        return atomic_cas64((volatile uint64_t *)addr, (uint64_t)old, (uint64_t)nw);
    else
        return atomic_cas_emulation(addr, old, nw);
}


template <class C, class D, class E>
inline bool atomic_cas2(volatile C * addr, D old1, E old2, D new1, E new2)
{
#if defined(__GNUC__) && ((__GNUC__ >  4) || ( (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 2) ) ) && defined(__i386__) && \
    (defined(__i686__) || defined(__pentiumpro__) || defined(__nocona__ ) || \
     defined (__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8))

    struct packed_c
    {
        D d;
        E e;
    };

    union cu
    {
        packed_c c;
        long long l;
    };

    cu old;
    old.c.d = old1;
    old.c.e = old2;

    cu nw;
    nw.c.d = new1;
    nw.c.e = new2;

    return __sync_bool_compare_and_swap_8(reinterpret_cast<volatile long long*>(addr),
                                          old.l,
                                          nw.l);
#elif defined(_M_IX86)
    bool ok;
    __asm {
        mov eax,[old1]
            mov edx,[old2]
            mov ebx,[new1]
            mov ecx,[new2]
            mov edi,[addr]
            lock cmpxchg8b [edi]
            setz [ok]
            }
    return ok;
#elif defined(__GNUC__) && (defined(__i686__) || defined(__pentiumpro__) || defined(__nocona__ ))
    char result;
#ifndef __PIC__
    __asm__ __volatile__("lock; cmpxchg8b %0; setz %1"
                         : "=m"(*addr), "=q"(result)
                         : "m"(*addr), "d" (old1), "a" (old2),
                           "c" (new1), "b" (new2) : "memory");
#else
    __asm__ __volatile__("push %%ebx; movl %6,%%ebx; lock; cmpxchg8b %0; setz %1; pop %%ebx"
                         : "=m"(*addr), "=q"(result)
                         : "m"(*addr), "d" (old1), "a" (old2),
                           "c" (new1), "m" (new2) : "memory");
#endif
    return result != 0;
#elif defined(AO_HAVE_double_compare_and_swap_full)
    if (sizeof(D) != sizeof(AO_t) || sizeof(E) != sizeof(AO_t)) {
        assert(false);
        return false;
    }

    return AO_compare_double_and_swap_double_full(
        reinterpret_cast<volatile AO_double_t*>(addr),
        static_cast<AO_t>(old2),
        reinterpret_cast<AO_t>(old1),
        static_cast<AO_t>(new2),
        reinterpret_cast<AO_t>(new1)
        );

#elif defined(__GNUC__) && defined(__x86_64__) &&                       \
    ( __GCC_HAVE_SYNC_COMPARE_AND_SWAP_16 ) ||                          \
    ( (__GNUC__ >  4) || ( (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 2) ) && defined(__nocona__ ))

    struct packed_c
    {
        long d;
        long e;
    };

    typedef int TItype __attribute__ ((mode (TI)));

    BOOST_STATIC_ASSERT(sizeof(packed_c) == sizeof(TItype));

    union cu
    {
        packed_c c;
        TItype l;
    };

    cu old;
    old.c.d = (long)old1;
    old.c.e = (long)old2;

    cu nw;
    nw.c.d = (long)new1;
    nw.c.e = (long)new2;

    return __sync_bool_compare_and_swap_16(reinterpret_cast<volatile TItype*>(addr),
                                           old.l,
                                           nw.l);

#elif defined(__GNUC__) && defined(__x86_64__)
    /* handcoded asm, will crash on early amd processors */
    char result;
    __asm__ __volatile__("lock; cmpxchg16b %0; setz %1"
                         : "=m"(*addr), "=q"(result)
                         : "m"(*addr), "d" (old2), "a" (old1),
                           "c" (new2), "b" (new1) : "memory");
    return result != 0;
#else

#ifdef _MSC_VER
#pragma message ("blocking CAS2 emulation")
#else
#warning ("blocking CAS2 emulation")
#endif

    struct packed_c
    {
        D d;
        E e;
    };

    volatile packed_c * packed_addr = reinterpret_cast<volatile packed_c*>(addr);

    static boost::detail::lightweight_mutex guard;
    boost::detail::lightweight_mutex::scoped_lock lock(guard);

    if (packed_addr->d == old1 &&
        packed_addr->e == old2)
    {
        packed_addr->d = new1;
        packed_addr->e = new2;
        return true;
    }
    else
        return false;
#endif
}

} /* namespace lockfree */
} /* namespace boost */

#endif /* BOOST_LOCKFREE_CAS_HPP_INCLUDED */
