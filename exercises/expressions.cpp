#include <iostream>
#include <algorithm>
#include <vector>

enum class operation
{
    mul, add, nop
};

struct expression
{
    std::vector<operation>  operations;
    std::vector<int>        operands;

    void clearop (operation op)
    {
        auto b = begin (operations);
        auto e = end (operations);

        operations.erase (std::remove (b, e, op), e);
    }
};

void print (expression const &expr)
{
    int nextop = 0;
    for (auto v : expr.operands)
    {
        std::cout << ' ' << v << ' ';

        if (nextop < expr.operations.size())
            switch (expr.operations[nextop++])
            {
                case operation::mul:
                    std::cout << '*'; break;
                case operation::add:
                    std::cout << '+'; break;
                case operation::nop:
                    std::cout << '_'; break;
            }
    }

    std::cout << std::endl;
}

std::vector<expression> generate (expression const &current, int index)
{
    std::vector<expression> result;

    if (index == 0)
        result.push_back (current);

    else
    {
        expression with_mul {current.operations, {}};
        expression with_add {current.operations, {}};
        expression with_nop {current.operations, {}};

        with_mul.operations.push_back (operation::mul);
        with_add.operations.push_back (operation::add);
        with_nop.operations.push_back (operation::nop);

        auto a = generate (with_mul, index-1);
        auto b = generate (with_add, index-1);
        auto c = generate (with_nop, index-1);

        result.insert (end (result), begin (a), end (a));
        result.insert (end (result), begin (b), end (b));
        result.insert (end (result), begin (c), end (c));
    }

    return result;
}

void gather (std::string const &digits, expression &expr)
{
    int result = 0;
    int nextop = 0;

    for (auto ch : digits)
    {
        result *= 10;
        result += static_cast<int> (ch - '0');

        if (expr.operations[nextop] != operation::nop)
        {
            expr.operands.push_back (result);
            result = 0;
        }

        nextop++;
    }

    expr.clearop (operation::nop);
}

int evaluate (expression const &expr)
{
    int result = 0;
    expression simplified;

    result = expr.operands[0];
    for (int i=0, j=1; i < expr.operations.size(); ++i)
    {
        auto op = expr.operations[i];
        if (op == operation::mul)
            result *= expr.operands[j++];

        else
        {
            simplified.operands.push_back (result);
            simplified.operations.push_back (op);
            result = expr.operands[j++];
        }
    }

    simplified.operands.push_back (result);

    result = simplified.operands[0];
    for (int i=0, j=1; i < simplified.operations.size(); ++i)
        result += simplified.operands[j++];

    return result;
}

bool test (std::string const &digits, int sum)
{
    bool found = false;

    expression blank;
    auto permutations = generate (blank, digits.size()-1);

    for (auto &expr : permutations)
    {
        gather (digits, expr);

        if (found = (evaluate (expr) == sum))
        {
            print (expr);
            break;
        }
    }

    return found;
}

int main ()
{
    test ("1231231234", 11353);
    test ("3456237490", 1185);
    test ("3456237490", 9191);

    return 0;
}
