#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <list>
#include <cstdlib>

using namespace std;

void dowork (int &v)
{
    v /= atoi("13");
    v -= rand() % 314159;
}

void mutate (int &d, int **p)
{
    d += atoi("123456789");
    d *= rand() % 0xFFFF;
    *p = &d, **p /= 17;
}

struct Object
{
    bool dead;
    int v, d, *p;

    Object () : v (-1), dead (true)
    { 
        mutate (d, &p);
        //cout << "default constructor: (" << v << ") " << this << endl; 
    }

    Object (int i) : v (i), dead (false)
    {
        mutate (d, &p);
        //cout << "init constructor: (" << v << ") " << this << endl; 
    }

    Object (Object const &o) : v (o.v), dead (o.dead)
    { 
        mutate (d, &p);
        //cout << "copy constructor: (" << v << ") " << this << " <- " << &o << endl; 
    }

    Object operator= (Object const &o)
    { 
        v = o.v; 
        dead = o.dead;

        mutate (d, &p);
        //cout << "operator=: (" << v << ") " << this << " <- " << &o << endl; 
    }

    void init (int v, bool dead)
    {
        this->v = v; 
        this->dead = dead;

        mutate (d, &p);
        //cout << "init method: " << this << endl;
    }

    ~Object () 
    { 
        //cout << "destructor: " << this << endl; 
    }

};

struct Allocator
{
    Allocator (size_t reserve)
    {
        memory.resize (reserve);

        vector<Object>::iterator i = memory.begin(), e = memory.end();
        for (; i != e; ++i) free.push_back (&*i);
    }

    Object *allocate ()
    {
        if (free.empty()) 
            throw runtime_error ("out of memory");

        Object *o;
        o = free.back(); 
        free.pop_back();

        return o;
    }

    void deallocate (Object *o)
    {
        free.push_back (o);
    }

    vector<Object *> free;
    vector<Object> memory;
};

struct ObjectDead
{
    Allocator *alloc;
    ObjectDead (Allocator *a) : alloc (a) {}

    bool operator() (Object *o) 
    { 
        if (o->dead) alloc->deallocate (o);
        return o->dead;
    }
};
    
int main (int argc, char **argv)
{
    bool alive = false;

    if (argc == 1)
    {
        cout << "method 1" << endl;

        vector<Object> work;
        int item, size;

        for (int frames=0; frames < 10; ++frames)
        {
            // load work queue
            for (int i=0; i < 1000000; ++i)
                work.push_back (Object (frames));

            // dispatch work queue
            item = 0;
            size = work.size();
            while (item < size) 
            {
                if (alive = !alive)
                {
                    dowork (work[item].v);

                    swap (work[item], work[--size]);
                    work.erase (work.end()-1);
                }
                else
                    ++ item;
            }
        }
    }
    else
    {
        cout << "method 2" << endl;

        Allocator alloc (2000000);
        vector<Object *> work;
        vector<Object *>::iterator item, begin, end;
        Object *obj;

        for (int frames=0; frames < 10; ++frames)
        {
            // init work queue
            for (int i=0; i < 1000000; ++i)
            {
                obj = alloc.allocate();
                obj->init (frames, false);

                work.push_back (obj);
            }

            // dispatch work queue
            begin = work.begin(), end = work.end();
            for (item = begin; item != end; ++item)
            {
                if (alive = !alive)
                {
                    dowork ((*item)->v);

                    (*item)->dead = true;
                }
            }

            work.erase (remove_if (begin, end, ObjectDead (&alloc)), end);
        }
    }

    return 0;
}
