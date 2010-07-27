/* main.cpp
 *
 * Resource: Modern C++ Design by Andrei Alexandrescu
 *
 *			Ryan McDougall
 */

#include <iostream>
#include <algorithm>
#include <iterator>
#include <set>

#include <cassert>
#include <cmath>

//#include <main.h>
#include "ExampleClasses.h"

const int g_MaxObjectSize = 64;
const int g_MaxNumberOfObjectsInPool = 1000;
typedef unsigned char uint8_t;

namespace Memory
{

    // Chunk manages allocations for up to 2^sizeof(Index) objects of type T.
    // Ensure sizeof(Index) <= sizeof(T) to avoid memory overhead from Index.

    template <typename T, typename Index, size_t Size>
    class Chunk
    {
        // Block contains either a value of type T
        // or an index within an array that points
        // to the next available block; a linked list

        union Block
        {
            Index next;
            T value;
        };

        public:
            typedef std::set <Chunk *> Set;

            // allocate size blocks
            // space complexity: O(1)
            Chunk ()
            {
                // ensure Index can index Size
                assert ((Index)(~0) <= Size);

                // allocate size Blocks
                mem_ = new Block [Size];

                // linked list of available blocks
                for (size_t i=0; i < Size; ++i)
                    mem_[i].next = i+1;

                available_ = static_cast <Index> (Size);
                next_ = 0;
            } 

            // deallocate blocks
            ~Chunk ()
            {
                delete [] mem_;
            }

            // allocate one item of type T
            // time complexity: O(1)
            T *allocate ()
            {
                // check there is space available
                assert (available_);

                // get next available block
                // and check bounds
                Block *b = mem_ + next_;
                assert ((b >= mem_) && (b < mem_ + Size));

                // remove from available pool
                // advance to next available
                next_ = b->next;
                -- available_;
                
                return reinterpret_cast <T *> (b);
            }

            // deallocate one item of type T
            // time complexity: O(1)
            void deallocate (T *p)
            {
                using std::swap;

                // convert to block pointer
                // and check bounds
                Block *b = reinterpret_cast <Block *> (p); 
                assert ((b >= mem_) && (b < mem_ + Size));

                // compute block's index
                b->next = b - mem_;

                // add to available pool
                // and insert as next head
                swap (b->next, next_);
                ++ available_;

                // head must be in bounds
                assert (next_ < Size);
            }

            // return true if p was allocated by this chunk
            // time complexity: O(1)
            inline bool owns (T *p)
            {
                return ((Block *)p >= mem_) && ((Block *)p < mem_ + Size);
            }

            // return available space
            // time complexity: O(1)
            inline size_t available ()
            {
                return available_;
            }

            // return true if empty
            // time complexity: O(1)
            inline bool empty ()
            {
                return available_ == Size;
            }


            // debugging dump
            void print (std::ostream &out)
            {
                using std::copy;
                using std::ostream_iterator;

                T *begin = reinterpret_cast <T *> (mem_);
                T *end = reinterpret_cast <T *> (mem_ + Size);

                out << "next: " << (int) next_ << std::endl;
                out << "available: " << (int) available_ << std::endl;
                copy (begin, end, ostream_iterator <T> (out, ", "));
                out << std::endl;
            }

        private:
            Index   available_;
            Index   next_;

            Block   *mem_;
    };

    // Pool extends Chunk for dynamic allocation 
    // by creating new chunks on request.

    template <typename T, typename Index = uint8_t, size_t ChunkSize = 0xFF>
    class Pool
    {
        public:
            typedef Chunk <T, Index, ChunkSize> ChunkType;

            Pool (int reserve = 0)
            {
                // reserve a certain number of chunks
                const int N = (reserve)? 
                    ceil (static_cast <float> (reserve) / ChunkSize) : 1;

                for (int i = 0; i < N; ++i)
                    pool_.insert (new ChunkType);

                // for LRU/MRU use
                last_allocated_ = last_deallocated_ = *pool_.begin();
            }

            ~Pool ()
            {
                delete_chunks_ (pool_);
                delete_chunks_ (dead_);
            }

            T *get ()
            {
                // check latest for availability
                if (!last_allocated_->available ())
                {
                    typename ChunkType::Set::iterator i = pool_.begin();
                    typename ChunkType::Set::iterator e = pool_.end();
                    for (; i != e; ++i)
                    {
                        // found available chunk
                        if ((*i)->available ())
                        {
                            last_allocated_ = *i;
                            break;
                        }
                    }

                    // no available chunks found
                    if (i == e)
                    {
                        if (dead_.size())
                        {
                            // reuse dead chunk
                            pool_.insert (*dead_.begin());
                            dead_.erase (dead_.begin());
                        }
                        else
                            // allocate new chunk
                            pool_.insert (new ChunkType);
                    }
                }
                    
                return last_allocated_->allocate ();
            }

            void release (T *p)
            {
                // check latest for availability
                if (!last_deallocated_->owns (p))
                {
                    typename ChunkType::Set::iterator i = pool_.begin();
                    typename ChunkType::Set::iterator e = pool_.end();
                    for (; i != e; ++i)
                    {
                        // found owner chunk
                        if ((*i)->owns (p))
                        {
                            last_deallocated_ = *i;
                            break;
                        }
                    }

                    // check owner was found
                    assert (i != e);
                }
                    
                last_deallocated_->deallocate (p);

                // mark chunks dead if empty
                if (last_deallocated_->empty())
                {
                    dead_.insert (last_deallocated_);
                    pool_.erase (last_deallocated_);
                    last_allocated_ = *pool_.begin();
                }
                    
                // reclaim unused chunks
                if (dead_.size() > pool_.size())
                    delete_chunks_ (dead_);
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
            typename ChunkType::Set dead_;

            ChunkType   *last_allocated_;
            ChunkType   *last_deallocated_;
    };
}

using namespace std;

//=============================================================================
// Main entry point
int
main (int argc, char** argv)
{
    // only POD allowed in Unions!
    Memory::Pool <Base1> pool;

    Base1 *a = pool.get ();

    cout << "b1: " << a->GetNumber() << endl;

    pool.release (a);

    return 0;
}
