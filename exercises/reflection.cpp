#include <iostream>
#include <type_traits>
#include <vector>
#include <map>

template <typename Type>
struct has_find_method
{
    // Use the declared type of function declarations (return type) to test using SFINAE: the more
    // specific declaration will be selected if the decltype is valid, and deleted if it's invalid.
    // Functions cannot be overloaded by return type, so we pass a neutral parameter value (nullptr).
    // Further expression validity detection types can be created by replacing the expression in the
    // found case decltype before the sequencing operator (operator,).

    // Found case: function declaration deleted if Container::find(Container::key_type) is invalid
    template <typename Container>
    static auto test(std::nullptr_t)
        -> decltype(std::declval<Container>().find(std::declval<typename Container::key_type>()),
                    std::true_type());

    // Base case: declares function that matches all invocations
    template <typename Container>
    static auto test(...) -> std::false_type;

    using type = decltype(test<Type>(nullptr));
    static constexpr bool value = decltype(test<Type>(nullptr))::value;
};

template <typename MapType, typename KeyType>
std::enable_if_t<has_find_method<MapType>::value, bool>
contains(const MapType& map, const KeyType& key)
{
    std::cout << "map contains" << std::endl;
    return map.find(key) != map.end();
}

template <typename ListType, typename ItemType>
std::enable_if_t<!has_find_method<ListType>::value, bool>
contains(const ListType& list, const ItemType& item)
{
    std::cout << "map contains" << std::endl;
    return std::find(list.begin(), list.end(), item) != list.end();
}

int main()
{
    std::vector<int> list;
    std::map<int, std::string> map;

    list.push_back(5);
    map[5] = "hello";

    std::cout << "test 1: " << contains(map, 5) << std::endl;
    std::cout << "test 2: " << contains(map, 6) << std::endl;
    std::cout << "test 3: " << contains(list, 5) << std::endl;
    std::cout << "test 4: " << contains(list, 6) << std::endl;
}
