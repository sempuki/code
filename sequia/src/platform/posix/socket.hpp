#ifndef _PLATFORM_POSIX_SOCKET_HPP_
#define _PLATFORM_POSIX_SOCKET_HPP_

#include <sys/socket.h>
#include <netinet/in.h>

namespace platform { namespace posix { namespace socket {

    using handle_type = int;
    using size_type = socklen_t;
    using address_type = sockaddr_in;

    static const handle_type INVALID = -1;

    bool try_string_to_inet4 (std::string const &str, address_type &addr);
    bool try_inet4_to_string (address_type const &addr, std::string &str);

    bool try_open (int family, int type, int protocol, int &handle);
    bool try_close (handle_type &handle);
    
    bool try_bind (handle_type handle, sockaddr const *addr, size_type size);
    bool try_connect (handle_type handle, sockaddr const *addr, size_type size);

    bool try_listen (handle_type handle, int backlog);
    bool try_accept (handle_type handle, sockaddr *addr, size_type &size, 
            handle_type &accepted);

    bool try_get_local_address (handle_type handle, sockaddr *addr, size_type &size);
    bool try_get_remote_address (handle_type handle, sockaddr *addr, size_type &size);

} } }

#endif
