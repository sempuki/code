#include <iostream>
#include <sstream>
#include <thread>
#include <system_error>

#include <cstring>
#include <cerrno>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace std;

struct scoped_thread
{
    std::thread thread;

    scoped_thread (std::thread t) 
        : thread {std::move (t)} {}

    scoped_thread (scoped_thread const &) = delete;
    scoped_thread &operator= (scoped_thread const &) = delete;
    
    ~scoped_thread ()
    {
        if (thread.joinable ())
            thread.join ();
    }
};

namespace io { namespace net 
{
namespace platform 
{ 
    namespace posix { namespace socket 
    {
        using handle_type = int;
        using size_type = socklen_t;

        static const handle_type INVALID = -1;

        bool try_string_to_inet4 (std::string const &str, sockaddr_in &addr)
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

        bool try_inet4_to_string (sockaddr_in const &addr, std::string &str)
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
    }}

    namespace posix { namespace system 
    {
        void load_last_error_code (std::error_code &error)
        {
            return error.assign (errno, std::system_category ()); 
        }
    }}
}

namespace detail
{
    // TODO: platform::windows et al.
    // TODO: #define the correct platform into namespace
    using namespace platform::posix;
}

class socket
{
    public:
        using size_type = detail::socket::size_type;
        using handle_type = detail::socket::handle_type;
        static const handle_type INVALID = detail::socket::INVALID;

        enum class type { NONE, TCP, UDP }; // TODO: VDP et al.

        class address
        {
            public:
                address ()
                {
                    std::memset (&address_, 0, sizeof (address_));
                    address_.sin_family = AF_INET;
                    error_ = std::errc::bad_address;
                }

                address (address const &other) = default;
                address &operator= (address const &other) = default;
                ~address () = default;

            public:
                address (std::string const &addr)
                {
                    if (!detail::socket::try_string_to_inet4 (addr, address_))
                        detail::system::load_last_error_code (error_);
                }

                operator std::string () const 
                {
                    std::string result;
                    detail::socket::try_inet4_to_string (address_, result);
                    return result;
                }

                operator sockaddr* () 
                { 
                    return reinterpret_cast<sockaddr *> (&address_); 
                }

                operator sockaddr const* () const
                { 
                    return reinterpret_cast<sockaddr const *> (&address_); 
                }

                size_type sockaddr_size () const
                {
                    return sizeof (address_);
                }

            public:
                operator bool () const { return !error_; }
                std::error_code error () const { return error_; }

            public:
                bool has_same_port (address const &other) const
                {
                    return address_.sin_port == other.address_.sin_port;
                }

                bool has_same_host (address const &other) const
                {
                    return address_.sin_addr.s_addr == other.address_.sin_addr.s_addr;
                }

                bool operator== (address const &other) const 
                {
                    return has_same_port (other) && has_same_host (other);
                }

            public:
                uint16_t port () const { return ntohs (address_.sin_port); }
                void set_port (uint16_t port) { address_.sin_port = htons (port); }

                uint32_t host () const { return ntohl (address_.sin_addr.s_addr); }
                void set_host (uint32_t host) { address_.sin_addr.s_addr = htonl (host); }

            private:
                sockaddr_in     address_;
                std::error_code error_;
        };

    public:
        socket () = default;
        socket (socket const &other) = default;
        socket &operator= (socket const &socket) = default;

        socket (socket &&other) :
            socket {other} 
        { 
            other.invalidate (); 
        }

        socket &operator= (socket &&other) 
        {
            *this = other;
            other.invalidate ();
        }

        socket (type kind) { open (kind); }
        ~socket () { close (); }

    public:
        void invalidate () { handle_ = INVALID; }
        socket::type get_type () const { return type_; }

    public:
        std::error_code open (type kind)
        {
            int sockfam, socktype, sockproto;
            map_socket_type (kind, sockfam, socktype, sockproto);

            if (!detail::socket::try_open (sockfam, socktype, sockproto, handle_))
                detail::system::load_last_error_code (error_);

            return error_;
        }

        std::error_code close ()
        {
            if (!detail::socket::try_close (handle_))
                detail::system::load_last_error_code (error_);

            invalidate ();

            return error_;
        }

        std::error_code bind (address const &local)
        {
            auto addr = (sockaddr const *) local;
            auto size = local.sockaddr_size ();

            if (!detail::socket::try_bind (handle_, addr, size))
                detail::system::load_last_error_code (error_);

            return error_;
        }

        std::error_code connect (address const &remote)
        {
            auto addr = (sockaddr const *) remote;
            auto size = remote.sockaddr_size ();

            if (!detail::socket::try_connect (handle_, addr, size))
                detail::system::load_last_error_code (error_);

            return error_;
        }

    public:
        std::error_code listen (int backlog = 8)
        {
            if (!detail::socket::try_listen (handle_, backlog))
                detail::system::load_last_error_code (error_);

            return error_;
        }

        std::error_code accept (socket &accepted)
        {
            address remote;
            auto addr = (sockaddr *) remote;
            auto size = remote.sockaddr_size ();

            if (!detail::socket::try_accept (handle_, addr, size, accepted.handle_))
                detail::system::load_last_error_code (error_);

            return error_;
        }

    public:
        address local_address ()
        {
            address local;
            auto addr = (sockaddr *) local;
            auto size = local.sockaddr_size ();

            if (!detail::socket::try_get_local_address (handle_, addr, size))
                detail::system::load_last_error_code (error_);

            return local;
        }

        address remote_address ()
        {
            address remote;
            auto addr = (sockaddr *) remote;
            auto size = remote.sockaddr_size ();

            if (!detail::socket::try_get_remote_address (handle_, addr, size))
                detail::system::load_last_error_code (error_);

            return remote;
        }

    public:
        operator bool () const { return !error_; }
        std::error_code error () const { return error_; }

    private:
        void map_socket_type (type kind, int &sockfam, int &socktype, int &sockprot)
        {
            switch (kind)
            {
                case type::TCP:
                    sockfam = AF_INET;
                    socktype = SOCK_STREAM;
                    sockprot = IPPROTO_TCP;
                    break;

                case type::UDP:
                    sockfam = AF_INET;
                    socktype = SOCK_DGRAM;
                    sockprot = IPPROTO_UDP;
                    break;

                default:
                    sockfam = socktype = sockprot = -1;
                    break;
            }
        }

    private:
        handle_type     handle_ = INVALID;
        socket::type    type_ = type::NONE;
        std::error_code error_;
};

}}

int main (int argc, char **argv)
{
    io::net::socket::address addr {"127.0.0.1:8080"};
    cout << (std::string) addr << endl;
    cout << addr.host() << endl;
    cout << addr.port() << endl;

    io::net::socket sock {io::net::socket::type::UDP};

    //scoped_thread network { std::thread { [] { } } };

    //bool quit = false;
    //std::string command;

    //while (!quit && cin >> command)
    //{
    //    if (command == "quit")
    //        quit = true;
    //}

    return 0;
}
