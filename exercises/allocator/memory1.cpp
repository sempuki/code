/* main.cpp -- main module
 *
 *			Ryan McDougall
 */

#include <iostream>
#include <algorithm>
#include <vector>

#include <tr1/memory>

#include <assert.h>
#include <stdint.h>

using namespace std;
using namespace std::tr1;

struct A
{
    A () { cout << "A()" << endl; }
    A (int a) { cout << "A(int)" << endl; }
    A (int a, float b) { cout << "A(int,float)" << endl; }
    A (A const &copy) { cout << "A(copy)" << endl; }
    ~A () { cout << "~A()" << endl; }
};

// Large-block, static allocator.
// Meant to logically partition available memory at load time.
// Not to be used for dynamic/small-object memory allocation.
// All allocations much be as large as sizeof(void *).
// Fragmentation from repeated allocation/deallocation is expected.
// Allocation is linear; deallocation is constant.

namespace Memory
{
    template <size_t Alignment = 1>
    class StaticAllocation
    {
        public:
            typedef size_t      size_type;
            typedef uint8_t     byte_type;

        private:
            struct record_type
            {
                uint32_t    guard;
                size_type   size;
                byte_type   *pointer;
            };

        public:
            StaticAllocation (string name, size_type size, StaticAllocation *parent = 0) : 
                name_ (name), 
                parent_ (parent),
                max_bytes_ (size),
                free_bytes_ (size)
        {
            if (parent_)
                memory_ = (byte_type *)(parent_->allocate (size));
            else
                memory_ = new byte_type [size];

            free_bytes_ -= get_record_size_ ();
            next_free_ = 0, write_free_record_ (memory_, free_bytes_);
            next_free_ = (byte_type *)(memory_);
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

                // allocate for requested alignment
                bytes = get_aligned_ (bytes);

                // find next suitable free block (includes the free record)
                while (free && ((diff = check_free_record_ (free, bytes)) < 0))
                    prev = free, free = get_next_free_block_ (prev);

                if (free)
                {
                    // ensure there is enough memory to fragment
                    bool fragment = (diff > sizeof (record_type));
                    if (!fragment) bytes += diff;

                    // allocate memory
                    free_bytes_ -= bytes;
                    free = write_alloc_record_ (prev, free, bytes);
                    memory = (void *)(get_aligned_ (free));

                    // fragment memory
                    if (fragment) 
                    {
                        diff -= get_record_size_ ();
                        free_bytes_ -= get_record_size_ ();
                        write_free_record_ (free + bytes, diff);
                    }

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
                write_free_record_ (alloc, size);
                free_bytes_ += size;
            }

            void defragment ()
            {
                using std::sort;
                using std::vector;

                record_type *record;
                vector<byte_type *> free;

                // sort free blocks for coalescing
                for (byte_type *b = next_free_; b; b = get_next_free_block_ (b))
                    free.push_back (b);
                sort (free.begin(), free.end());

                // coalesce and re-link
                vector<byte_type *>::iterator i, n, e;
                for (i = free.begin(), e = free.end(); (i != e) && (n != e); ++i)
                {
                    record = (record_type *)(*i);

                    // find largest contiguous chunk
                    for (n = i+1; (*i + get_block_size_ (*i) == *n) && (n != e); ++n)
                        record->size += get_block_size_ (*n);

                    // re-link current free-list
                    if (n != e) record->pointer = *n; 
                    else record->pointer = 0;
                }

                // set new free head and count
                free_bytes_ = 0;
                next_free_ = *free.begin();
                for (byte_type *b = next_free_; b; b = get_next_free_block_ (b))
                    free_bytes_ += ((record_type *)(b))->size;
            }

        private:
            void write_free_record_ (byte_type *p, size_type s)
            {
                record_type *record = (record_type *)(p);
                record->guard = 0xDDDDDDDD;
                record->size = s;
                record->pointer = next_free_;

                // update head of free list to this
                next_free_ = p;
            }

            byte_type *write_alloc_record_ (byte_type *prev, byte_type *p, size_type s)
            {
                record_type *record = (record_type *)(p);
                record->guard = 0xAAAAAAAA;
                record->size = s;

                // update predecessor to next free
                if (prev) ((record_type *)(prev))->pointer = record->pointer;
                else next_free_ = record->pointer;

                // return address of allocated memory
                return p + offsetof (record_type, pointer);
            }

            size_type check_free_record_ (byte_type *p, size_type s)
            {
                record_type *record = (record_type *)(p);
                assert (record->guard == 0xDDDDDDDD);

                // return diff of available and requested size
                return record->size - s;
            }

            size_type check_alloc_record_ (byte_type *p)
            {
                record_type *record = (record_type *)(p);
                assert (record->guard == 0xAAAAAAAA);

                // return allocated size
                return record->size;
            }

            byte_type *get_next_free_block_ (byte_type *p)
            {
                return ((record_type *)(p))->pointer;
            }

            byte_type *get_block_ (byte_type *p)
            {
                // allocated memory starts from pointer
                return (byte_type *)(p) - offsetof (record_type, pointer);
            }

            size_type get_block_size_ (byte_type *p)
            {
                // free + record size
                return ((record_type *)(p))->size + get_record_size_ ();
            }

            size_type get_record_size_ ()
            {
                // record pointer must be the last member
                return offsetof (record_type, pointer);
            }

            byte_type *get_aligned_ (byte_type *p)
            {
                return (byte_type *)(((uintptr_t)(p) + Alignment-1) & ~(Alignment-1));
            }

            size_type get_aligned_ (size_type s)
            {
                return (s + Alignment-1) & ~(Alignment-1);
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
}

namespace Object
{
    template <typename T, typename Allocator> 
    class Factory
    {
        public:
            typedef Allocator AllocatorType;

            Factory (Allocator *a) : mem_(a) {}
            Allocator *allocator () { return mem_; }

            T *create () 
            { 
                void *memory = mem_->allocate (sizeof (T));
                if (!memory) throw std::bad_alloc();
                return new (memory) T (); 
            }

            template <typename A0> 
            T *create (A0 a0) 
            { 
                void *memory = mem_->allocate (sizeof (T));
                if (!memory) throw std::bad_alloc();
                return new (memory) T (a0); 
            }

            template <typename A0, typename A1> 
            T *create (A0 a0, A1 a1) 
            { 
                void *memory = mem_->allocate (sizeof (T));
                if (!memory) throw std::bad_alloc();
                return new (memory) T (a0, a1); 
            }

            template <typename A0, typename A1, typename A2> 
            T *create (A0 a0, A1 a1, A2 a2) 
            { 
                void *memory = mem_->allocate (sizeof (T));
                if (!memory) throw std::bad_alloc();
                return new (memory) T (a0, a1, a2); 
            }

            template <typename A0, typename A1, typename A2, typename A3> 
            T *create (A0 a0, A1 a1, A2 a2, A3 a3) 
            { 
                void *memory = mem_->allocate (sizeof (T));
                if (!memory) throw std::bad_alloc();
                return new (memory) T (a0, a1, a2, a3); 
            }

            template <typename A0, typename A1, typename A2, typename A3, typename A4> 
            T *create (A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) 
            { 
                void *memory = mem_->allocate (sizeof (T));
                if (!memory) throw std::bad_alloc();
                return new (memory) T (a0, a1, a2, a3, a4); 
            }

            template <typename U>
            T *createArray (size_t num, U const &init) 
            {
                using std::uninitialized_fill;

                size_t bytes = sizeof (T) * num;
                T *array = (T *)(mem_->allocate (bytes));
                if (!array) throw std::bad_alloc();

                uninitialized_fill (array, array + bytes, init);

                return array;
            }

            void destroy (T *object)
            {
                object->~T();
                mem_->deallocate (object);
            }

            void destroyArray (size_t num, T *array)
            {
                for (size_t i=0; i < num; ++i)
                    array[i].~T();
                mem_->deallocate (array);
            }

        private:
            Allocator   *mem_;
    };
}

//=============================================================================
// Main entry point
int
main (int argc, char** argv)
{
    int i = 5;
    float f = 3.14;

    Memory::StaticAllocation<4> allocation ("main", 100);
    cout << "available memory: " << allocation.free() << endl;

    Object::Factory <A, Memory::StaticAllocation<4> > factory (&allocation);
    A *a0 = factory.create ();
    A *a1 = factory.create (i);
    A *a2 = factory.create (i,f);
    cout << "available memory: " << allocation.free() << endl;

    A *array = factory.createArray (5, *a2);
    cout << "available memory: " << allocation.free() << endl;

    factory.destroy (a0);
    factory.destroy (a1);
    factory.destroy (a2);
    cout << "available memory: " << allocation.free() << endl;

    factory.destroyArray (5, array);
    cout << "available memory: " << allocation.free() << endl;

    factory.allocator()->defragment();
    cout << "available memory: " << allocation.free() << endl;

    return 0;
}
