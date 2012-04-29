#include <iostream>
#include <algorithm>

#include <cstdint>

#include "memory.hpp"
#include "stream.hpp"
#include "container.hpp"

using namespace std;

namespace App
{
    class Test
    {
        public:
            Test(int a, float b, char c)
                : a_(a), b_(b), c_(c) {}

            template <typename T>
            bool serialize (sequia::stream<T> *s) const
            {
                *s << a_ << b_ << c_;
            }

            template <typename T>
            bool deserialize (sequia::stream<T> *s)
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
    template <>
    struct element <App::Test>
    {
        typedef custom_serializable_tag serialization;
    };
}


int main(int argc, char **argv)
{
    size_t N = sizeof(App::Test) * 10;
    uint8_t buf[N];

    sequia::stream<uint8_t> stream (buf, N);
    App::Test in (1, 0.1, 'a');
    App::Test out (0, 0.0, 0);
    
    stream << in;
    stream >> out;

    sequia::fixedvector<10, int> vec;
    
    for (int i=0; i < 10; ++i)
        vec.push_back (i);

    for (int i=0; i < 10; ++i)
        cout << vec[i] << endl;

    sequia::fixedmap<10, int, int> map;

    for (int i=0; i < 10; i++)
        map[i] = i;

    for (int i=0; i < 10; i++)
        cout << map[i] << endl;

    return 0; 
}
