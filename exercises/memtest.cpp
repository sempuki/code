#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <list>

using namespace std;

struct Object
{
    int v;
    bool dead;

    Object () : v (-1), dead (false)
    { 
        //cout << "default constructor: (" << v << ") " << this << endl; 
    }

    Object (int i) : v (i), dead (false)
    {
        //cout << "init constructor: (" << v << ") " << this << endl; 
    }

    Object (Object const &o) : v (o.v), dead (false)
    { 
        //cout << "copy constructor: (" << v << ") " << this << " <- " << &o << endl; 
    }

    Object operator= (Object const &o)
    { 
        v = o.v; 
        dead = o.dead;
        //cout << "operator=: (" << v << ") " << this << " <- " << &o << endl; 
    }

    ~Object () 
    { 
        //cout << "destructor: " << this << endl; 
    }

};

struct ObjectDeadPred
{
    bool operator() (Object const *o) { return o->dead; }
};
    
struct Allocator
{
    // Cache-friendly O(1) time (and easily space) complexity allocator
    // Making this STL-ready is an exercise for the reader

    Allocator (size_t reserve)
    {
        // allocate room for all objects
        // ideally reserve is a multiple of cache line size
        memory.resize (reserve);

        // create free list containing all objects
        // optimization: free list can be embedded in unused memory 
        // (see Modern C++ Design [Alexandrescu])
        vector<Object>::iterator i = memory.begin(), e = memory.end();
        for (; i != e; ++i) free.push_back (&*i);
    }

    Object *allocate ()
    {
        if (free.empty())
            throw std::runtime_error ("out of memory");

        Object *o;
        o = free.front(); 
        free.pop_front();
        return o;
    }

    void deallocate (Object *o)
    {
        // if one is worried about compaction for use in DMA transfers
        // one can insert the freed object in sorted order at the cost of O(n) time complexity
        free.push_back (o);
    }

    list<Object *> free;
    vector<Object> memory;
};

int main (int argc, char **argv)
{
    int done = 0;
    bool alive = false;

    if (argc == 1)
    {
        cout << "method 1" << endl;

        vector<Object> work;
        int item, size;

        for (int frames=0; frames < 1000; ++frames)
        {
            // load work queue
            for (int i=0; i < 100; ++i)
                work.push_back (Object (frames));

            // dispatch work queue
            item = 0;
            size = work.size();
            while (item < size) 
            {
                if (alive = !alive)
                {
                    //cout << "do: " << work[item].v << endl;
                    swap (work[item], work[--size]);
                    work.erase (work.end()-1);
                    ++ done;
                }
                else
                    ++ item;
            }
        }
    }
    else
    {
        cout << "method 2" << endl;

        Allocator alloc (1000);
        vector<Object *> work;
        vector<Object *>::iterator item, dead, begin, end;
        Object *obj;

        for (int frames=0; frames < 1000; ++frames)
        {
            // init work queue
            for (int i=0; i < 100; ++i)
            {
                obj = alloc.allocate();
                obj->v = i;

                work.push_back (obj);
            }

            // dispatch work queue
            begin = work.begin(), end = work.end();
            
            for (item = begin; item != end; ++item)
            {
                if (alive = !alive)
                {
                    //cout << "do: " << (*item)->v << endl;
                    (*item)->dead = true;
                    ++ done;
                }
            }

            dead = remove_if (begin, end, ObjectDeadPred());

            for (item = dead; item != end; ++item)
                alloc.deallocate (*item);

            work.erase (dead, end);
        }
    }

    cout << "work done: " << done << endl;

    return 0;
}
