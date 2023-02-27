// Copyright 2023 Ryan McDougall
#pragma once

#include <exception>
#include <iostream>

#define ASSERT(predicate) \
  if (!(predicate)) {     \
    std::terminate();     \
  }
#define EXPECT(predicate) \
  if (!(predicate)) {     \
    throw predicate;      \
  }
#define IT_CAN(test_name, test_body)      \
  const auto it_can_##test_name = [&]() { \
    test_body                             \
  };
#define IT_SHOULD(test_name, test_body)        \
  const auto it_should_##test_name = [&]() {   \
    test_body;                                 \
  };                                           \
  try {                                        \
    std::cerr << "Test it_should_##test_name"; \
    it_should_##test_name();                   \
    std::cerr << " passed.\n";                 \
  } catch (...) {                              \
    std::cerr << " failed.\n";                 \
  }

