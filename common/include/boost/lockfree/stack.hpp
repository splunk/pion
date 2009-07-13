//  Copyright (C) 2008 Tim Blechmann
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  Disclaimer: Not a Boost library.

#ifndef BOOST_LOCKFREE_STACK_HPP_INCLUDED
#define BOOST_LOCKFREE_STACK_HPP_INCLUDED

#include <boost/checked_delete.hpp>

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_base_of.hpp>

#include <boost/lockfree/detail/tagged_ptr.hpp>
#include <boost/lockfree/detail/freelist.hpp>
#include <boost/noncopyable.hpp>


namespace boost
{
namespace lockfree
{
template <typename T,
          typename freelist_t = caching_freelist_t,
          typename Alloc = std::allocator<T>
          >
class stack:
    boost::noncopyable
{
    struct node
    {
        node(T const & v):
            v(v)
        {}

        tagged_ptr<node> next;
        T v;
    };

    typedef tagged_ptr<node> ptr_type;

    typedef typename Alloc::template rebind<node>::other node_allocator;
/*     typedef typename detail::select_freelist<node, node_allocator, freelist_t>::type pool_t; */

    typedef typename boost::mpl::if_<boost::is_same<freelist_t, caching_freelist_t>,
                                     caching_freelist<node, node_allocator>,
                                     static_freelist<node, node_allocator>
                                     >::type pool_t;

public:
    stack(void):
        tos(NULL), pool(128)
    {}

    explicit stack(std::size_t n):
        tos(NULL), pool(n)
    {}

    bool push(T const & v)
    {
        node * newnode = alloc_node(v);

        if (newnode == 0)
            return false;

        ptr_type old_tos;
        do
        {
            old_tos.set(tos);
            newnode->next.set_ptr(old_tos.get_ptr());
        }
        while (!tos.cas(old_tos, newnode));

        return true;
    }

    bool pop(T * ret)
    {
        for (;;)
        {
            ptr_type old_tos;
            old_tos.set(tos);

            if (!old_tos)
                return false;

            node * new_tos = old_tos->next.get_ptr();

            if (tos.cas(old_tos, new_tos))
            {
                *ret = old_tos->v;
                dealloc_node(old_tos.get_ptr());
                return true;
            }
        }
    }

    bool empty(void) const
    {
        return tos == NULL;
    }

private:
    node * alloc_node(T const & t)
    {
        node * chunk = pool.allocate();
        new(chunk) node(t);
        return chunk;
    }

    void dealloc_node(node * n)
    {
        n->~node();
        pool.deallocate(n);
    }

    ptr_type tos;
    pool_t pool;
};


} /* namespace lockfree */
} /* namespace boost */

#endif /* BOOST_LOCKFREE_STACK_HPP_INCLUDED */
