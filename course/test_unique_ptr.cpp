#include <cassert>
#include <memory>

#define IT_SHOULD(test_name, test_body) \
  struct it_should_##test_name {        \
    it_should_##test_name() { test(); } \
    void test() { test_body }           \
  } do_##test_name;
#define IT_CAN(test_name, test_body) IT_SHOULD(test_name, test_body)
#define IT_CANNOT(test_name, test_body) IT_SHOULD(test_name, {})

struct Scope {
  static bool alive;
  Scope() { alive = true; }
  ~Scope() { alive = false; }
};

bool Scope::alive = false;

struct Foo {
  void bar() {}
};

template <typename T>
using type_under_test = std::unique_ptr<T>;

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

    IT_CANNOT(be_copy_constructeded, {
      some p;
      some q{p};
    });

    IT_CANNOT(be_copy_assigned, {
      some p;
      some q;
      q = p;
    });

    IT_SHOULD(be_empty_after_default_construction, {
      some p;
      assert(p == some{});
    });

    IT_SHOULD(not_be_empty_after_value_construction, {
      int *heap = new int;
      some p{heap};
      assert(p != some{});
    });

    IT_SHOULD(be_empty_after_null_construction, {
      some p{nullptr};
      assert(p == some{});
    });

    IT_SHOULD(be_not_empty_after_value_construction, {
      int *heap = new int;
      some p{heap};
      assert(p != some{});
    });

    IT_SHOULD(be_empty_after_assignment_initialization_from_default, {
      some p;
      some q = std::move(p);
      assert(q == some{});
    });

    IT_SHOULD(not_be_empty_after_assignment_initialization_from_value, {
      int *heap = new int;
      some p{heap};
      some q = std::move(p);
      assert(q != some{});
    });

    IT_SHOULD(be_empty_after_assignment_from_default, {
      some p;
      some q;
      q = std::move(p);
      assert(q == some{});
    });

    IT_SHOULD(not_be_empty_after_assignment_from_value, {
      int *heap = new int;
      some p{heap};
      some q;
      q = std::move(p);
      assert(q != some{});
    });

    IT_SHOULD(be_be_empty_after_move_from, {
      int *heap = new int;
      some p{heap};
      some q;
      q = std::move(p);
      assert(p == some{});
    });

    IT_SHOULD(have_value_address, {
      int *heap = new int;
      some p{heap};
      assert(p.get() == heap);
    });

    IT_SHOULD(release_ownership, {
      int *heap = new int;
      some p{heap};
      delete p.release();
      assert(p.get() == nullptr);
    });

    IT_SHOULD(transfer_ownership_on_move, {
      int *heap = new int;
      some p{heap};
      some q;

      assert(p.get() == heap);
      assert(q.get() == nullptr);

      q = std::move(p);

      assert(p.get() == nullptr);
      assert(q.get() == heap);
    });

    IT_CANNOT(release_ownership_from_const, {
      int *heap = new int;
      const some p{heap};
      delete p.release();
    });

    IT_CANNOT(transfer_ownership_on_move_from_const, {
      const some p;
      some q;
      q = std::move(p);
    });

    IT_CANNOT(transfer_ownership_on_move_to_const, {
      some p;
      const some q;
      q = std::move(p);
    });
  }
  {
    using some = type_under_test<Scope>;

    IT_SHOULD(destroy_owned_value_on_scope_exit, {
      assert(Scope::alive == false);

      Scope *heap = new Scope{};
      assert(Scope::alive == true);

      {
        some p{heap};
        assert(Scope::alive == true);
      }

      assert(Scope::alive == false);
    });

    IT_SHOULD(destroy_owned_value_on_reset, {
      assert(Scope::alive == false);

      Scope *heap = new Scope{};
      assert(Scope::alive == true);

      some p{heap};
      assert(Scope::alive == true);

      p.reset();
      assert(Scope::alive == false);
    });
    IT_SHOULD(destroy_owned_value_on_move, {
      assert(Scope::alive == false);

      some p{new Scope{}};
      assert(Scope::alive == true);

      some q{new Scope{}};
      assert(Scope::alive == true);

      p = std::move(q);
      assert(Scope::alive == false);
    });
  }
  {
    using some = type_under_test<Foo>;

    IT_CAN(star_dereference_owned_value, {
      some p{new Foo};
      (*p).bar();
    });

    IT_CAN(arrow_dereference_owned_value, {
      some p{new Foo};
      p->bar();
    });
  }
}
