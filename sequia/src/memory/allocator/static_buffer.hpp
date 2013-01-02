#ifndef _STATIC_BUFFER_ALLOCATOR_HPP_
#define _STATIC_BUFFER_ALLOCATOR_HPP_

namespace sequia
{
    namespace memory
    {
        namespace allocator
        {
            //=====================================================================
            // Allocates the same static buffer each call

            template <typename Type, size_t N>
            class static_buffer : public std::allocator <Type>
            {
                public:
                    static_buffer (static_buffer const &copy) :
                        static_buffer {} 
                    {
                        WATCHF (false, "allocator copy constructor called");
                    }

                    template <class U>
                    static_buffer (static_buffer<U,N> const &copy ) :
                        static_buffer {} 
                    {
                        WATCHF (false, "allocator rebind copy constructor called");
                    }

                public:
                    size_t max_size () const 
                    { 
                        return N;
                    }

                    Type *allocate (size_t num, const void* = 0) 
                    { 
                        ASSERTF (!mem_.valid(), "previously allocated");
                        ASSERTF (N == num, "incorrect allocation size");

                        return mem_.items;
                    }

                    void deallocate (Type *ptr, size_t num) 
                    {
                        ASSERTF (mem_.valid(), "not previously allocated");
                        ASSERTF (mem_.items == ptr, "not from this allocator");
                        ASSERTF (N == num, "incorrect allocation size");

                        mem_.invalidate();
                    }

                private:
                    memory::static_buffer<Type,N>  mem_;
            };
        }
    }
}

#endif
