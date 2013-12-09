
#ifdef _WIN32
#include <WS2tcpip.h>
#else
#include <unistd.h>
#include <arpa/inet.h>
#endif

#include "socket.hpp"

namespace sys { 

    namespace socket {

        void load_last_error_code(std::error_code &error)
        {
#ifdef _WIN32
            int errcode = WSAGetLastError();
#else
            int errcode = errno;
#endif
            return error.assign(errcode, std::system_category());
        }

#ifdef _WIN32
        namespace impl {
            std::atomic<bool> winsock_initialized_ = false;
        }
#endif

        bool try_initialize_sockets ()
        {
#ifdef _WIN32
            WSADATA response;
            uint16_t request {0x0202}; // Winsock 2.2

            int result = WSAStartup (request, &response);
            bool success = result == 0 && response.wVersion == 0x0202;
            if (success) impl::winsock_initialized_ = true;

            return success;
#else
            return true;
#endif
        }

        bool try_finalize_sockets ()
        {
#ifdef _WIN32
            int result = impl::winsock_initialized_ && WSACleanup ();
            bool success = result == 0;
            if (success) impl::winsock_initialized_ = false;

            return success;
#else
            return true;
#endif
        }

        bool has_initialized_sockets ()
        {
#ifdef _WIN32
            return impl::winsock_initialized_;
#else
            return true;
#endif
        }

        bool try_string_to_inet4 (std::string const &str, address_type &addr)
        {
            using std::string;
            using std::stoul; // WARN: can throw

            string host;
            size_t portmark = str.rfind (':');

            host = str.substr (0, portmark);
            if (portmark != string::npos) 
                addr.sin_port = htons (static_cast<uint16_t> (stoul (str.substr (portmark + 1))));

            int result = inet_pton (AF_INET, host.c_str(), &addr.sin_addr);
            bool success = result > 0;

            return success;
        }

        bool try_inet4_to_string (address_type const &addr, std::string &str)
        {
            using std::to_string; // WARN: can throw

            char buf [INET6_ADDRSTRLEN];

#ifdef _WIN32
            char const *result = inet_ntop (AF_INET, const_cast<in_addr *>(&addr.sin_addr), buf, sizeof (buf));
#else
            char const *result = inet_ntop (AF_INET, &addr.sin_addr, buf, sizeof (buf));
#endif
            bool success = result != nullptr;

            if (success)
            {
                str = buf; 
                str += ':';
                str += to_string (ntohs (addr.sin_port));
            }

            return success;
        }

        bool try_set_socket_options (handle_type handle, int level, int name, const void *value, size_type length)
        {
#ifdef _WIN32
            int result = setsockopt (handle, level, name, reinterpret_cast<const char *> (value), length);
#else
            int result = setsockopt (handle, level, name, value, length);
#endif
            return result == 0;
        }

        bool try_set_send_timeout (handle_type handle, std::chrono::milliseconds timeout)
        {
#ifdef _WIN32
            auto duration = static_cast<uint32_t> (timeout.count());
#else
            using std::chrono::duration_cast;
            auto s = duration_cast<std::chrono::seconds> (timeout);
            auto us = duration_cast<std::chrono::microseconds> (timeout - s);
            auto seconds = static_cast<long> (s.count());
            auto microseconds = static_cast<long> (us.count());
            timeval duration { seconds, microseconds };
#endif
            return try_set_socket_options (handle, SOL_SOCKET, SO_SNDTIMEO, &duration, sizeof(duration));
        }

        bool try_set_receive_timeout (handle_type handle, std::chrono::milliseconds timeout)
        {
#ifdef _WIN32
            auto duration = static_cast<uint32_t> (timeout.count());
#else
            using std::chrono::duration_cast;
            auto s = duration_cast<std::chrono::seconds> (timeout);
            auto us = duration_cast<std::chrono::microseconds> (timeout - s);
            auto seconds = static_cast<long> (s.count());
            auto microseconds = static_cast<long> (us.count());
            timeval duration { seconds, microseconds };
#endif
            return try_set_socket_options (handle, SOL_SOCKET, SO_RCVTIMEO, &duration, sizeof(duration));
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
            if (handle != INVALID)
                std::cout << "closing socket " << handle << std::endl;
#ifdef _WIN32
            handle_type result = closesocket (handle);
#else
            handle_type result = close (handle);
#endif
            bool success = result == 0;

            if (success)
                handle = INVALID;

            return success;
        }

