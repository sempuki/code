#include <iostream>

#include "hazard.hpp"

int main() {
    ctl::posix_domain posix;
    ctl::hazard w1 =
        posix.raise(ctl::posix_domain::kind::PERM, std::source_location::current(), "send help!");

    std::cout << "raised hazard " << w1.message() << " at " << w1.location().function_name()
              << " in " << w1.location().file_name() << " on line " << w1.location().line() << "\n";

    ctl::win32_domain win32;
    ctl::hazard w2 = win32.raise(ctl::win32_domain::kind::ACCESS_DENIED,
                                 std::source_location::current(),
                                 "another one bits the dust.");

    std::cout << "raised hazard " << w2.message() << " at " << w2.location().function_name()
              << " in " << w2.location().file_name() << " on line " << w2.location().line() << "\n";

    std::cout << "posix system id: " << w1.system() << "\n";
    std::cout << "win32 system id: " << w2.system() << "\n";

    return 0;
}
