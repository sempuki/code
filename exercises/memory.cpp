/* main.cpp -- main module
 *
 *			Ryan McDougall
 */

#include <iostream>
#include <algorithm>
#include <vector>

#include <tr1/memory>
#include <tr1/functional>

#include <assert.h>
#include <stdint.h>

using namespace std;
using namespace std::tr1;
using namespace std::tr1::placeholders;

struct A
{
    A () { cout << "A()" << endl; }
    A (int a) { cout << "A(int)" << endl; }
    A (int a, float b) { cout << "A(int,float)" << endl; }
    A (A const &copy) { cout << "A(copy)" << endl; }
    ~A () { cout << "~A()" << endl; }
};

namespace Memory
{
    void *align (void *pointer, size_t alignment)
    {
        return (void *)(((uintptr_t)(pointer) + alignment-1) & ~(alignment-1));
    }

    size_t alignment_offset (void *pointer, size_t alignment)
    {
        return (uintptr_t)(align (pointer, alignment)) - (uintptr_t)(pointer);
    }

    // Large-block, static allocator, using first-fit allocation.
    // Meant to logically partition available memory at load time.
    // Not to be used for dynamic/small-object memory allocation.
    // Fragmentation from repeated allocation/deallocation is expected.
    // Allocation is linear, deallocation is constant; free-list uses embedded linked-list.
    // Uses no heap memory external to the allocation.

    class StaticAllocation
    {
        public:
            typedef size_t  size_type;
            typedef uint8_t byte_type;

        private:
            struct record_type
            {
                typedef std::vector<record_type *> list;

                uint32_t    guard;
                size_type   size;
                record_type *next;
            
                static const size_t overhead;
            };

        public:
            StaticAllocation 
                (string name, 
                 size_type size, 
                 StaticAllocation *parent = 0, 
                 size_type alignment = 1)
                : name_ (name), parent_ (parent)
            {
                max_bytes_ = size;
                memory_ = base_allocate_ (size, alignment);

                free_bytes_ = size - record_type::overhead;
                free_list_ = 0, write_free_record_ ((record_type *)(memory_), free_bytes_);
                free_list_ = (record_type *)(memory_);
            }

            ~StaticAllocation ()
            {
                base_deallocate_ (memory_, max_bytes_);
            }

            size_type size () const { return max_bytes_; }
            size_type free () const { return free_bytes_; }

            StaticAllocation *suballocate (string name, size_type bytes)
            {
                using std::tr1::alignment_of;

                void *m = allocate (sizeof (StaticAllocation), alignment_of<StaticAllocation>::value);
                return new (m) StaticAllocation (name, bytes, this);
            }

            void *allocate (size_type bytes, size_type alignment = 1) 
            {
                void *memory = 0;

                size_type diff = 0;
                record_type *prev = 0;
                record_type *free = free_list_;

                // ensure space for complete free record
                if (bytes < sizeof (byte_type *))
                    bytes = sizeof (byte_type *);

                // find next suitable free block of memory
                while (free && ((diff = check_free_record_ (free, bytes, alignment)) < 0))
                    prev = free, free = prev->next;

                if (free)
                {
                    // ensure there is enough memory to fragment
                    bool fragment = (diff > sizeof (record_type));
                    if (!fragment) bytes += diff;

                    // find alignment requirements
                    memory = get_memory_ (free);
                    bytes += alignment_offset (memory, alignment);
                    
                    // allocate memory
                    write_alloc_record_ (prev, free, bytes);
                    free_bytes_ -= bytes;

                    // fragment memory
                    if (fragment) 
                    {
                        diff -= record_type::overhead;
                        free_bytes_ -= record_type::overhead;
                        write_free_record_ (free + bytes, diff);
                    }

                    // assert allocation is in-bounds
                    assert (memory > memory_ && memory < memory_ + max_bytes_);
                }

                return memory;
            }

            void deallocate (void *memory, size_type size)
            {
                deallocate (memory);
            }

