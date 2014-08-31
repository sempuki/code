#include <iostream>
#include <tuple>

using namespace std;

struct layer1
{
  int on_push(int v) { cout << "layer1 on_push: " << v << endl; return ++v; }
  int on_pop(int v) { cout << "layer1 on_pop: " << v << endl; return --v; }
};

struct layer2
{
  int on_push(int v) { cout << "layer2 on_push: " << v << endl; return ++v; }
  int on_pop(int v) { cout << "layer2 on_pop: " << v << endl; return --v; }
};

struct layer3
{
  int on_push(int v) { cout << "layer3 on_push: " << v << endl; return ++v; }
  int on_pop(int v) { cout << "layer3 on_pop: " << v << endl; return --v; }
};

template <typename T, typename ...Layers>
class stack 
{
  public:
    void push(T value) { push_<N-1, Layers...>(value); }
    void pop(T value) { pop_<0, Layers...>(value); }

  private:
    template<size_t Index, typename Current, typename ...Rest>
      T push_(T value) 
      { 
        return push_<Index-1, Rest...>(std::get<Index>(layers).on_push(value)); 
      }

    template<size_t Index>
      T push_(T value) { return value; }
  
  private:
    template<size_t Index, typename Current, typename ...Rest>
      T pop_(T value) 
      { 
        return std::get<Index>(layers).on_pop(pop_<Index+1, Rest...>(value)); 
      }

    template<size_t Index>
      T pop_(T value) { return value; }
  
  private:
    std::tuple<Layers...> layers;
    static constexpr size_t N = std::tuple_size<decltype(layers)>::value;
};

int main ()
{
  stack<int, layer1, layer2, layer3> s;
  s.push(5);
  s.pop(5);
  return 0;
}
