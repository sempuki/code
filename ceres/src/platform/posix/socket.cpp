#include <core/standard.hpp>

#include <unistd.h>
#include <arpa/inet.h>

#include <platform/posix/socket.hpp>
#include <platform/posix/error.hpp>

namespace ceres { namespace platform { namespace posix { namespace socket {

    bool try_string_to_inet4 (std::string const &str, address_type &addr)
    {
        using std::string;
        using std::stoul; // WARN: can throw

        string host;
        size_t portmark = str.rfind (':');

        host = str.substr (0, portmark);
        if (portmark != string::npos) 
            addr.sin_port = htons (stoul (str.substr (portmark + 1)));

        int result = inet_pton (AF_INET, host.c_str(), &addr.sin_addr);
        bool success = result > 0;

        return success;
    }

    bool try_inet4_to_string (address_type const &addr, std::string &str)
    {
        using std::to_string; // WARN: can throw

        char buf [INET6_ADDRSTRLEN];

        char const *result = inet_ntop (AF_INET, &addr.sin_addr, buf, sizeof (buf));
        bool success = result != nullptr;

        if (success)
        {
            str = buf; 
            str += ':';
            str += to_string (ntohs (addr.sin_port));
        }

        return success;
    }

    bool try_open (int family, int type, int protocol, int &handle)
    {
        using ::socket;

        int result = socket (family, type, protocol);
        bool success = result >= 0;

        if (success)
            handle = result;

        return success;
    }

    bool try_close (handle_type &handle)
    {
        using ::close;

        handle_type result = close (handle);
        bool success = result == 0;

        if (success)
            handle = INVALID;

        return success;
    }

    bool try_bind (handle_type handle, sockaddr const *addr, size_type size)
    {
        using ::bind;

        handle_type result = bind (handle, addr, size);
        bool success = result != INVALID;

        return success;
    }

    bool try_connect (handle_type handle, sockaddr const *addr, size_type size)
    {
        using ::connect;

        handle_type result = connect (handle, addr, size);
        bool success = result != INVALID;

        return success;
    }

    bool try_listen (handle_type handle, int backlog)
    {
        using ::listen;

        handle_type result = listen (handle, backlog);
        bool success = result != INVALID;

        return success;
    }

    bool try_accept (handle_type handle, sockaddr *addr, size_type &size, 
            handle_type &accepted)
    {
        using ::accept;

        handle_type result = accept (handle, addr, &size);
        bool success = result != INVALID;

        if (success)
            accepted = result;

        return success;
    }

    bool try_get_local_address (handle_type handle, sockaddr *addr, size_type &size)
    {
        using ::getsockname;

        handle_type result = getsockname (handle, addr, &size);
        bool success = result != INVALID;

        return success;
    }

    bool try_get_remote_address (handle_type handle, sockaddr *addr, size_type &size)
    {
        using ::getpeername;

        handle_type result = getpeername (handle, addr, &size);
        bool success = result != INVALID;

        return success;
    }

} } } }
