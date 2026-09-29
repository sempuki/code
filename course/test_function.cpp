#include "catch2/catch_test_macros.hpp"
#include "course/soln_function.hpp"

#include <functional>

// Each IT_* case is a Catch2 section. IT_CANNOT documents code that must not
// compile; its body is discarded.
#define IT_SHOULD(test_name, test_body) SECTION("it should " #test_name) test_body
#define IT_CAN(test_name, test_body) SECTION("it can " #test_name) test_body
#define IT_CANNOT(test_name, test_body)

#define IT_CAN_BE_DEFAULT_COPY_AND_MOVE_CONSTRUCTED \
    IT_CAN(be_default_constructed, { some f; });    \
    IT_CAN(be_move_constructed, {                   \
        some f;                                     \
        some g{std::move(f)};                       \
    });                                             \
    IT_CAN(be_move_assigned, {                      \
        some f;                                     \
        some g;                                     \
        g = std::move(f);                           \
    });                                             \
    IT_CAN(be_copy_constructeded, {                 \
        some f;                                     \
        some g{f};                                  \
    });                                             \
    IT_CAN(be_copy_assigned, {                      \
        some f;                                     \
        some g;                                     \
        g = f;                                      \
    });

// Run against the standard library to validate the tests themselves, and
// against the course solution to validate the solution.
#if defined(COURSE_TEST_REFERENCE)
template <typename T>
using type_under_test = std::function<T>;
#else
template <typename T>
using type_under_test = soln::function<T>;
#endif

int count = 0;
const auto l = [capture = 1](int v) mutable -> int {
    ::count += v;
    capture += v;
    return capture;
};

int fun(int v) {
    ::count += v;
    return ::count;
}

TEST_CASE("function") {
    {
        using some = type_under_test<void()>;
        IT_CAN_BE_DEFAULT_COPY_AND_MOVE_CONSTRUCTED
    }
    {
        using some = type_under_test<void(float, int)>;
        IT_CAN_BE_DEFAULT_COPY_AND_MOVE_CONSTRUCTED
    }
    {
        using some = type_under_test<int()>;
        IT_CAN_BE_DEFAULT_COPY_AND_MOVE_CONSTRUCTED
    }
    {
        using some = type_under_test<bool(const int&, float&)>;
        IT_CAN_BE_DEFAULT_COPY_AND_MOVE_CONSTRUCTED
    }
    {
        using some = type_under_test<int(int)>;

        IT_SHOULD(invoke_free_function, {
            count = 0;
            some f{fun};

            f(5);
            CHECK(count == 5);
        });

        IT_SHOULD(invoke_free_function_twice, {
            count = 0;
            some f{fun};

            f(5);
            CHECK(count == 5);

            f(6);
            CHECK(count == 11);
        });

        IT_SHOULD(invoke_lambda, {
            count = 0;
            some f{l};

            f(5);
            CHECK(count == 5);
        });

        IT_SHOULD(invoke_lambda_twice, {
            count = 0;
            some f{l};

            f(5);
            CHECK(count == 5);

            f(6);
            CHECK(count == 11);
        });

        IT_SHOULD(invoke_original_lambda_after_copy, {
            count = 0;

            some f{l};
            f(5);
            CHECK(count == 5);

            some g{f};
            f(6);
            CHECK(count == 11);
        });

        IT_SHOULD(invoke_copied_lambda_after_copy, {
            count = 0;
            some f{l};
            some g{f};

            f(5);
            CHECK(count == 5);

            g(6);
            CHECK(count == 11);
        });

        IT_SHOULD(make_distinct_copies, {
            some f{l};
            some g{f};

            CHECK(f(5) == 6);
            CHECK(g(6) == 7);
            CHECK(f(5) == 11);
            CHECK(g(6) == 13);
        });
    }
}
