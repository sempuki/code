#include <iostream>

#include "hazard.hpp"

ctl::posix_domain posix{0x1};
ctl::win32_domain win32{0x2};

ctl::hazard foo() {
    return posix.raise(ctl::posix_domain::condition::PERM, "send help!");
}

ctl::hazard bar() {
    return win32.raise(ctl::win32_domain::condition::ACCESS_DENIED, "another one bits the dust.");
}

int main() {
    auto hazard = foo();
    return 0;
}
