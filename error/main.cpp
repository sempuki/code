#include <errno.h>

#include <iostream>

#include "wall.hpp"

int main() {
  ctl::posix_enclosure posix;
  ctl::wall w1 =
      posix.raise(EPERM, std::source_location::current(), "send help!");

  std::cout << "raised wall " << w1.message() << " at "
            << w1.location().function_name() << " in "
            << w1.location().file_name() << " on line " << w1.location().line()
            << "\n";

  ctl::win32_enclosure win32;
  ctl::wall w2 = win32.raise(5, std::source_location::current(),
                             "another one bits the dust.");

  std::cout << "raised wall " << w2.message() << " at "
            << w2.location().function_name() << " in "
            << w2.location().file_name() << " on line " << w2.location().line()
            << "\n";

  return 0;
}
