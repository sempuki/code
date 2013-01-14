#ifndef _MEMORY_LAYOUT_HPP_
#define _MEMORY_LAYOUT_HPP_


namespace sequia { namespace memory {

    using page = buffer<uint8_t>;

    struct heap
    {
        core::name name;
        size_t const page_size;

        core::fixed_vector<uint8_t *> page_list;
        uint64_t page_allocation; // TODO: thread-safe atomic

        //page acquire_page () { }
        //void release_page (page mem) { }
    };

    //-------------------------------------------------------------------------
    // Organizes the global layout of memory as read in from a layout description 
    // file by allocating heaps on construction and returning them by name. 
    // Allocators request and release pages from named heaps on demand. Different 
    // allocators may share a heap, and so operations must be thread-safe.

    class layout
    {
    };
    
} }

#endif
