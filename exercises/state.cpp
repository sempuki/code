#include <iostream>
#include <vector>
#include <stack>

using namespace std;

template <typename StateType>
class Machine
{
    public:
        typedef StateType State;
        typedef std::stack<Machine<State> *, std::vector<Machine<State> *>> Stack;

        Machine (Stack &stack) : stack_ (stack) {}

        void start () { stack_.push (this); entered (); }
        void finish () { stack_.pop (); exited (); }

        virtual void entered () const {}
        virtual void exited () const {}

        virtual void turn (State &state) = 0;

    private:
        Stack   &stack_;
};

template <typename Machine>
class Engine
{
    public:
        typedef typename Machine::State State;
        typedef typename Machine::Stack Stack;

        Engine() : machine_ (stack_) { machine_.start(); }

        bool turn (State &state)
        {
            stack_.top()->turn (state);
            return !stack_.empty();
        }

    private:
        Stack   stack_;
        Machine machine_;
};

class PrintMachine : public Machine<int>
{
    public:
        PrintMachine (Stack &stack) : Machine<int> (stack) {}

        void entered () const { cout << "current state: "; }
        void exited () const { cout << endl; }
        
        void turn (int &state) { cout << state; ++state; finish(); }
};

class MyMachine : public Machine<int>
{
    public:
        enum { STATE1, STATE2, STATE3, STATE4 };

        MyMachine (Stack &stack) : 
            Machine<int> (stack), 
            printer1_ (stack),
            printer2_ (stack),
            printer3_ (stack) {}

        void turn (int &globalstate)
        {
            switch (globalstate)
            {
                case 0: localstate_ = STATE1; break;
                case 1: localstate_ = STATE2; break;
                case 2: localstate_ = STATE3; break;
                case 3: localstate_ = STATE4; break;
            }

            switch (localstate_)
            {
                case STATE1: printer1_.start(); break;
                case STATE2: printer2_.start(); break;
                case STATE3: printer3_.start(); break;
                case STATE4: finish(); break;
            }
        }

    private:
        PrintMachine printer1_;
        PrintMachine printer2_;
        PrintMachine printer3_;
        int localstate_;
};

int main ()
{
    int state = 0;
    Engine <MyMachine> statemachine;

    while (statemachine.turn (state));

    return 0;
}
