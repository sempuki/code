#ifndef _LINEAR_ALLOCATOR_HPP_
#define _LINEAR_ALLOCATOR_HPP_

#include <memory/stateful_allocator_base.hpp>

namespace sequia
{
    namespace memory
    {
        //-------------------------------------------------------------------------
        // Does linear first-fit search of allocation descriptor vector
        // Intended for a small number of variable-size allocations

        template <typename T>
        class linear_allocator : public stateful_allocator_base<T>
        {
            public:
                typedef stateful_allocator_base<T>  parent_type;
                DECLARE_INHERITED_ALLOCATOR_TYPES_ (parent_type);

                typedef identity_allocator<size_type> descriptor_allocator;
                typedef std::vector<size_type, descriptor_allocator> descriptor_list;

                static constexpr size_type freebit = core::one << ((sizeof(size_type) * 8) - 1);
                static constexpr size_type areabits = ~freebit;

                template <typename U> 
                struct rebind { typedef linear_allocator<U> other; };

                template <typename U> 
                linear_allocator (linear_allocator<U> const &r);
                linear_allocator (pointer pitems, size_type nitems, size_type *pallocs, size_type nallocs);

                size_type max_size () const;
                pointer allocate (size_type num, const void* = 0);
                void deallocate (pointer ptr, size_type num);

            private:
                using parent_type::size;
                using parent_type::mem;

                size_type       nfree_;
                descriptor_list list_;
        };

        //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
        // Constructor

        template <typename T>
        linear_allocator<T>::linear_allocator 
            (pointer pitems, size_type nitems, size_type *pallocs, size_type nallocs) : 
                parent_type (pitems, nitems), 
                nfree_ (nitems), 
                list_ (descriptor_allocator (pallocs, nallocs))
        {
            list_.push_back (nitems | freebit);
        }

        // Copy Constructor

        template <typename T>
        template <typename U> 
        linear_allocator<T>::linear_allocator (linear_allocator<U> const &r) : 
            parent_type (r), 
            nfree_ (r.nfree_), 
            list_ (r.list_)
        {}

        // Capacity

        template <typename T>
        auto linear_allocator<T>::max_size () const -> size_type 
        {
            return nfree_;
        }

        // Allocate

        template <typename T>
        auto linear_allocator<T>::allocate (size_type num, const void*) -> pointer 
        {
            ASSERTF (num < freebit, "allocation size impinges on free bit");

            nfree_ -= num;

            pointer ptr = 0;
            size_type free, area;
            typename descriptor_list::iterator descr = std::begin(list_); 
            typename descriptor_list::iterator end = std::end(list_); 

            for (pointer p = mem; descr != end; ++descr, p += area)
            {
                free = *descr & freebit;
                area = *descr & areabits; 

                if (free && area >= num)
                {
                    *descr = num;
                    area -= num;

                    if (area > 0) // fragment descriptor
                        list_.insert (descr+1, freebit | area);

                    ptr = p;
                    break;
                }
            }

            ASSERTF (descr != end, "unable to allocate pointer");
            ASSERTF ((ptr >= mem) && (ptr < mem + size), "free list is corrupt");

            return ptr;
        }

        // Deallocate

        template <typename T>
        auto linear_allocator<T>::deallocate (pointer ptr, size_type num) -> void
        {
            ASSERTF ((ptr >= mem) && (ptr < mem + size), "pointer is not from this heap");

            using std::remove;

            nfree_ += num;

            size_type free, area;
            typename descriptor_list::iterator begin = std::begin(list_); 
            typename descriptor_list::iterator descr = begin;
            typename descriptor_list::iterator end = std::end(list_); 
            typename descriptor_list::iterator next;

            for (pointer p = mem; descr != end; ++descr, p += area)
            {
                if (ptr == p)
                {
                    ASSERTF (!(*descr & freebit), "pointer was double-freed");

                    *descr |= freebit;

                    // merge neighboring descriptors

                    next = descr + 1;
                    free = *next & freebit;
                    area = *next & areabits; 

                    if (free && next != end)
                    {
                        *descr += area;
                        *next = 0; // clear for removal
                    }

                    next = descr--;
                    free = *descr & freebit;
                    area = *descr & areabits; 

                    if (free && next != begin)
                    {
                        *descr += area;
                        *next = 0; // clear for removal
                    }

                    break;
                }
            }

            ASSERTF (descr != end, "unable to deallocate pointer");

            // remove any invalid descriptors due to merge
            list_.erase (remove (descr, descr+3, 0), end(list_));
        }
    }
}

#endif
