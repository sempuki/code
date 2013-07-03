#ifndef _PLATFORM_POSIX_ERROR_HPP_
#define _PLATFORM_POSIX_ERROR_HPP_

namespace ceres { namespace platform { namespace posix {

    void load_last_error_code (std::error_code &error);

} } }

#endif
