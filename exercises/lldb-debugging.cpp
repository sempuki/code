#include <algorithm>
#include <iostream>
#include <vector>

#define SHHHH (int*)92374

std::string level;
int* pointer = new int[5];

struct Test
{
    std::string value = "default";
    std::vector<int> list = {1, 2, 3, 4};
    float pi = 3.14;
};

void sneaky()
{
    // totally nothing abnormal going on here
    // really
    // you should trust me.
    // would I lie to you?
    pointer = SHHHH;
    // see. totaly safe.
}

void deeper()
{
    sneaky();
}

void baz()
{
    level = "much baz";
}

void bar()
{
    delete[] pointer;
    pointer = new int[5];
    deeper();
}

void foo()
{
    bar();
    baz();
    pointer[0] = 5;
}

void harmless()
{
    foo();
}

void use_args(char** args)
{
    for (; *args; args++)
    {
        std::cout << *args << std::endl;
    }

    pointer[1] = 1;
}

const std::string LART = "LART";
void use_env(char** env)
{
    for (int count = 0; *env; env++, count++)
    {
        if (std::equal(begin(LART), end(LART), *env))
        {
            std::cout << *env << std::endl;
        }
    }

    pointer[2] = 2;
}

int main(int argc, char** argv, char** env)
{
    use_args(argv);
    use_env(env);

    Test value;
    harmless();
}

/*
# run the program with parameters
env LART=yes
run --awesomeify

# orient yourself
th list
bt
fr select 1
fr var
ta var
r

# examine the faulty line
p pointer[0]
p pointer
p *(int*)0x00000000000168d6
memory read --size 4 --format x --count 5 0x00000000000168d6

# restart for stepping
br set -n ma<tab>
br set -n main
display pointer
r

# examine main
list
p pointer[0]

# examine use_args
th step-in
th step-over
<enter> x5
p pointer[1]
th step-over

# examine use_env
th step-in
th until 77
ex pointer[2] = 5
p pointer[2]
th step-out
th step-over
<enter> x2

# use break/watch points
br delete 1
br set -l 47 -c 'pointer == 0x168d6'
r
br disable 2
br se -n foo
r
wa set var pointer
c
wa modify -c 'pointer < 0x100000000'
c
br delete
br set -l 23
r
th step-over

# fin
*/
