#include <cassert>
#include <functional>

#define IT_SHOULD(test_name, test_body)     \
    struct it_should_##test_name {          \
        it_should_##test_name() { test(); } \
        void test() { test_body }           \
    } do_##test_name;
#define IT_CAN(test_name, test_body) IT_SHOULD(test_name, test_body)
#define IT_CANNOT(test_name, test_body) IT_SHOULD(test_name, {})

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

template <typename T>
using type_under_test = std::function<T>;

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

int main() {
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
            assert(count == 5);
        });

        IT_SHOULD(invoke_free_function_twice, {
            count = 0;
            some f{fun};

            f(5);
            assert(count == 5);

            f(6);
            assert(count == 11);
        });

        IT_SHOULD(invoke_lambda, {
            count = 0;
            some f{l};

            f(5);
            assert(count == 5);
        });

        IT_SHOULD(invoke_lambda_twice, {
            count = 0;
            some f{l};

            f(5);
            assert(count == 5);

            f(6);
            assert(count == 11);
        });

        IT_SHOULD(invoke_original_lambda_after_copy, {
            count = 0;

            some f{l};
            f(5);
            assert(count == 5);

            some g{f};
            f(6);
            assert(count == 11);
        });

        IT_SHOULD(invoke_copied_lambda_after_copy, {
            count = 0;
            some f{l};
            some g{f};

            f(5);
            assert(count == 5);

            g(6);
            assert(count == 11);
        });

        IT_SHOULD(make_distinct_copies, {
            some f{l};
            some g{f};

            assert(f(5) == 6);
            assert(g(6) == 7);
            assert(f(5) == 11);
            assert(g(6) == 13);
        });
    }
}
