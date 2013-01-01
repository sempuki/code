#ifndef _MEMORY_LAYOUT_HPP_
#define _MEMORY_LAYOUT_HPP_


namespace sequia
{
    namespace memory
    {
        struct heap
        {
            size_t const page_size;
            page_list;
        };

        //--------------------------------------------------------------------
        // Organizes the layout of memory as read in from a layout description
        // file by allocating heaps on construction and returning them by name.

        class layout
        {

        };
    }
}

#endif