            void deallocate (void *memory)
            {
                // assert allocation is in-bounds
                assert (memory > memory_ && memory < memory_ + max_bytes_);

                // get the block of memory
                record_type *alloc = get_record_ (memory);
                size_type size = check_alloc_record_ (alloc);

                // write free record
                write_free_record_ (alloc, size);
                free_bytes_ += size;
            }

            void defragment ()
            {
                using std::sort;

                // sort free blocks for coalescing
                typename record_type::list free;
                for (record_type *r = free_list_; r; r = r->next)
                    free.push_back (r);

                sort (free.begin(), free.end());

                // coalesce and re-link
                typename record_type::list::iterator i = free.begin();
                typename record_type::list::iterator n = free.begin();
                typename record_type::list::iterator e = free.end();

                for (; (i != e) && (n != e); ++i)
                {
                    // find largest contiguous chunk
                    for (n = i+1; contiguous_block_ (*i, *n) && (n != e); ++n)
                        (*i)->size += get_block_size_ (*n);

                    // re-link current free-list
                    if (n != e) (*i)->next = *n; 
                    else (*i)->next = 0;
                }

                // set new free head and count
                free_bytes_ = 0;
                free_list_ = free.front();

                for (record_type *r = free_list_; r; r = r->next)
                    free_bytes_ += r->size;
            }

        private:
            byte_type *base_allocate_ (size_type size, size_type alignment)
            {
                void *memory = 0;

                if (parent_)
                    memory = parent_->allocate (size, alignment);
                else
                {
                    size += alignment - 1; // over-allocate for alignment
                    memory = align (new byte_type [size], alignment);
                }

                return (byte_type *)(memory);
            }

            void base_deallocate_ (byte_type *&memory, size_type size)
            {
                if (parent_)
                    parent_->deallocate (memory, size);
                else
                    delete [] memory;

                memory = 0;
            }
                
            record_type *get_record_ (void *pointer)
            {
                // rewind to record from pointer
                return (record_type *)((byte_type *)(pointer) - record_type::overhead);
            }

            void *get_memory_ (record_type *rec)
            {
                // free memory starts from next
                return (byte_type *)(rec) + record_type::overhead;
            }

            void write_free_record_ (record_type *rec, size_type size)
            {
                rec->guard = 0xDDDDDDDD;
                rec->size = size;

                // add record to head of free list
                rec->next = free_list_;
                free_list_ = rec;
            }

            void write_alloc_record_ (record_type *prev, record_type *rec, size_type size)
            {
                rec->guard = 0xAAAAAAAA;
                rec->size = size;

                // update predecessor to next free
                if (prev) prev->next = rec->next;
                else free_list_ = rec->next;
            }

            size_type check_free_record_ (record_type *rec, size_type size, size_type alignment = 1)
            {
                assert (rec->guard == 0xDDDDDDDD);

                // include space needed for alignment
                size += alignment_offset (get_memory_ (rec), alignment);

                // return diff of available and requested size
                return rec->size - size;
            }

            size_type check_alloc_record_ (record_type *rec)
            {
                assert (rec->guard == 0xAAAAAAAA);

                // return allocated size
                return rec->size;
            }

            bool contiguous_block_ (record_type *a, record_type *b)
            {
                return ((byte_type *)(a) + a->size + record_type::overhead) == (byte_type *)(b);
            }

            size_type get_block_size_ (record_type *rec)
            {
                return rec->size + record_type::overhead;
            }

        private:
            StaticAllocation (const StaticAllocation &copy);
            void operator= (const StaticAllocation &rhs);

        private:
            string              name_;
            StaticAllocation    *parent_;

            byte_type           *memory_;
            size_type           free_bytes_;
            size_type           max_bytes_;
            
            record_type         *free_list_;
    };

    // record_type::next is considered free memory
    const size_t StaticAllocation::record_type::overhead = 
        sizeof (StaticAllocation::record_type) - sizeof (StaticAllocation::record_type::next);

    // Designed for fast allocation of large blocks of non-uniform memory using 
    // worst-fit allocation. Allocation and deallocation is logarithmic ammortized 
    // complexity; the free-list uses a heap. Consumes external heap memory 
    // (from std::allocator), linear in the size of the free-list.
    
