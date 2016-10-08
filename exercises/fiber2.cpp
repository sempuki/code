#include <cxxabi.h>
#include <array>
#include <atomic>
#include <cassert>
#include <deque>
#include <iostream>
#include <map>
#include <thread>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <vector>

inline std::string demangle(const std::string& name) {
    size_t size = 1024;  // we *don't* want realloc() to be called
    char buffer[size];   // NOLINT

    int status = 0;
    abi::__cxa_demangle(name.c_str(), buffer, &size, &status);
    assert(status == 0 && "Demanging failed");

    return buffer;
}

template <typename Functional, typename Tuple, size_t... Index>
decltype(auto) apply_helper(Functional&& functional, Tuple&& tuple,
                            std::index_sequence<Index...>) {
    return functional(std::get<Index>(std::forward<Tuple>(tuple))...);
}

template <typename Functional, typename Tuple>
decltype(auto) apply(Functional&& functional, Tuple&& tuple) {
    constexpr auto Size = std::tuple_size<std::decay_t<Tuple>>::value;
    return apply_helper(std::forward<Functional>(functional),
                        std::forward<Tuple>(tuple),
                        std::make_index_sequence<Size>{});
};

template <typename Type>
struct function_traits_helper {};

template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits_helper<ReturnType (ClassType::*)(Args...) const> {
    using class_type = ClassType;
    using result_type = ReturnType;
    using argument_tuple = std::tuple<Args...>;
    enum { arity = sizeof...(Args) };

    template <size_t i>
    struct arg {
        typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
        // the i-th argument is equivalent to the i-th tuple element of a tuple
        // composed of those arguments.
    };
};

template <typename Type>
struct function_traits
    : public function_traits_helper<decltype(&std::decay_t<Type>::operator())> {
};

struct Fiber {
    Fiber() {
        arguments = &b0;
        result = &b1;
    }

    template <typename Functional>
    Fiber& setup_run(Functional&& work) {
        using ArgTuple = typename function_traits<Functional>::argument_tuple;
        using ResultType = typename function_traits<Functional>::result_type;

        assert(sizeof(ArgTuple) <= arguments->size() && "Argument buffer overrun");
        assert(sizeof(ResultType) <= result->size() && "Result buffer overrun");
        std::cout << demangle(typeid(ArgTuple).name()) << std::endl;

        first_type_code = typeid(ArgTuple).hash_code();
        next_type_code = typeid(ResultType).hash_code();

        auto args = reinterpret_cast<ArgTuple*>(arguments->data());
        auto res = reinterpret_cast<ResultType*>(result->data());
        std::swap(arguments, result);

        work_list.push_back([args, res, work] { *res = apply(work, *args); });
        return *this;
    }

    template <typename Functional>
    Fiber& then_do(Functional&& work) {
        using ArgTuple = typename function_traits<Functional>::argument_tuple;
        using ResultType = typename function_traits<Functional>::result_type;

        assert(sizeof(ArgTuple) <= arguments->size() && "Argument buffer overrun");
        assert(sizeof(ResultType) <= result->size() && "Result buffer overrun");

        assert(typeid(ArgTuple).hash_code() == next_type_code && "Argument type chain mismatch");
        next_type_code = typeid(ResultType).hash_code();

        auto args = reinterpret_cast<ArgTuple*>(arguments->data());
        auto res = reinterpret_cast<ResultType*>(result->data());
        std::swap(arguments, result);

        work_list.push_back([args, res, work] { *res = apply(work, *args); });
        return *this;
    }

    template <typename... Args>
    void start_run(Args&&... args) {
        new (arguments) std::tuple<Args...>(std::forward<Args>(args)...);
    }

    void run_once() {
        work_list[work_index++]();
    }

    std::vector<std::function<void()>> work_list;
    size_t work_index = 0;

    using ParamBuffer = std::array<uint8_t, 1024>;
    ParamBuffer b0, b1;
    ParamBuffer* arguments = nullptr;
    ParamBuffer* result = nullptr;

    size_t first_type_code = 0;
    size_t next_type_code = 0;
};



int main() {
    Fiber fiber;
    fiber
        .setup_run([](int a, const char* b) {
            std::cout << "Set up." << std::endl;
            std::cout << b << std::endl;
            return std::make_tuple(42, false);
        })
        .then_do([](int a, bool success) {
            std::cout << "Then do 1" << std::endl;
            std::cout << a << std::endl;
            if (success) {
                std::cout << "success" << std::endl;
            } else {
                std::cout << "fail" << std::endl;
            }
            return true;
        })
        .start_run(5, "hello");

    fiber.run_once();
    fiber.run_once();
}
