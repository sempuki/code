#ifndef _SOCKET_HPP_
#define _SOCKET_HPP_

#ifdef _WIN32
#include <Winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include "standard.hpp"
#include "buffer.hpp"

namespace sys {
    
    namespace socket {

#ifdef _WIN32
		using handle_type = int;
		using size_type = int;
		using ssize_type = int;
		using address_type = sockaddr_in;
#else
		using handle_type = int;
		using size_type = socklen_t;
		using ssize_type = ssize_t;
		using address_type = sockaddr_in;
#endif
		static const handle_type INVALID = -1;
    
        void load_last_error_code (std::error_code &error);

        bool try_initialize_sockets ();
        bool try_finalize_sockets ();

        bool try_string_to_inet4 (std::string const &str, address_type &addr);
        bool try_inet4_to_string (address_type const &addr, std::string &str);

        bool try_open (int family, int type, int protocol, int &handle);
        bool try_close (handle_type &handle);
        
        bool try_bind (handle_type handle, sockaddr const *addr, size_type size);
        bool try_connect (handle_type handle, sockaddr const *addr, size_type size);

        bool try_listen (handle_type handle, int backlog);
        bool try_accept (handle_type handle, sockaddr *addr, size_type &size, handle_type &accepted);

        bool try_get_local_address (handle_type handle, sockaddr *addr, size_type &size);
        bool try_get_remote_address (handle_type handle, sockaddr *addr, size_type &size);

        bool try_send (handle_type handle, void const *buf, size_type bytes, int flags, size_type &sent);
        bool try_sendto (handle_type handle, sockaddr const *addr, size_type size, 
                void const *buf, size_type bytes, int flags, size_type &sent);
        
        bool try_recv (handle_type handle, void *buf, size_type bytes, int flags, size_type &received);
        bool try_recvfrom (handle_type handle, sockaddr *addr, size_type size,
                void *buf, size_type bytes, int flags, size_type &received);

        struct system
        {
            system () { try_initialize_sockets (); }
            ~system () { try_finalize_sockets (); }
        };
    }
}

namespace net {

    class socket
    {
        public:
            using handle_type = sys::socket::handle_type;
            using size_type = sys::socket::size_type;

            static const handle_type INVALID = sys::socket::INVALID;

            enum class type { NONE, TCP, UDP };

            class address
            {
                public:
                    using native_type = sys::socket::address_type;

                public:
                    address ();
                    address (std::string const &addr);

                public:
                    address (address const &other) = default;
                    address &operator= (address const &other) = default;
                    ~address () = default;

                public:
                    operator std::string () const;
                    operator sockaddr* ();
                    operator sockaddr const* () const;
                    size_type sockaddr_size () const;

                public:
                    operator bool () const;
                    std::error_code error () const;

                public:
                    bool has_same_port (address const &other) const;
                    bool has_same_host (address const &other) const;
                    bool operator== (address const &other) const;

                public:
                    uint16_t port () const;
                    void set_port (uint16_t port);

                    uint32_t host () const;
                    void set_host (uint32_t host);

                private:
                    native_type     address_;
                    std::error_code error_;
            };

        public:
            socket ();
            socket (type kind);
            socket (socket const &other) = delete;
            socket (socket &&other);
            ~socket ();

        public:
            socket &operator= (socket const &socket) = delete;
            socket &operator= (socket &&other);

        public:
            bool operator== (socket const &socket);
            bool operator!= (socket const &socket);
            bool operator< (socket const &socket);

        public:
            operator bool () const;
            std::error_code error () const;
            void clear_error ();

        public:
            bool is_open () const;
            bool receive_interrupted () const;

        public:
            bool invalid () const;
            void invalidate ();

        public:
            socket::type get_type () const;
            handle_type get_handle () const;

        public:
            void set_send_timeout (std::chrono::milliseconds timeout);
            void set_receive_timeout (std::chrono::milliseconds timeout);

        public:
            std::error_code open (type kind);
            std::error_code close ();
            std::error_code bind (address const &local);
            std::error_code connect (address const &remote);

        public:
            std::error_code listen (int backlog = 8);
            std::error_code accept (socket &accepted);

        public:
            address local_address ();
            address remote_address ();

        public:
            std::error_code send (mem::buffer<uint8_t const> buf, size_type &sent);
            std::error_code send_all (mem::buffer<uint8_t const> buf, size_type &sent);
            std::error_code send_to (address const &remote, mem::buffer<uint8_t const> buf, size_type &sent);

        public:
            std::error_code receive (mem::buffer<uint8_t> buf, size_type &received);
            std::error_code receive_all (mem::buffer<uint8_t> buf, size_type &received);
            std::error_code receive_from (address &remote, mem::buffer<uint8_t> buf, size_type &received);

        private:
            void map_socket_type (type kind, int &sockfam, int &socktype, int &sockprot);

        private:
            handle_type     handle_ = INVALID;
            socket::type    type_ = type::NONE;
            std::error_code error_;

            bool            recv_interrupt_ = false;
    };
}

#endif
