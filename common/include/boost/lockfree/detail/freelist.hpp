//  lock-free freelist
//
//  Copyright (C) 2008, 2009 Tim Blechmann
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  Disclaimer: Not a Boost library.

#ifndef BOOST_LOCKFREE_FREELIST_HPP_INCLUDED
#define BOOST_LOCKFREE_FREELIST_HPP_INCLUDED

#include <boost/lockfree/detail/tagged_ptr.hpp>
#include <boost/lockfree/atomic_int.hpp>
#include <boost/noncopyable.hpp>

#include <boost/mpl/map.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/mpl/at.hpp>
#include <boost/type_traits/is_pod.hpp>

#include <algorithm>            /* for std::min */

namespace boost
{
namespace lockfree
{
namespace detail
{

template <typename T, typename Alloc = std::allocator<T> >
class dummy_freelist:
    private boost::noncopyable,
    private Alloc
{
public:
    T * allocate (void)
    {
        return Alloc::allocate(1);
    }

    void deallocate (T * n)
    {
        Alloc::deallocate(n, 1);
    }
};

} /* namespace detail */

/** simple freelist implementation  */
template <typename T,
          std::size_t maximum_size = 64,
          typename Alloc = std::allocator<T> >
class freelist:
    private detail::dummy_freelist<T, Alloc>
{
    struct freelist_node
    {
        lockfree::tagged_ptr<freelist_node> next;
    };

    typedef lockfree::tagged_ptr<freelist_node> tagged_ptr;

public:
    freelist(void):
        pool_(NULL)
    {}

    explicit freelist(std::size_t initial_nodes):
        pool_(NULL)
    {
        for (std::size_t i = 0; i != std::min(initial_nodes, maximum_size); ++i)
        {
            T * node = detail::dummy_freelist<T, Alloc>::allocate();
            deallocate(node);
        }
    }

    ~freelist(void)
    {
        free_memory_pool();
    }

    T * allocate (void)
    {
        for(;;)
        {
            tagged_ptr old_pool(pool_);

            if (!old_pool)
                return detail::dummy_freelist<T, Alloc>::allocate();

            freelist_node * new_pool = old_pool->next.get_ptr();

            if (pool_.cas(old_pool, new_pool))
            {
                --free_list_size;
                return reinterpret_cast<T*>(old_pool.get_ptr());
            }
        }
    }

    void deallocate (T * n)
    {
        if (free_list_size > maximum_size)
        {
            detail::dummy_freelist<T, Alloc>::deallocate(n);
            return;
        }

        for(;;)
        {
            tagged_ptr old_pool (pool_);

            freelist_node * new_pool = reinterpret_cast<freelist_node*>(n);

            new_pool->next.set_ptr(old_pool.get_ptr());

            if (pool_.cas(old_pool, new_pool))
            {
                --free_list_size;
                return;
            }
        }
    }

private:
    void free_memory_pool(void)
    {
        tagged_ptr current (pool_);

        while (current)
        {
            freelist_node * n = current.get_ptr();
            current.set(current->next);
            detail::dummy_freelist<T, Alloc>::deallocate(reinterpret_cast<T*>(n));
        }
    }

    tagged_ptr pool_;
    atomic_int<unsigned long> free_list_size;
};

template <typename T, typename Alloc = std::allocator<T> >
class caching_freelist:
    private detail::dummy_freelist<T, Alloc>
{
    struct freelist_node
    {
        lockfree::tagged_ptr<freelist_node> next;
    };

    typedef lockfree::tagged_ptr<freelist_node> tagged_ptr;

public:
    caching_freelist(void):
        pool_(NULL)
    {}

    explicit caching_freelist(std::size_t initial_nodes):
        pool_(NULL)
    {
        for (std::size_t i = 0; i != initial_nodes; ++i)
        {
            T * node = detail::dummy_freelist<T, Alloc>::allocate();
            deallocate(node);
        }
    }

    ~caching_freelist(void)
    {
        free_memory_pool();
    }

    T * allocate (void)
    {
        for(;;)
        {
            tagged_ptr old_pool(pool_);

            if (!old_pool)
                return detail::dummy_freelist<T, Alloc>::allocate();

            freelist_node * new_pool = old_pool->next.get_ptr();

            if (pool_.cas(old_pool, new_pool))
                return reinterpret_cast<T*>(old_pool.get_ptr());
        }
    }

    void deallocate (T * n)
    {
        for(;;)
        {
            tagged_ptr old_pool (pool_);

            freelist_node * new_pool = reinterpret_cast<freelist_node*>(n);

            new_pool->next.set_ptr(old_pool.get_ptr());

            if (pool_.cas(old_pool,new_pool))
                return;
        }
    }

private:
    void free_memory_pool(void)
    {
        tagged_ptr current (pool_);

        while (current)
        {
            freelist_node * n = current.get_ptr();
            current.set(current->next);
            detail::dummy_freelist<T, Alloc>::deallocate(reinterpret_cast<T*>(n));
        }
    }

    tagged_ptr pool_;
};

template <typename T, typename Alloc = std::allocator<T> >
class static_freelist:
    private Alloc
{
    struct freelist_node
    {
        lockfree::tagged_ptr<freelist_node> next;
    };

    typedef lockfree::tagged_ptr<freelist_node> tagged_ptr;

public:
    explicit static_freelist(std::size_t max_nodes):
        pool_(NULL), total_nodes(max_nodes)
    {
        chunks = Alloc::allocate(max_nodes);
        for (std::size_t i = 0; i != max_nodes; ++i)
        {
            T * node = chunks + i;
            deallocate(node);
        }
    }

    ~static_freelist(void)
    {
        Alloc::deallocate(chunks, total_nodes);
    }

    T * allocate (void)
    {
        for(;;)
        {
            tagged_ptr old_pool(pool_);

            if (!old_pool)
                return 0;       /* allocation fails */

            freelist_node * new_pool = old_pool->next.get_ptr();

            if (pool_.cas(old_pool, new_pool))
                return reinterpret_cast<T*>(old_pool.get_ptr());
        }
    }

    void deallocate (T * n)
    {
        for(;;)
        {
            tagged_ptr old_pool (pool_);

            freelist_node * new_pool = reinterpret_cast<freelist_node*>(n);

            new_pool->next.set_ptr(old_pool.get_ptr());

            if (pool_.cas(old_pool,new_pool))
                return;
        }
    }

private:
    tagged_ptr pool_;

    const std::size_t total_nodes;
    T* chunks;
};


struct caching_freelist_t {};
struct static_freelist_t {};

namespace detail
{

#if 0
template <typename T, typename Alloc, typename tag>
struct select_freelist
{
private:
    typedef typename Alloc::template rebind<T>::other Allocator;

    typedef typename boost::lockfree::caching_freelist<T, Allocator> cfl;
    typedef typename boost::lockfree::static_freelist<T, Allocator> sfl;

    typedef typename boost::mpl::map<
        boost::mpl::pair < caching_freelist_t, cfl/* typename boost::lockfree::caching_freelist<T, Alloc> */ >,
        boost::mpl::pair < static_freelist_t,  sfl/* typename boost::lockfree::static_freelist<T, Alloc> */ >,
        int
        > freelists;
public:
    typedef typename boost::mpl::at<freelists, tag>::type type;
};
#endif

} /* namespace detail */
} /* namespace lockfree */
} /* namespace boost */

#endif /* BOOST_LOCKFREE_FREELIST_HPP_INCLUDED */
