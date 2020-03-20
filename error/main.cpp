#include <iostream>

#include "gate.hpp"

int main() {
  ctl::posix_system posix;
  ctl::gate w1 = posix.raise(ctl::posix_system::kind::PERM,
                             std::source_location::current(), "send help!");

  std::cout << "raised gate " << w1.message() << " at "
            << w1.location().function_name() << " in "
            << w1.location().file_name() << " on line " << w1.location().line()
            << "\n";

  ctl::win32_system win32;
  ctl::gate w2 = win32.raise(ctl::win32_system::kind::ACCESS_DENIED,
                             std::source_location::current(),
                             "another one bits the dust.");

  std::cout << "raised gate " << w2.message() << " at "
            << w2.location().function_name() << " in "
            << w2.location().file_name() << " on line " << w2.location().line()
            << "\n";

  std::cout << "posix system id: " << w1.system() << "\n";
  std::cout << "win32 system id: " << w2.system() << "\n";

  return 0;
}
