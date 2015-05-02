#ifndef SRC_CORE_ALGORITHMS_HPP_
#define SRC_CORE_ALGORITHMS_HPP_

namespace core {

template <typename Type>
struct const_projection {
  const Type value;
  template <typename ...Args>
  Type operator()(Args &&...) const { return value; }
};

static const const_projection<bool> always {true};
static const const_projection<bool> never {false};

template<typename Container, typename Type>
size_t remove_erase(Container &cont, const Type &item) {  // NOLINT
  auto iter = std::remove(std::begin(cont), std::end(cont), item);
  auto count = std::distance(iter, std::end(cont));
  cont.erase(iter, std::end(cont));
  return count;
}

template<typename Container, typename Predicate>
size_t remove_erase_if(Container &cont, Predicate &&pred) {  // NOLINT
  auto iter = std::remove_if(std::begin(cont), std::end(cont), pred);
  auto count = std::distance(iter, std::end(cont));
  cont.erase(iter, std::end(cont));
  return count;
}

template<typename Container, typename Type>
bool contains(Container &cont, const Type &item) {  // NOLINT
  return std::find(std::begin(cont), std::end(cont), item) != end(cont);
}

template<typename Container, typename Predicate>
bool contains_if(Container &cont, Predicate &&pred) {  // NOLINT
  return std::find_if(std::begin(cont), std::end(cont), pred)!= end(cont);
}

}  // namespace core


#endif
