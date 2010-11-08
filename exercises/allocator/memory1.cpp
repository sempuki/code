/* main.cpp -- main module
 *
 *			Ryan McDougall
 */

#include <iostream>
#include <tr1/memory>
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
// Allocation is O(1), but deallocation is linear. 
// Fragmentation is not handled explicitly.

template <typename AlignType>
class Allocation
{
    public:
        typedef AlignType   align_type;
        typedef size_t      size_type;
        typedef uint8_t     byte_type;
        typedef uint32_t    guard_type;

    private:
        struct record_type
        {
            guard_type  guard;
            size_type   size;
        };
    
    public:
        Allocation (string name, size_type size, Allocation *parent = 0) : 
            name_ (name), 
            parent_ (parent),
            max_bytes_ (size)
        {
            if (parent_)
                memory_ = parent_->allocate (size);
            else
                memory_ = new byte_type [size];

            free_bytes_ = max_bytes_ - sizeof (record_type);
            next_ = 0, write_free_record_ (memory_, free_bytes_);
            next_ = memory_;
        }

        ~Allocation ()
        {
            if (parent_)
                parent_->deallocate (memory_, max_bytes_);
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
            byte_type *free = next_;

            // find next suitable free block (includes the free record)
            while (free && (diff = check_free_record_ (free, bytes) < 0))
                free = get_next_free_block_ (free);

            if (free)
            {
                // write allocation record
                free = write_alloc_record_ (free, bytes);

                // fragment the block
                if (diff > (2 * sizeof (record_type)))
                    write_free_record_ (free + bytes, diff);

                // allocation
                memory = (void *)free;

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

            // write free record
            size_type size = check_alloc_record_ (alloc);
            byte_type *next = write_free_record_ (alloc, size);

            // update head of free list
            next_ = alloc;
        }

        void coalesce ()
        {
        }

    private:
        byte_type *write_free_record_ (byte_type *p, size_type size)
        {
            record_type *record = (record_type *)(p);
            record->guard = (guard_type) 0xDDDDDDDD;
            record->size = (size_type) size;
            p += sizeof (record_type);

            // write the last head of the free list
            *((byte_type **)(p)) = (byte_type *)(next_);

            // update free byte count
            free_bytes_ += size;

            // return address of pointer to next free block
            return p;
        }

        byte_type *write_alloc_record_ (byte_type *p, size_type size)
        {
            record_type *record = (record_type *)(p);
            record->guard = (guard_type) 0xAAAAAAAA;
            record->size = (size_type) size;
            p += sizeof (record_type);

            // update free byte count
            free_bytes_ -= size + sizeof (record_type);

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
            return *(p + sizeof (record_type));
        }

        byte_type *get_block_ (byte_type *p)
        {
            return (byte_type *)(p) - sizeof (record_type);
        }

    private:
        Allocation (const Allocation &copy);
        void operator= (const Allocation &rhs);

    private:
        string          name_;
        Allocation      *parent_;
        byte_type       *memory_;
        byte_type       *next_;
        size_type       free_bytes_;
        size_type const max_bytes_;
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

    Factory<A> factory;
    A *a0 = factory.create();
    A *a1 = factory.create(i);
    A *a2 = factory.create(i,f);

    return 0;
}