    template <typename Allocator = StaticAllocation>
    class LargeBlockAllocator
    {
        public:
            typedef typename Allocator::size_type size_type;
            typedef typename Allocator::byte_type byte_type;

        private:
            struct record_type
            {
                typedef std::vector<record_type> heap;

                size_type   size;
                byte_type   *memory;

                record_type (byte_type *b, size_type s) : memory (b), size (s) {}
            };

            struct record_empty_pred
            {
                bool operator() (const record_type &b) const
                {
                    return b.size == 0;
                }
            };

            struct record_addr_comp
            {
                bool operator() (const record_type &l, const record_type &r) const
                {
                    return l.memory < r.memory;
                }
            };

            struct record_size_comp
            {
                bool operator() (const record_type &l, const record_type &r) const
                {
                    return l.size < r.size;
                }
            };

        public:
            LargeBlockAllocator 
                (string name, 
                 size_type size, 
                 Allocator *parent = 0, 
                 size_type alignment = 1)
                : name_ (name), parent_ (parent)
            {
                max_bytes_ = size;
                memory_ = base_allocate_ (size, alignment);

               free_bytes_ = 0, push_free_record_ (record_type (memory_, max_bytes_));
            }

            ~LargeBlockAllocator ()
            {
                base_deallocate_ (memory_, max_bytes_);
            }

            size_type size () const { return max_bytes_; }
            size_type free () const { return free_bytes_; }

            void *allocate (size_type bytes, size_t alignment = 1)
            {
                void *memory = 0;

                record_type rec (top_free_record_ ());

                if (rec.size >= bytes)
                {
                    pop_free_record_ ();

                    if (rec.size > bytes)
                    {
                        record_type fragment (rec.memory + bytes, rec.size - bytes);
                        push_free_record_ (fragment);
                    }
                    
                    memory = rec.memory;
                }

                return memory;
            }

            void deallocate (void *memory, size_type bytes)
            {
                push_free_record_ (record_type ((byte_type *)(memory), bytes));
            }

            void defragment ()
            {
                using std::sort;
                using std::remove_if;

                // sort free blocks for coalescing
                sort (free_list_.begin(), free_list_.end(), addr_comp_);
                
                // coalesce and re-link
                typename record_type::heap::iterator i = free_list_.begin();
                typename record_type::heap::iterator n = free_list_.begin();
                typename record_type::heap::iterator e = free_list_.end();

                for (; (i != e) && (n != e); ++i)
                {
                    // find largest contiguous chunk
                    for (n = i+1; i->memory + i->size == n->memory && (n != e); ++n)
                        i->size += n->size, n->size = 0;
                }

                // remove empty blocks
                n = remove_if (free_list_.begin(), free_list_.end(), empty_pred_);
                free_list_.erase (n, free_list_.end());
                
                // rebuild heap
                make_heap (free_list_.begin(), free_list_.end(), size_comp_);
            }

        private:
            byte_type *base_allocate_ (size_type size, size_type alignment)
            {
                void *memory = 0;

                if (parent_)
                    memory = parent_->allocate (size, alignment);
                else
                {
                    size += alignment - 1; // over-allocate for alignment
                    memory = align (new byte_type [size], alignment);
                }

                return (byte_type *)(memory);
            }

            void base_deallocate_ (byte_type *&memory, size_type size)
            {
                if (parent_)
                    parent_->deallocate (memory, size);
                else
                    delete [] memory;

                memory = 0;
            }
                
            record_type top_free_record_ () const
            {
                return free_list_.size()? *(free_list_.begin()) : record_type (0, 0);
            }

            void push_free_record_ (const record_type &rec)
            {
                using std::push_heap;

                free_list_.push_back (rec); 
                free_bytes_ += free_list_.back().size; 

                push_heap (free_list_.begin(), free_list_.end(), size_comp_);
            }

            void pop_free_record_ ()
            {
                using std::pop_heap;

                pop_heap (free_list_.begin(), free_list_.end(), size_comp_); 

                free_bytes_ -= free_list_.back().size;
                free_list_.pop_back ();
            }