        bool try_bind (handle_type handle, sockaddr const *addr, size_type size)
        {
            handle_type result = bind (handle, addr, size);
            bool success = result != INVALID;

            return success;
        }

        bool try_connect (handle_type handle, sockaddr const *addr, size_type size)
        {
            handle_type result = connect (handle, addr, size);
            bool success = result != INVALID;

            return success;
        }

        bool try_listen (handle_type handle, int backlog)
        {
            handle_type result = listen (handle, backlog);
            bool success = result != INVALID;

            return success;
        }

        bool try_accept (handle_type handle, sockaddr *addr, size_type &size, handle_type &accepted)
        {
            handle_type result = accept (handle, addr, &size);
            bool success = result != INVALID;

            if (success)
                accepted = result;

            return success;
        }

        bool try_get_local_address (handle_type handle, sockaddr *addr, size_type &size)
        {
            handle_type result = getsockname (handle, addr, &size);
            bool success = result == 0;

            return success;
        }

        bool try_get_remote_address (handle_type handle, sockaddr *addr, size_type &size)
        {
            handle_type result = getpeername (handle, addr, &size);
            bool success = result == 0;

            return success;
        }
        
        bool try_send (handle_type handle, void const *buf, size_type bytes, int flags, size_type &sent)
        {
#ifdef _WIN32
            ssize_type result = send (handle, static_cast<char const *> (buf), bytes, flags);
#else
            ssize_type result = send (handle, buf, bytes, flags);
#endif
            bool success = result >= 0;

            sent = success? static_cast<size_type> (result) : 0;
            std::cout << "sending bytes: " << sent << " to " << (int) handle << std::endl;
            return success;
        }

        bool try_sendto (handle_type handle, sockaddr const *addr, size_type size, 
                void const *buf, size_type bytes, int flags, size_type &sent)
        {
#ifdef _WIN32
            ssize_type result = sendto (handle, static_cast<char const *> (buf), bytes, flags, addr, size);
#else
            ssize_type result = sendto (handle, buf, bytes, flags, addr, size);
#endif
            bool success = result >= 0;

            sent = success? static_cast<size_type> (result) : 0;
            return success;
        }
        
        bool try_recv (handle_type handle, void *buf, size_type bytes, int flags, size_type &received)
        {
#ifdef _WIN32
            ssize_type result = recv (handle, static_cast<char *> (buf), bytes, flags);
#else
            ssize_type result = recv (handle, buf, bytes, flags);
#endif
            bool success = result > 0;

            received = success? static_cast<size_type> (result) : 0;
            std::cout << "received bytes: " << received << " out of " << bytes << " from " << (int) handle << std::endl;
            return success;
        }

        bool try_recvfrom (handle_type handle, sockaddr *addr, size_type size,
                void *buf, size_type bytes, int flags, size_type &received)
        {
#ifdef _WIN32
            ssize_type result = recvfrom (handle, static_cast<char *> (buf), bytes, flags, addr, &size);
#else
            ssize_type result = recvfrom (handle, buf, bytes, flags, addr, &size);
#endif
            bool success = result > 0;

            received = success? static_cast<size_type> (result) : 0;
            return success;
        }
} }

namespace net {

    socket::address::address ()
    {
        address_.sin_family = AF_INET;
        address_.sin_addr.s_addr = INADDR_ANY;
        address_.sin_port = 0;
        error_ = std::make_error_code(std::errc::bad_address);
    }

    socket::address::address (std::string const &addr)
    {
        address_.sin_family = AF_INET;
        if (!sys::socket::try_string_to_inet4 (addr, address_))
            sys::socket::load_last_error_code (error_);
    }

    socket::address::operator std::string () const 
    {
        std::string result;
        sys::socket::try_inet4_to_string (address_, result);
        return result;
    }

