#include <cassert>
#include <vector>

#define IT_SHOULD(test_name, test_body)                                                            \
  struct it_should_##test_name {                                                                   \
    it_should_##test_name() { test(); }                                                            \
    void test() { test_body }                                                                      \
  } do_##test_name;
#define IT_CAN(test_name, test_body) IT_SHOULD(test_name, test_body)
#define IT_CANNOT(test_name, test_body) IT_SHOULD(test_name, {})

template <typename T> using type_under_test = std::vector<T>;

int main() {}