        private:
            string      name_;
            Allocator   *parent_;

            byte_type   *memory_;
            size_type   free_bytes_;
            size_type   max_bytes_;

            typename record_type::heap  free_list_;
            record_addr_comp            addr_comp_;
            record_size_comp            size_comp_;
            record_empty_pred           empty_pred_;
    };

    template <typename Allocator = StaticAllocation>
    class SmallBlockAllocator
    {
        public:
            typedef typename Allocator::size_type size_type;
            typedef typename Allocator::byte_type byte_type;
            
        private:
            struct block_type
            {
                size_type   size;
                byte_type   *memory;

                block_type (byte_type *b, size_type s) : memory (b), size (s) {}
            };

        public:
            SmallBlockAllocator 
                (string name, 
                 size_type size, 
                 Allocator *parent = 0,
                 size_type alignment = 1)
                : name_ (name), parent_ (parent)
            {
                max_bytes_ = size;
                memory_ = base_allocate_ (size, alignment);

                free_bytes_ = 0;
            }

            ~SmallBlockAllocator()
            {
                base_deallocate_ (memory_, max_bytes_);
            }

            void *allocate (size_type size)
            {
            }

            void deallocate (void *memory, size_type bytes)
            {
            }

            void deallocate (void *memory)
            {
            }
            
        private:
            byte_type *base_allocate_ (size_type size, size_type alignment)
            {
                void *memory = 0;

                if (parent_)
                    memory = parent_->allocate (size, alignment);
                else
                {
                    size += alignment - 1; // over-allocate for alignment
                    memory = align (new byte_type [size], alignment);
                }

                return (byte_type *)(memory);
            }

            void base_deallocate_ (byte_type *&memory, size_type size)
            {
                if (parent_)
                    parent_->deallocate (memory, size);
                else
                    delete [] memory;

                memory = 0;
            }

        private:
            string      name_;
            Allocator   *parent_;

            byte_type   *memory_;
            size_type   max_bytes_;
            size_type   free_bytes_;
    };
}

namespace Object
{
    template <typename T, typename Allocator> 
    class Factory
    {
        public:
            typedef Allocator AllocatorType;
            typedef std::tr1::shared_ptr<T> SharedPtrType;

            ~Factory () {} // TODO: destroy all allocated objects

            Factory (Allocator *a) : mem_(a) {}
            Allocator *allocator () { return mem_; }

            T *create () 
            { 
                void *memory = mem_->allocate (sizeof(T), alignment_of<T>::value);
                if (!memory) throw std::bad_alloc();
                return new (memory) T (); 
            }

            template <typename A0> 
            T *create (A0 a0) 
            { 
                void *memory = mem_->allocate (sizeof(T), alignment_of<T>::value);
                if (!memory) throw std::bad_alloc();
                return new (memory) T (a0); 
            }

            template <typename A0, typename A1> 
            T *create (A0 a0, A1 a1) 
            { 
                void *memory = mem_->allocate (sizeof(T), alignment_of<T>::value);
                if (!memory) throw std::bad_alloc();
                return new (memory) T (a0, a1); 
            }

            template <typename A0, typename A1, typename A2> 
            T *create (A0 a0, A1 a1, A2 a2) 
            { 
                void *memory = mem_->allocate (sizeof(T), alignment_of<T>::value);
                if (!memory) throw std::bad_alloc();
                return new (memory) T (a0, a1, a2); 
            }

            template <typename A0, typename A1, typename A2, typename A3> 
            T *create (A0 a0, A1 a1, A2 a2, A3 a3) 
            { 
                void *memory = mem_->allocate (sizeof(T), alignment_of<T>::value);
                if (!memory) throw std::bad_alloc();
                return new (memory) T (a0, a1, a2, a3); 
            }

            template <typename A0, typename A1, typename A2, typename A3, typename A4> 
            T *create (A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) 
            { 
                void *memory = mem_->allocate (sizeof(T), alignment_of<T>::value);
                if (!memory) throw std::bad_alloc();
                return new (memory) T (a0, a1, a2, a3, a4); 
            }

