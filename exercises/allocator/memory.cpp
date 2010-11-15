/* main.cpp
 *
 * Resource: Modern C++ Design by Andrei Alexandrescu
 *
 *			Ryan McDougall
 */

#include <iostream>
#include <algorithm>
#include <iterator>
#include <memory>
#include <set>

#include <cassert>
#include <cmath>

const int g_MaxObjectSize = 64;
const int g_MaxNumberOfObjectsInPool = 1000;
typedef unsigned char uint8_t;

namespace Memory
{
    // Chunk is a segment of memory divided into N uniform blocks.
    // Each block is space to hold one type of object T.
    // The first bytes of an unused block are an index pointing
    // to the next unused block. It is an implicit linked list
    // that keeps operation within constant time complexity.
    // Index must be chosen such that sizeof(Index) <= sizeof(T).
    // Size is how many objects T will be allocated per chunk.
    // For correct indexing, Size must be chosen such that
    // Size < 2^(sizeof(Index)*8).

    template <typename T, typename Index, Index Size>
    struct Chunk 
    {
        typedef std::set <Chunk *> Set;

        // Pointer to allocated memory
        uint8_t *memory;

        // Index to number of available and next available
        Index   available;
        Index   next;

        // allocate size blocks
        // space complexity: O(1)
        Chunk ()
        {
            // Check Chunk constraints. (todo: static_assert)
            // ensure Index is smaller than T and can index Size
            assert (sizeof(Index) <= sizeof(T));
            assert (Size <= ((Index) ~0));

            // allocate enough bytes for Size blocks
            memory = new uint8_t [sizeof(T) * Size];

            // init linked list of available blocks
            uint8_t *b = memory; 
            for (size_t i = 1; i < Size-1; ++i, increment (b))
                index (b) = i;
            index (b) = 0;

            // init number of available and next available
            available = Size;
            next = 0;
        } 

        // deallocate blocks
        ~Chunk ()
        {
            delete [] memory;
        }

        // allocate one object T
        // time complexity: O(1)
        T *allocate ()
        {
            // check there is space available
            assert (available);

            // get next available block
            // and check bounds
            uint8_t *b = memory;
            advance (b, next);
            assert (owns (b));

            // remove from available pool
            // set index to next available
            next = index (b);
            -- available;

            // head must be in bounds
            assert (next < Size);

            return object (b);
        }

        // deallocate one object T
        // time complexity: O(1)
        void deallocate (T *o)
        {
            using std::swap;

            // convert to block pointer
            // and check bounds
            uint8_t *b = block (o);
            assert (owns (b));

            // compute block's index
            index (b) = distance (memory, b);

            // add to available pool
            // and insert as next head
            swap (index (b), next);
            ++ available;

            // head must be in bounds
            assert (next < Size);
        }

        // return pointer to object T's block
        inline uint8_t *block (T *o)
        {
            return reinterpret_cast <uint8_t *> (o);
        }

        // return pointer to block b's object T
        inline T *object (uint8_t *b)
        {
            return reinterpret_cast <T *> (b);
        }

        // return reference to block b's embedded index
        inline Index &index (uint8_t *b)
        {
            return *(reinterpret_cast <Index *> (b));
        }

        // return the block ith block from b
        inline void increment (uint8_t *&b)
        {
            b += sizeof(T);
        }

        // return the block ith block from b
        inline void advance (uint8_t *&b, Index i)
        {
            b += (sizeof(T) * i);
        }

        // return the index distance between a, b (a < b)
        inline Index distance (uint8_t *a, uint8_t *b)
        {
            return (b - a) / sizeof(T);
        }

        // return true if b was allocated by this chunk
        inline bool owns (uint8_t *b)
        {
            return (b >= memory) && 
                (b < memory + (sizeof(T) * Size));
        }

        // return true if b was allocated by this chunk
        inline bool owns (T *b)
        {
            return ((uint8_t *)b >= memory) && 
                ((uint8_t *)b < memory + (sizeof(T) * Size));
        }

        // return true if empty
        inline bool empty ()
        {
            return available == Size;
        }

        // return true if full
        inline bool full ()
        {
            return available == 0;
        }


        // debugging dump
        void print (std::ostream &out)
        {
            using std::copy;
            using std::ostream_iterator;

            out << "next: " << (int) next << std::endl;
            out << "available: " << (int) available << std::endl;
            copy (memory, memory+Size, ostream_iterator <T> (out, ", "));
            out << std::endl;
        }
    };

    // Pool extends Chunk for dynamic allocation by creating new chunks 
    // on request. The caller is responsible for calling the constructor
    // (via placement new) and the destructor manually.

    template <typename T, typename Index = uint8_t, Index ChunkSize = 0xFF>
    class Pool
    {
        public:
            typedef T ObjectType;
            typedef Chunk <T, Index, ChunkSize> ChunkType;

            Pool (int reserve = 0) : 
                last_allocated_ (0), 
                last_deallocated_ (0)
            {
                // reserve a certain number of chunks
                const int N = (reserve)? 
                    ceil (static_cast <float> (reserve) / ChunkSize) : 1;

                for (int i = 0; i < N; ++i)
                    pool_.insert (new ChunkType);
            }

            ~Pool ()
            {
                delete_chunks_ (pool_);
                delete_chunks_ (empty_);
            }

