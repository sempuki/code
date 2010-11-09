/* main.cpp -- main module
 *
 *			Ryan McDougall
 */

#include <iostream>
#include <tr1/memory>

#include <assert.h>
#include <stdint.h>

using namespace std;

struct A
{
    A () { cout << "A()" << endl; }
    A (int a) { cout << "A(int)" << endl; }
    A (int a, float b) { cout << "A(int,float)" << endl; }
    ~A () { cout << "~A()" << endl; }
};

// Meant to logically partition available memory at load time.
// Not to be used for dynamic/small-object memory allocation.
// Fragmentation from repeated allocation/deallocation is expected.
// Allocation is linear; deallocation is constant.

template <int Alignment>
class StaticAllocation
{
    private:
        typedef size_t      size_type;
        typedef uint8_t     byte_type;
        typedef uint32_t    guard_type;

        struct record_type
        {
            guard_type  guard;
            size_type   size;
        };
    
    public:
        StaticAllocation (string name, size_type size, StaticAllocation *parent = 0) : 
            name_ (name), 
            parent_ (parent),
            max_bytes_ (size)
        {
            if (parent_)
                memory_ = (byte_type *)(parent_->allocate (size));
            else
                memory_ = new byte_type [size];

            free_bytes_ = 0;
            next_free_ = 0; // sentinel value indicating end of free list

            write_free_record_ (&next_free_, memory_, size - sizeof (record_type));
        }

        ~StaticAllocation ()
        {
            if (parent_)
                parent_->deallocate ((void *)(memory_));
            else
                delete [] memory_;

            memory_ = 0;
        }

        size_type size () const { return max_bytes_; }
        size_type free () const { return free_bytes_; }

        void *allocate (size_type bytes) 
        {
            void *memory = 0;

            size_type diff = 0;
            byte_type *prev = 0;
            byte_type *free = next_free_;

            // find next suitable free block (includes the free record)
            while (free && ((diff = check_free_record_ (free, bytes)) < 0))
                prev = free, free = get_next_free_block_ (prev);

            if (free)
            {
                bool fragment = false;
                if (diff > sizeof (record_type) << 1)
                    diff -= sizeof (record_type), fragment = true;
                else
                    bytes += diff;

                // allocate memory
                free = write_alloc_record_ (&prev, free, bytes);
                free_bytes_ -= bytes + sizeof (record_type);
                memory = (void *)free;

                // fragment memory
                if (fragment) write_free_record_ (&next_free_, free + bytes, diff);

                // assert allocation is in-bounds
                assert (memory > memory_ && memory < memory_ + max_bytes_);
            }

            return memory;
        }

        void deallocate (void *memory)
        {
            // assert allocation is in-bounds
            assert (memory > memory_ && memory < memory_ + max_bytes_);

            // get the block (includes the allocation record)
            byte_type *alloc = get_block_ ((byte_type *)(memory));
            size_type size = check_alloc_record_ (alloc);

            // write free record
            write_free_record_ (&next_free_, alloc, size);
            free_bytes_ += size;
        }

        void coalesce ()
        {
        }

    private:
        byte_type *write_free_record_ (byte_type **head, byte_type *p, size_type size)
        {
            record_type *record = (record_type *)(p);
            record->guard = (guard_type) 0xDDDDDDDD;
            record->size = (size_type) size;
            p += sizeof (record_type);

            // write the last head of the free list
            *((byte_type **)(p)) = *(head);

            // update head of free list to this
            *(head) = p - sizeof (record_type);

            // return address of pointer to next free block
            return p;
        }

        byte_type *write_alloc_record_ (byte_type **pred, byte_type *p, size_type size)
        {
            record_type *record = (record_type *)(p);
            record->guard = (guard_type) 0xAAAAAAAA;
            record->size = (size_type) size;
            p += sizeof (record_type);

            // update predecessor to next free
            *(pred) = *((byte_type **)(p));
            
            // return address of allocated memory
            return p;
        }

        size_type check_free_record_ (byte_type *p, size_type size)
        {
            record_type *record = (record_type *)(p);
            assert (record->guard == 0xDDDDDDDD);

            // return diff of available and requested size
            return record->size - size;
        }

        size_type check_alloc_record_ (byte_type *p)
        {
            record_type *record = (record_type *)(p);
            assert (record->guard == 0xAAAAAAAA);

            // return allocated size
            return record->size;
        }

        size_type get_block_size_ (byte_type *p)
        {
            record_type *record = (record_type *)(p);
            return record->size + sizeof (record_type);
        }

        byte_type *get_next_free_block_ (byte_type *p)
        {
            return *((byte_type **)(p + sizeof (record_type)));
        }

        byte_type *get_block_ (byte_type *p)
        {
            return (byte_type *)(p) - sizeof (record_type);
        }

    private:
        StaticAllocation (const StaticAllocation &copy);
        void operator= (const StaticAllocation &rhs);

    private:
        string              name_;
        StaticAllocation    *parent_;
        byte_type           *memory_;
        byte_type           *next_free_;
        size_type           free_bytes_;
        size_type const     max_bytes_;
};

template <typename T, typename Allocator> 
class Factory
{
    public:
        Factory (Allocator &a) : alloc_(a) {}

        T *create () { return new T(); }

        template <typename A0> 
        T *create (A0 a0) 
        { 
            return new T(a0); 
        }

        template <typename A0, typename A1> 
        T *create (A0 a0, A1 a1) 
        { 
            return new T(a0, a1); 
        }

        template <typename A0, typename A1, typename A2> 
        T *create (A0 a0, A1 a1, A2 a2) 
        { 
            return new T(a0, a1, a2); 
        }

        template <typename A0, typename A1, typename A2, typename A3> 
        T *create (A0 a0, A1 a1, A2 a2, A3 a3) 
        { 
            return new T(a0, a1, a2, a3); 
        }

        template <typename A0, typename A1, typename A2, typename A3, typename A4> 
        T *create (A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) 
        { 
            return new T(a0, a1, a2, a3, a4); 
        }

    private:
        Allocator   &alloc_;
};

//=============================================================================
// Main entry point
int
main (int argc, char** argv)
{
    int i = 5;
    float f = 3.14;

    //Factory<A> factory;
    //A *a0 = factory.create();
    //A *a1 = factory.create(i);
    //A *a2 = factory.create(i,f);

    StaticAllocation <sizeof(void *)> allocation ("main", 100);
    int *a = (int *)(allocation.allocate (sizeof(int)));
    int *b = (int *)(allocation.allocate (sizeof(int)));
    int *c = (int *)(allocation.allocate (sizeof(int)));

    *a = 69;
    *b = 69;
    *c = 69;

    return 0;
}
