#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <list>
#include <cstdlib>
#include <ctime>

using namespace std;

const int FRAMES = 10;
const int ITEMS = 10000000;
int workdone;

void dowork (int &v)
{
    v /= atoi("13");
    v -= rand() % 314159;
    ++workdone;
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
    }

    Object (int i) : v (i), dead (false)
    {
        mutate (d, &p);
    }

    Object (Object const &o) : v (o.v), dead (o.dead)
    { 
        mutate (d, &p);
    }

    Object operator= (Object const &o)
    { 
        v = o.v; 
        dead = o.dead;

        mutate (d, &p);
    }

    void init (int v, bool dead)
    {
        this->v = v; 
        this->dead = dead;

        mutate (d, &p);
    }

    ~Object () 
    { 
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

void method2()
{
    time_t start_time = time(NULL);
    int cnt = 0;
    workdone = 0;
    Allocator alloc(ITEMS);
    vector<Object*> v;

    for (int n=0; n < FRAMES; ++n)
    {
        // grow the work list
        while (v.size() < ITEMS)
        {
            Object *obj = alloc.allocate();
            obj->init (cnt++, false);
            v.push_back(obj);
        }

        // operate on the work list
        for(vector<Object*>::iterator i=v.begin();i!=v.end();++i)
        {
            int n = rand()%100;
            if (n<33)
            {
                dowork((*i)->v);
                (*i)->dead = true;
            }
        }

        // reclaim dead work items
        v.erase (remove_if (v.begin(), v.end(), ObjectDead (&alloc)), v.end());
    }

    time_t end_time = time(NULL);
    cout << "method2" << endl;
    cout << "Work Done: " << workdone << endl;
    cout << "Total: " << difftime(end_time,start_time)  << "s" << endl;
}

void method1()
{
    time_t start_time = time(NULL);
    int cnt = 0;
    workdone = 0;
    vector<Object> v;

    for (int n=0; n < FRAMES; ++n)
    {
        // grow the work list
        while (v.size() < ITEMS)
        {
            v.push_back(Object(cnt++));
        }

        // operate on the work list
        for (int i=0; i < v.size();)
        {
            int n = rand()%100;
            if (n<33)
            {
                dowork(v[i].v);
                v[i] = v.back();
                v.pop_back();
            }
            else ++i;
        }
    }

    time_t end_time = time(NULL);
    cout << "method1" << endl;
    cout << "Work Done: " << workdone << endl;
    cout << "Total: " << difftime(end_time,start_time)  << "s" << endl;
}

int main(int argc, char* argv[])
{
    time_t t = time(NULL);
    srand(t);
    method1();
    srand(t);
    method2();
    return 0;
}