    socket::address::operator sockaddr* () 
    { 
        return reinterpret_cast<sockaddr *> (&address_); 
    }

    socket::address::operator sockaddr const* () const
    { 
        return reinterpret_cast<sockaddr const *> (&address_); 
    }

    socket::size_type socket::address::sockaddr_size () const
    {
        return sizeof (address_);
    }

    socket::address::operator bool () const { return !error_; }
    std::error_code socket::address::error () const { return error_; }

    bool socket::address::has_same_port (address const &other) const
    {
        return address_.sin_port == other.address_.sin_port;
    }

    bool socket::address::has_same_host (address const &other) const
    {
        return address_.sin_addr.s_addr == other.address_.sin_addr.s_addr;
    }

    bool socket::address::operator== (address const &other) const 
    {
        return has_same_port (other) && has_same_host (other);
    }

    uint16_t socket::address::port () const { return ntohs (address_.sin_port); }
    void socket::address::set_port (uint16_t port) { address_.sin_port = htons (port); }

    uint32_t socket::address::host () const { return ntohl (address_.sin_addr.s_addr); }
    void socket::address::set_host (uint32_t host) { address_.sin_addr.s_addr = htonl (host); }

    socket::socket ()
    {
#ifdef _WIN32
        if (!sys::socket::has_initialized_sockets ())
            if (!sys::socket::try_initialize_sockets ())
                sys::socket::load_last_error_code (error_);
#endif
    }

    socket::socket (type kind) :
        socket {}
    { 
        open (kind); 
    }

    socket::~socket () 
    { 
        close (); 
    }

    socket::socket (socket &&other) :
        handle_ {other.handle_},
        type_ {other.type_},
        error_ {other.error_}
    { 
        other.invalidate (); 
    }

    socket &socket::operator= (socket &&other) 
    {
        handle_ = other.handle_;
        type_ = other.type_;
        error_ = other.error_;
        other.invalidate ();
        return *this;
    }

    bool socket::operator== (socket const &socket) 
    { 
        return handle_ == socket.handle_; 
    }

    bool socket::operator!= (socket const &socket) 
    { 
        return handle_ != socket.handle_; 
    }

    bool socket::operator< (socket const &socket) 
    { 
        return handle_ < socket.handle_; 
    }

    socket::operator bool () const 
    { 
        return !error_; 
    }

    std::error_code socket::error () const 
    { 
        return error_; 
    }

    void socket::clear_error () 
    { 
        error_ = {}; 
    }

    bool socket::is_open () const 
    {
        return handle_ != INVALID && !error_;
    }

    bool socket::receive_interrupted () const
    {
        return recv_interrupt_;
    }
            
    bool socket::invalid () const
    {
        return handle_ == INVALID;
    }

    void socket::invalidate () 
    { 
        handle_ = INVALID; 
    }

    socket::type socket::get_type () const 
    { 
        return type_; 
    }
    
    socket::handle_type socket::get_handle () const 
    { 
        return handle_; 
    }

    void socket::set_send_timeout (std::chrono::milliseconds timeout)
    {
        if (!sys::socket::try_set_send_timeout (handle_, timeout))
            sys::socket::load_last_error_code (error_);
    }

    void socket::set_receive_timeout (std::chrono::milliseconds timeout)
    {
        if (!sys::socket::try_set_receive_timeout (handle_, timeout))
            sys::socket::load_last_error_code (error_);
    }

    std::error_code socket::open (type kind)
    {
        int sockfam, socktype, sockproto;
        map_socket_type (kind, sockfam, socktype, sockproto);
        type_ = kind;

        if (!sys::socket::try_open (sockfam, socktype, sockproto, handle_))
            sys::socket::load_last_error_code (error_);

        return error_;
    }

    std::error_code socket::close ()
    {
        if (!sys::socket::try_close (handle_))
            sys::socket::load_last_error_code (error_);

        invalidate ();

        return error_;
    }

    std::error_code socket::bind (address const &local)
    {
        auto addr = (sockaddr const *) local;
        auto size = local.sockaddr_size ();

        if (!sys::socket::try_bind (handle_, addr, size))
            sys::socket::load_last_error_code (error_);

        return error_;
    }

