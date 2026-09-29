#include "catch2/catch_test_macros.hpp"
#include "course/soln_unique_ptr.hpp"

#include <memory>

// Each IT_* case is a Catch2 section. IT_CANNOT documents code that must not
// compile; its body is discarded.
#define IT_SHOULD(test_name, test_body) SECTION("it should " #test_name) test_body
#define IT_CAN(test_name, test_body) SECTION("it can " #test_name) test_body
#define IT_CANNOT(test_name, test_body)

struct Scope {
  static bool alive;
  Scope() { alive = true; }
  ~Scope() { alive = false; }
};

bool Scope::alive = false;

struct Foo {
  void bar() {}
};

// Run against the standard library to validate the tests themselves, and
// against the course solution to validate the solution.
#if defined(COURSE_TEST_REFERENCE)
template <typename T>
using type_under_test = std::unique_ptr<T>;
#else
template <typename T>
using type_under_test = soln::unique_ptr<T>;
#endif

TEST_CASE("unique_ptr") {
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
      CHECK(p == some{});
    });

    IT_SHOULD(not_be_empty_after_value_construction, {
      int *heap = new int;
      some p{heap};
      CHECK(p != some{});
    });

    IT_SHOULD(be_empty_after_null_construction, {
      some p{nullptr};
      CHECK(p == some{});
    });

    IT_SHOULD(be_not_empty_after_value_construction, {
      int *heap = new int;
      some p{heap};
      CHECK(p != some{});
    });

    IT_SHOULD(be_empty_after_assignment_initialization_from_default, {
      some p;
      some q = std::move(p);
      CHECK(q == some{});
    });

    IT_SHOULD(not_be_empty_after_assignment_initialization_from_value, {
      int *heap = new int;
      some p{heap};
      some q = std::move(p);
      CHECK(q != some{});
    });

    IT_SHOULD(be_empty_after_assignment_from_default, {
      some p;
      some q;
      q = std::move(p);
      CHECK(q == some{});
    });

    IT_SHOULD(not_be_empty_after_assignment_from_value, {
      int *heap = new int;
      some p{heap};
      some q;
      q = std::move(p);
      CHECK(q != some{});
    });

    IT_SHOULD(be_be_empty_after_move_from, {
      int *heap = new int;
      some p{heap};
      some q;
      q = std::move(p);
      CHECK(p == some{});
    });

    IT_SHOULD(have_value_address, {
      int *heap = new int;
      some p{heap};
      CHECK(p.get() == heap);
    });

    IT_SHOULD(release_ownership, {
      int *heap = new int;
      some p{heap};
      delete p.release();
      CHECK(p.get() == nullptr);
    });

    IT_SHOULD(transfer_ownership_on_move, {
      int *heap = new int;
      some p{heap};
      some q;

      CHECK(p.get() == heap);
      CHECK(q.get() == nullptr);

      q = std::move(p);

      CHECK(p.get() == nullptr);
      CHECK(q.get() == heap);
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
      CHECK(Scope::alive == false);

      Scope *heap = new Scope{};
      CHECK(Scope::alive == true);

      {
        some p{heap};
        CHECK(Scope::alive == true);
      }

      CHECK(Scope::alive == false);
    });

    IT_SHOULD(destroy_owned_value_on_reset, {
      CHECK(Scope::alive == false);

      Scope *heap = new Scope{};
      CHECK(Scope::alive == true);

      some p{heap};
      CHECK(Scope::alive == true);

      p.reset();
      CHECK(Scope::alive == false);
    });
    IT_SHOULD(destroy_owned_value_on_overwritten, {
      CHECK(Scope::alive == false);

      some p{new Scope{}};
      CHECK(Scope::alive == true);
      {
        some q{new Scope{}};
        CHECK(Scope::alive == true);

        p = std::move(q);
      }
      CHECK(Scope::alive == false);
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
