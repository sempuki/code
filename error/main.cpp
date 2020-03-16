#include <errno.h>

#include <iostream>

#include "wall.hpp"

int main() {
  ctl::posix_enclosure enclosure;
  ctl::wall wall =
      enclosure.raise(EPERM, std::source_location::current(), "send help!");
  std::cout << "raised wall " << wall.message() << " at "
            << wall.location().function_name() << " in "
            << wall.location().file_name() << "\n";
  return 0;
}