    std::error_code socket::connect (address const &remote)
    {
        auto addr = (sockaddr const *) remote;
        auto size = remote.sockaddr_size ();

        if (!sys::socket::try_connect (handle_, addr, size))
            sys::socket::load_last_error_code (error_);

        return error_;
    }

    std::error_code socket::listen (int backlog)
    {
        if (!sys::socket::try_listen (handle_, backlog))
            sys::socket::load_last_error_code (error_);

        return error_;
    }

    std::error_code socket::accept (socket &accepted)
    {
        address remote;
        auto addr = (sockaddr *) remote;
        auto size = remote.sockaddr_size ();

        if (!sys::socket::try_accept (handle_, addr, size, accepted.handle_))
            sys::socket::load_last_error_code (error_);

        return error_;
    }

    socket::address socket::local_address ()
    {
        address local;
        auto addr = (sockaddr *) local;
        auto size = local.sockaddr_size ();

        if (!sys::socket::try_get_local_address (handle_, addr, size))
            sys::socket::load_last_error_code (error_);

        return local;
    }

    socket::address socket::remote_address ()
    {
        address remote;
        auto addr = (sockaddr *) remote;
        auto size = remote.sockaddr_size ();

        if (!sys::socket::try_get_remote_address (handle_, addr, size))
            sys::socket::load_last_error_code (error_);

        return remote;
    }

    std::error_code socket::send (mem::buffer<uint8_t const> buf, size_type &sent)
    {
        if (!sys::socket::try_send (handle_, buf.pointer, buf.bytes, 0, sent))
            sys::socket::load_last_error_code (error_);

        return error_;
    }

    std::error_code socket::send_all (mem::buffer<uint8_t const> buf, size_type &sent)
    {
        size_type totalsize = 0, blocksize = 0;
        bool expecting = true; 
        bool success = true;

        while ((expecting = size (buf) > 0) &&
               (success = sys::socket::try_send (handle_, buf.pointer, buf.bytes, 0, blocksize)))
        {
            buf = advance (buf, blocksize);
            totalsize += blocksize;
        }

        sent = totalsize;
            
        if (!success) 
            sys::socket::load_last_error_code (error_);

        return error_;
    }

    std::error_code socket::send_to (address const &remote, mem::buffer<uint8_t const> buf, size_type &sent)
    {
        auto addr = (sockaddr const *) remote;
        auto size = remote.sockaddr_size ();

        if (!sys::socket::try_sendto (handle_, addr, size, buf.pointer, buf.bytes, 0, sent))
            sys::socket::load_last_error_code (error_);

        return error_;
    }

    std::error_code socket::receive (mem::buffer<uint8_t> buf, size_type &received)
    {
        if (!sys::socket::try_recv (handle_, buf.pointer, buf.bytes, 0, received))
            sys::socket::load_last_error_code (error_);

        recv_interrupt_ = received == 0;

        return error_;
    }
    
    std::error_code socket::receive_all (mem::buffer<uint8_t> buf, size_type &received)
    {
        size_type totalsize = 0, blocksize = 0;
        bool expecting = true; 
        bool success = true; 
        bool open = true;

        while ((expecting = size (buf) > 0) &&
               (success = sys::socket::try_recv (handle_, buf.pointer, buf.bytes, 0, blocksize)) && 
               (open = blocksize != 0))
        {
            buf = advance (buf, blocksize);
            totalsize += blocksize;
        }

        received = totalsize;
            
        if (!success) 
            sys::socket::load_last_error_code (error_);

        recv_interrupt_ = blocksize == 0;

        return error_;
    }

    std::error_code socket::receive_from (address &remote, mem::buffer<uint8_t> buf, size_type &received)
    {
        auto addr = (sockaddr *) remote;
        auto size = remote.sockaddr_size ();

        if (!sys::socket::try_recvfrom (handle_, addr, size, buf.pointer, buf.bytes, 0, received))
            sys::socket::load_last_error_code (error_);

        recv_interrupt_ = received == 0;

        return error_;
    }

    void socket::map_socket_type (type kind, int &sockfam, int &socktype, int &sockprot)
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
}