            SharedPtrType createShared () 
            { 
                return SharedPtrType (create (), 
                        bind (&Factory<T,Allocator>::destroy, this, _1));
            }

            template <typename A0> 
            SharedPtrType createShared (A0 a0) 
            { 
                return SharedPtrType (create (a0), 
                        bind (&Factory<T,Allocator>::destroy, this, _1));
            }

            template <typename A0, typename A1> 
            SharedPtrType createShared (A0 a0, A1 a1) 
            { 
                return SharedPtrType (create (a0, a1), 
                        bind (&Factory<T,Allocator>::destroy, this, _1));
            }

            template <typename A0, typename A1, typename A2> 
            SharedPtrType createShared (A0 a0, A1 a1, A2 a2) 
            { 
                return SharedPtrType (create (a0, a1, a2), 
                        bind (&Factory<T,Allocator>::destroy, this, _1));
            }

            template <typename A0, typename A1, typename A2, typename A3> 
            SharedPtrType createShared (A0 a0, A1 a1, A2 a2, A3 a3) 
            { 
                return SharedPtrType (create (a0, a1, a2, a3), 
                        bind (&Factory<T,Allocator>::destroy, this, _1));
            }

            template <typename A0, typename A1, typename A2, typename A3, typename A4> 
            SharedPtrType createShared (A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) 
            { 
                return SharedPtrType (create (a0, a1, a2, a3, a4), 
                        bind (&Factory<T,Allocator>::destroy, this, _1));
            }

            template <typename U>
            T *createArray (size_t num, U const &init) 
            {
                using std::uninitialized_fill;

                size_t bytes = sizeof (T) * num;
                T *array = (T *)(mem_->allocate (bytes), alignment_of<T>::value);
                if (!array) throw std::bad_alloc();

                uninitialized_fill (array, array + bytes, init);

                return array;
            }

            void destroy (T *object)
            {
                object->~T();

                mem_->deallocate (object, sizeof (T));
            }

            void destroyArray (size_t num, T *object)
            {
                T *obj = object, *end = object + num;
                while (obj != end) (obj++)->~T();

                mem_->deallocate (object, sizeof (T) * num);
            }

        private:
            Allocator   *mem_;
    };

    template <typename T, typename Allocator>
    class Pool
    {
        public:
            Pool () {}
    };
}

//=============================================================================
// Main entry point
int
main (int argc, char** argv)
{
    int i = 5;
    int *v[10];
    float f = 3.14;

    typedef Memory::StaticAllocation AllocationType;
    typedef Memory::LargeBlockAllocator<AllocationType> AllocatorType;
    typedef Object::Factory <A, AllocatorType> FactoryType;
    typedef typename FactoryType::SharedPtrType SharedPtrType;

    AllocationType allocation ("main", 1000);
    AllocationType suballoc ("suballoc", 50, &allocation);
    AllocatorType allocator ("textures", 400, &allocation);

    cout << "total available memory: " << allocation.free() << endl;
    cout << "available sub-allocated memory: " << suballoc.free() << endl;
    cout << "available large block memory: " << allocator.free() << endl;

    FactoryType factory (&allocator);
    A *a0 = factory.create ();
    A *a1 = factory.create (i);
    A *a2 = factory.create (i,f);
    cout << "available memory: " << factory.allocator()->free() << endl;

    A *array = factory.createArray (5, *a2);
    cout << "available memory: " << factory.allocator()->free() << endl;

    factory.destroy (a0);
    factory.destroy (a1);
    factory.destroy (a2);
    cout << "available memory: " << factory.allocator()->free() << endl;

    factory.destroyArray (5, array);
    cout << "available memory: " << factory.allocator()->free() << endl;

    factory.allocator()->defragment();
    cout << "available memory: " << factory.allocator()->free() << endl;

    SharedPtrType shared = factory.createShared (i,f);
    cout << "available memory: " << factory.allocator()->free() << endl;

    return 0;
}
