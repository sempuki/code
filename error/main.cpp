#include <iostream>

#include "hazard.hpp"

ctl::posix_domain posix;
ctl::win32_domain win32;

ctl::hazard foo() {
    return posix.raise(ctl::posix_condition::PERM,  //
                       std::source_location::current(),
                       "send help!");
}

ctl::hazard bar() {
    return win32.raise(ctl::win32_condition::ACCESS_DENIED,  //
                       std::source_location::current(),
                       "another one bits the dust.");
}

void print(ctl::hazard h) {
    std::cerr << "raise hazard " << h.message() << " at " << h.location().file_name() << ":"
              << h.location().line() << "\n";
}

int main() {
    auto no_permission0 = posix.expect(ctl::posix_condition::PERM);
    auto no_permission1 = win32.expect(ctl::win32_condition::ACCESS_DENIED);

    auto hazard0 = foo();
    if (hazard0 == no_permission0) {
        print(hazard0);
    }

    auto hazard1 = bar();
    if (hazard1 == no_permission1) {
        print(hazard1);
    }

    return 0;
}
