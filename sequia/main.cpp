#include <iostream>
#include <algorithm>

#include <cstdint>

#include "memory.hpp"
#include "stream.hpp"
#include "container.hpp"

using namespace std;
using namespace sequia;

namespace App
{
    class Test
    {
        public:
            Test(int a, float b, char c)
                : a_(a), b_(b), c_(c) {}

            template <typename T>
            bool serialize (stream::stream<T> *s) const
            {
                *s << a_ << b_ << c_;
            }

            template <typename T>
            bool deserialize (stream::stream<T> *s)
            {
                *s >> a_ >> b_ >> c_;
            }

        private:
            int     a_;
            float   b_;
            char    c_;
    };
}

namespace traits
{
    namespace stream
    {
        template <>
        struct element <App::Test>
        {
            typedef custom_serializable_tag serialization;
        };
    }
}


int main(int argc, char **argv)
{
    size_t N = sizeof(App::Test) * 10;
    uint8_t buf[N];

    stream::stream<uint8_t> stream (buf, N);
    App::Test in (1, 0.1, 'a');
    App::Test out (0, 0.0, 0);
    
    stream << in;
    stream >> out;

    container::fixedvector<int, 10> vec;
    
    for (int i=0; i < 10; ++i)
        vec.push_back (i);

    for (int i=0; i < 10; ++i)
        cout << vec[i] << endl;

    container::fixedmap<int, int, 10> map;

    for (int i=0; i < 10; i++)
        map[i] = i;

    for (int i=0; i < 10; i++)
        cout << map[i] << endl;

    size_t  M = 10;
    size_t  allocvec[M]; // 10 allocations
    int     allocmem[M]; // 10 ints

    memory::linear_allocator<int> alloc (allocmem, M, allocvec, M);
    int *p = alloc.allocate(5);
    int *q = alloc.allocate(5);

    for (int i=0; i < 5; ++i)
        *p++ = *q++ = i;

    for (int i=0; i < M; ++i)
        cout << allocmem[i] << endl;

    return 0; 
}
