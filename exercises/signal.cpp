#include <iostream>
#include <functional>
#include <iterator>
#include <vector>

template <typename ...Args>
class signal
{
  public:
    signal &operator=(std::nullptr_t)
    {
      delegates_.clear();
      return *this;
    }

    signal &operator=(signal that)
    {
      delegates_ = std::move(that.delegates_);
      return *this;
    }

    template <typename Functional>
    signal &operator+=(Functional callback)
    {
      delegates_.push_back(std::move(callback));
      return *this;
    }

    signal &operator+=(signal that)
    {
      delegates_.insert(std::end(delegates_), 
          std::make_move_iterator(std::begin(that.delegates_)), 
          std::make_move_iterator(std::end(that.delegates_)));
      return *this;
    }

    void operator()(Args ...args) 
    {
      for (auto &delegate : delegates_)
        if (delegate) delegate(args...);
    }

  private:
    std::vector<std::function<void(Args...)>> delegates_;
};

struct Foo
{
  double var;
};

void func(int a, char b, const std::string &c)
{
  std::cout << "hello 2: " << a << " " << b << " " << c << std::endl;
}

struct Test
{
  void operator()(std::string a, const Foo &b) 
  {
    std::cout << "hello 3: " << a << " " << b.var << std::endl;
  }
};

int main(int argc, char **argv) 
{
  Foo test { 69.69 };
  signal<int, char, std::string> sig1;
  sig1 += [test](int a, char b, std::string c) 
    { 
      std::cout << "hello 1: " << a << " " << b << " " << c << " test: " << test.var 
        << std::endl;
    };
  sig1 += func;

  signal<std::string, Foo> sig2;
  sig2 += Test{};

  std::string s = "testing";
  sig1(42, 'a', s);
  std::cout << "s: " << s << std::endl;
  sig2("wtf", test);

  return 0;
}
