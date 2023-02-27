// Copyright 2023 Ryan McDougall
#pragma once

#include <iostream>
#include <source_location>

#define HERE                                                                                  \
  if constexpr (simplelog_on) {                                                               \
    [&](auto &&loc__) {                                                                       \
      std::cerr << "-- " << loc__.file_name() << "(" << loc__.line() << ":" << loc__.column() \
                << ") `" << loc__.function_name() << "`\n";                                   \
    }(std::source_location::current());                                                       \
  }