            // time complexity: 
            //  worst case: O(N) in the number of chunks
            //  best case: O(1) for MRU
            T *get ()
            {
                std::cout << "get" << std::endl;

                // check MRU for availability
                if (!last_allocated_ || last_allocated_->full())
                {
                    typename ChunkType::Set::iterator i = pool_.begin();
                    typename ChunkType::Set::iterator e = pool_.end();
                    for (; i != e; ++i)
                    {
                        // found available chunk
                        if ((*i)->available)
                        {
                            last_allocated_ = *i;
                            break;
                        }
                    }

                    // no available chunks found
                    if (i == e)
                    {
                        if (empty_.size())
                        {
                            // reuse dead chunk
                            last_allocated_ = *empty_.begin();
                            empty_.erase (empty_.begin());
                        }
                        else
                            // allocate new chunk
                            last_allocated_ = new ChunkType;

                        // insert newly available
                        pool_.insert (last_allocated_);
                    }
                }
                    
                return last_allocated_->allocate ();
            }

            // time complexity: 
            //  worst case: O(N) in the number of chunks
            //  best case: O(1) for MRU
            void release (T *b)
            {
                std::cout << "release" << std::endl;

                // check MRU for availability
                if (!last_deallocated_ || !last_deallocated_->owns (b))
                {
                    typename ChunkType::Set::iterator i = pool_.begin();
                    typename ChunkType::Set::iterator e = pool_.end();
                    for (; i != e; ++i)
                    {
                        // found owner chunk
                        if ((*i)->owns (b))
                        {
                            last_deallocated_ = *i;
                            break;
                        }
                    }

                    // check owner was found
                    assert (i != e);
                }
                    
                last_deallocated_->deallocate (b);

                // collect empty chunks
                if (last_deallocated_->empty())
                {
                    empty_.insert (last_deallocated_);
                    pool_.erase (last_deallocated_);
                    last_allocated_ = *pool_.begin();
                }
                    
                // reclaim unused chunks
                if (pool_.size() && empty_.size() > pool_.size())
                    delete_chunks_ (empty_);
            }

            void print (std::ostream &out)
            {
                typename ChunkType::Set::iterator i = pool_.begin();
                typename ChunkType::Set::iterator e = pool_.end();

                for (; i != e; ++i)
                {
                    (*i)->print (out);
                    out << std::endl;
                }
            }

        private:
            void delete_chunks_ (typename ChunkType::Set &set)
            {
                typename ChunkType::Set::iterator i = set.begin();
                typename ChunkType::Set::iterator e = set.end();

                for (; i != e; ++i)
                    delete *i;

                set.clear ();
            }

        private:
            typename ChunkType::Set pool_;
            typename ChunkType::Set empty_;

            ChunkType   *last_allocated_;   // MRU allocation chunk
            ChunkType   *last_deallocated_; // MRU deallocation chunk
    };

    template <typename T, typename MemoryPool = Pool<T> >
    class PooledObject
    {
        public:
            PooledObject () : pool_ (0) {}
            virtual ~PooledObject () {}

            static void *operator new (size_t size, MemoryPool *pool) 
            {
                // not suitable for our pool
                if (!pool || size != sizeof(typename MemoryPool::ObjectType) 
                        || size > g_MaxObjectSize)
                    return operator new (size);

                if (!size) size = 1;
                void *memory = 0;

                //for (;;)
                {
                    // allocate from pool
                    memory = pool->get ();

                    if (!memory)
                    {
                        // should never happen (??)
                        // try to get more memory from new_handler
                        std::new_handler handle = std::set_new_handler (0);
                        std::set_new_handler (handle);
                        if (!handle) throw std::bad_alloc ();
                        else handle ();
                    }
                }
            }

            static void operator delete (void *p, MemoryPool *pool) 
            {
                // not suitable for our pool
                if (!pool) return operator delete (p);

                // release to pool
                if (p) pool->release (p);
            }

            static void *operator new (size_t size) { return ::operator new (size); }
            static void operator delete (void *p) { return ::operator delete (p); }

        protected:
            MemoryPool *pool_;
    };

    template <typename T, typename MemoryPool>
    void destroy (PooledObject <T, MemoryPool> *obj, MemoryPool *pool)
    {
        obj->~PooledObject ();
        operator delete (obj, pool);
    }

    // Object Oriented front-end used for objects allocated directly
    // from a given memory pool.

    template <typename T, typename MemoryPool = Pool<T> >
    class Factory
    {
        public:
            Factory ()
                : mem_ (new MemoryPool) {}

            Factory (std::auto_ptr <MemoryPool> pool)
                : mem_ (pool) {}

            T *create ()
            {
                return new (mem_->get()) T;
            }

            template <typename U>
            T *create (const U &init)
            {
                return new (mem_->get()) T (init);
            }

            void destroy (T *item)
            {
                item->~T();
                mem_->release (item);
            }

        private:
            Factory (const Factory &);

        private:
            std::auto_ptr <MemoryPool>  mem_;
    };
}

#include "ExampleClasses.h"

using namespace std;

//=============================================================================
// Main entry point
int
main (int argc, char** argv)
{
    Memory::Factory <Base1> factory;
    Memory::Pool <Base2> *pool = new Memory::Pool <Base2>;

    Base1 *a = factory.create ();
    cout << "factory b1: " << a->GetNumber() << endl;
    factory.destroy (a);

    //Base2 *b2 = new (pool) Base2;
    //destroy (b2, pool);

    return 0;
}
