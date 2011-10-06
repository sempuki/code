#include <iostream>
#include <vector>
#include <stack>

using namespace std;

template <typename State>
class Machine
{
    public:
        typedef stack<Machine<State> *, vector<Machine<State> *> > Stack;

        Machine (Stack &stack) : stack_(stack) {}

        void start () { stack_.push(this); }
        void finish () { stack_.pop(); }

        virtual void run (State &state) = 0;

    private:
        Stack   &stack_;
};

struct PrintMachine : Machine<int>
{
    PrintMachine (Stack &stack) : Machine<int> (stack) {}

    void run (int &state)
    {
        cout << "print int: " << state << endl; 

        ++state;
        finish();
    }
};

struct MyMachine : Machine<int>
{
    PrintMachine print1, print2, print3;

    MyMachine (Stack &stack) : 
        Machine<int> (stack), 
        print1 (stack),
        print2 (stack),
        print3 (stack)
    {}

    void run (int &state)
    {
        switch (state)
        {
            case 0: print1.start(); break;
            case 1: print2.start(); break;
            case 2: print3.start(); break;
            case 3: finish(); break;
        }
    }
};

int main ()
{
    Machine<int>::Stack stack;
    Machine<int> *current;
    int state = 0;

    MyMachine machine (stack);
    machine.start();

    while (!stack.empty())
    {
        cout << ":" << stack.size() << endl;
        current = stack.top();
        current->run (state);
    }

    return 0;
}
