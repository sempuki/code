#include <cassert>
#include <cstddef>
#include <vector>

#define IT_SHOULD(test_name, test_body) \
  struct it_should_##test_name {        \
    it_should_##test_name() { test(); } \
    void test() { test_body }           \
  } do_##test_name;
#define IT_CAN(test_name, test_body) IT_SHOULD(test_name, test_body)
#define IT_CANNOT(test_name, test_body) IT_SHOULD(test_name, {})

template <typename T>
using type_under_test = std::vector<T>;

template <typename T>
void setup_sequence(type_under_test<T> &p, size_t I, size_t N) {
  for (size_t i = I; i < N; ++i) {
    p.push_back(i);
  }
}

#define verify_sequence(p, I, N)   \
  for (size_t i = I; i < N; ++i) { \
    assert(p[i] == i);             \
  }

int main() {
  {
    using some = type_under_test<int>;

    IT_CAN(be_default_constructed, { some p; });

    IT_CAN(be_move_constructed, {
      some p;
      some q{std::move(p)};
    });

    IT_CAN(be_move_assigned, {
      some p;
      some q;
      q = std::move(p);
    });

    IT_CAN(be_copy_constructeded, {
      some p;
      some q{p};
    });

    IT_CAN(be_copy_assigned, {
      some p;
      some q;
      q = p;
    });

    IT_SHOULD(be_empty_after_default_construction, {
      some p;
      assert(p == some{});
    });

    IT_SHOULD(not_have_size_after_default_construction, {
      some p;
      assert(p.size() == 0);
    });

    IT_SHOULD(not_be_empty_after_push_back, {
      some p;
      p.push_back(5);
      assert(p != some{});
    });

    IT_SHOULD(have_size_after_push_back, {
      some p;
      p.push_back(5);
      assert(p.size() != 0u);
    });

    IT_SHOULD(have_capacity_after_push_back, {
      some p;
      p.push_back(5);
      assert(p.capacity() != 0u);
    });

    IT_SHOULD(have_front_after_push_back, {
      some p;
      p.push_back(5);
      assert(p.front() == 5);
    });

    IT_CAN(have_mutable_front_when_mutable, {
      some p;
      p.push_back(5);
      p.front() = 6;
      assert(p.front() == 6);
    });

    IT_CANNOT(have_mutable_front_when_const, {
      some p;
      p.push_back(5);
      const some &q = p;
      q.front() = 6;
    });

    IT_SHOULD(have_back_after_push_back, {
      some p;
      p.push_back(5);
      assert(p.back() == 5);
    });

    IT_CAN(have_mutable_back_when_mutable, {
      some p;
      p.push_back(5);
      p.back() = 6;
      assert(p.back() == 6);
    });

    IT_CANNOT(have_mutable_back_when_const, {
      some p;
      p.push_back(5);
      const some &q = p;
      q.front() = 6;
    });

    IT_SHOULD(have_different_front_and_back_after_two_push_back, {
      some p;
      p.push_back(5);
      p.push_back(6);
      assert(p.front() != p.back());
    });

    IT_SHOULD(have_index_zero_after_push_back, {
      some p;
      p.push_back(5);
      assert(p[0] == 5);
    });

    IT_CAN(have_mutable_index_when_mutable, {
      some p;
      p.push_back(5);
      p[0] = 6;
      assert(p[0] == 6);
    });

    IT_CANNOT(have_mutable_index_when_const, {
      some p;
      p.push_back(5);
      const some &q = p;
      q[0] = 6;
    });

    IT_SHOULD(have_index_size_minus_one_after_push_back, {
      some p;
      p.push_back(5);
      assert(p[p.size() - 1] == 5);
    });

    IT_SHOULD(have_zero_and_one_after_two_push_back, {
      some p;
      p.push_back(5);
      p.push_back(6);
      assert(p[0] == 5);
      assert(p[1] == 6);
    });

    IT_SHOULD(resize, {
      some p;
      p.resize(5);
      assert(p.size() == 5);
    });

    IT_SHOULD(increase_capacity_if_necessary, {
      some p;
      auto capacity = p.capacity();
      p.resize(capacity + 1);
      assert(p.capacity() > capacity);
    });

    IT_SHOULD(retain_values_when_resizing_larger, {
      some p;
      p.push_back(5);
      p.push_back(6);
      p.resize(5);
      assert(p[0] == 5);
      assert(p[1] == 6);
    });

    IT_SHOULD(retain_values_when_resizing_smaller, {
      some p;
      p.push_back(5);
      p.push_back(6);
      p.resize(1);
      assert(p[0] == 5);
    });

    IT_SHOULD(be_able_to_grow_to_size_n, {
      for (size_t N : {10, 100, 1000, 10000, 100000}) {
        some p;
        setup_sequence(p, 0, N);
        assert(p.size() == N);
      }
    });

    IT_SHOULD(be_able_to_hold_list_of_n, {
      for (size_t N : {10, 100, 1000, 10000, 100000}) {
        some p;
        setup_sequence(p, 0, N);
        verify_sequence(p, 0, N);
      }
    });

    const size_t N = 100;

    IT_CAN(be_use_in_range_based_for_loop, {
      some p;
      for (auto v : p) {
        //
      }
    });

    IT_SHOULD(be_able_to_iterate_list_in_range_based_for_loop, {
      some p;
      setup_sequence(p, 0, N);

      int i = 0;
      for (auto v : p) {
        assert(v == i++);
      }
    });

    IT_SHOULD(have_same_values_after_copy_construction, {
      some p;
      setup_sequence(p, 0, N);
      some q{p};
      assert(p == q);
    });

    IT_SHOULD(have_same_values_after_copy_assignment, {
      some p;
      some q;
      setup_sequence(p, 0, N);
      q = p;
      assert(p == q);
    });

    IT_SHOULD(retain_values_after_copy_construction, {
      some p;
      setup_sequence(p, 0, N);
      some q{p};
      verify_sequence(p, 0, N);
    });

    IT_SHOULD(retain_values_after_copy_assignment, {
      some p;
      some q;
      setup_sequence(p, 0, N);
      q = p;
      verify_sequence(p, 0, N);
    });

    IT_SHOULD(transmit_values_to_constructed_copy, {
      some p;
      setup_sequence(p, 0, N);
      some q{p};
      verify_sequence(q, 0, N);
    });

    IT_SHOULD(transmit_values_to_assigned_copy, {
      some p;
      some q;
      setup_sequence(p, 0, N);
      q = p;
      verify_sequence(q, 0, N);
    });

    IT_SHOULD(have_same_values_after_move_construction, {
      some p;
      setup_sequence(p, 0, N);
      auto copy = p;
      some q{std::move(p)};
      assert(q == copy);
    });

    IT_SHOULD(have_same_values_after_move_assignment, {
      some p;
      some q;
      setup_sequence(p, 0, N);
      auto copy = p;
      q = std::move(p);
      assert(q == copy);
    });

    IT_SHOULD(transmit_values_to_construced_move, {
      some p;
      setup_sequence(p, 0, N);
      some q{std::move(p)};
      verify_sequence(q, 0, N);
    });

    IT_SHOULD(transmit_values_to_assigned_move, {
      some p;
      some q;
      setup_sequence(p, 0, N);
      q = std::move(p);
      verify_sequence(q, 0, N);
    });
  }
}
