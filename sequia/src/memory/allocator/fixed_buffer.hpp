#ifndef _FIXED_BUFFER_ALLOCATOR_HPP_
#define _FIXED_BUFFER_ALLOCATOR_HPP_

namespace sequia { namespace memory { namespace allocator {

    //=====================================================================
    // Allocates the same fixed buffer each call

    template <typename Type>
    class fixed_buffer : public std::allocator <Type>
    {
        public:
            template <class U> struct rebind { using other = fixed_buffer<U>; };

        public:
            fixed_buffer () {}

            fixed_buffer (size_t const size) :
                mem_ {(Type*) nullptr, size} {}

            fixed_buffer (fixed_buffer const &copy) :
                mem_ {(Type*) nullptr, copy.max_size()} {}

            template <class U>
            fixed_buffer (fixed_buffer<U> const &copy) :
                mem_ {(Type*) nullptr, copy.max_size()} {}

            ~fixed_buffer ()
            {
                delete [] mem_.items;
                mem_.invalidate ();
            }

        public:
            size_t max_size () const 
            { 
                return mem_.size ();
            }

            Type *allocate (size_t num, const void* = 0) 
            { 
                if (!mem_) 
                {
                    // TODO: don't use global heap...
                    auto mem = new Type [max_size ()]; 
                    swap (mem_, memory::buffer<Type> (mem, max_size ()));
                }

                return mem_.items;
            }

            void deallocate (Type *ptr, size_t num) 
            {
                ASSERTF (contains (mem_, ptr), "not from this allocator");
            }

        private:
            memory::buffer<Type> mem_;
    };
        
} } }

#endif
