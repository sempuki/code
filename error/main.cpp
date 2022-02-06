#include <iostream>

#include "error.hpp"

ctl::posix_domain posix;
ctl::win32_domain win32;

ctl::error foo() {
  return posix.raise(ctl::posix_condition::PERM,  //
                     std::source_location::current(),
                     "send help!");
}

ctl::error bar() {
  return win32.raise(ctl::win32_condition::ACCESS_DENIED,  //
                     std::source_location::current(),
                     "another one bites the dust.");
}

void print(ctl::error e) {
  std::cerr << "raise error " << e.message() << " at " << e.location().file_name() << ":"
            << e.location().line() << "\n";
}

int main() {
  auto no_permission0 = posix.expect(ctl::posix_condition::PERM);
  auto no_permission1 = win32.expect(ctl::win32_condition::ACCESS_DENIED);

  auto error0 = foo();
  if (error0 == no_permission0) {
    print(error0);
  }

  auto error1 = bar();
  if (error1 == no_permission1) {
    print(error1);
  }

  return 0;
}
