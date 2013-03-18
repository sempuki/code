#ifndef _IO_NET_SOCKET_HPP_
#define _IO_NET_SOCKET_HPP_

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

namespace sequia { namespace io { namespace net {

    class socket
    {
        public:
            using handle_type = system::socket::handle_type;
            using size_type = system::socket::size_type;

            static const handle_type INVALID = system::socket::INVALID;

            enum class type { NONE, TCP, UDP }; // TODO: VDP et al.

            class address
            {
                public:
                    using native_type = system::socket::address_type;

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
                        if (!system::socket::try_string_to_inet4 (addr, address_))
                            system::load_last_error_code (error_);
                    }

                    operator std::string () const 
                    {
                        std::string result;
                        system::socket::try_inet4_to_string (address_, result);
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
                    native_type     address_;
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
                type_ = kind;

                if (!system::socket::try_open (sockfam, socktype, sockproto, handle_))
                    system::load_last_error_code (error_);

                return error_;
            }

            std::error_code close ()
            {
                if (!system::socket::try_close (handle_))
                    system::load_last_error_code (error_);

                invalidate ();

                return error_;
            }

            std::error_code bind (address const &local)
            {
                auto addr = (sockaddr const *) local;
                auto size = local.sockaddr_size ();

                if (!system::socket::try_bind (handle_, addr, size))
                    system::load_last_error_code (error_);

                return error_;
            }

            std::error_code connect (address const &remote)
            {
                auto addr = (sockaddr const *) remote;
                auto size = remote.sockaddr_size ();

                if (!system::socket::try_connect (handle_, addr, size))
                    system::load_last_error_code (error_);

                return error_;
            }

        public:
            std::error_code listen (int backlog = 8)
            {
                if (!system::socket::try_listen (handle_, backlog))
                    system::load_last_error_code (error_);

                return error_;
            }

            std::error_code accept (socket &accepted)
            {
                address remote;
                auto addr = (sockaddr *) remote;
                auto size = remote.sockaddr_size ();

                if (!system::socket::try_accept (handle_, addr, size, accepted.handle_))
                    system::load_last_error_code (error_);

                return error_;
            }

        public:
            address local_address ()
            {
                address local;
                auto addr = (sockaddr *) local;
                auto size = local.sockaddr_size ();

                if (!system::socket::try_get_local_address (handle_, addr, size))
                    system::load_last_error_code (error_);

                return local;
            }

            address remote_address ()
            {
                address remote;
                auto addr = (sockaddr *) remote;
                auto size = remote.sockaddr_size ();

                if (!system::socket::try_get_remote_address (handle_, addr, size))
                    system::load_last_error_code (error_);

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

} } }

#endif
