#ifndef _NULL_ALLOCATOR_HPP_
#define _NULL_ALLOCATOR_HPP_

namespace sequia
{
    namespace memory
    {
        namespace allocator
        {
            //=========================================================================
            // Implements null-operation semantics
            // Fulfills stateful allocator concept
            // Fulfills rebindable allocator concept
            // Fulfills terminal allocator concept

            template <typename T>
            class null
            {
                public:
                    using base_type = std::false_type;
                    
                    using value_type = T;
                    using state_type = base_state<T>;

                public:
                    using propagate_on_container_copy_assignment = std::false_type;
                    using propagate_on_container_move_assignment = std::false_type;
                    using propagate_on_container_swap = std::false_type;

                public:
                    template <typename U> 
                    struct rebind 
                    { 
                        using other = null<U>; 
                    };

                public:
                    // default constructor
                    null () = default;

                    // copy constructor
                    null (null const &copy) = default;

                    // stateful constructor
                    explicit null (state_type const &state) {}

                    // destructor
                    ~null () = default;

                public:
                    // return max allocation of zero
                    size_t max_size () const { return 0; }

                    // allocation is a null operation
                    T *allocate (size_t num, const void* = 0) {}

                    // deallocation is a null operation
                    void deallocate (T *ptr, size_t num) {}
                
                public:
                    // state accessor
                    virtual state_type const &state() const = 0;
            };
        }
    }
}

#endif
