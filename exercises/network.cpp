#include <iostream>
#include <thread>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

class socket
{
    public:
        enum class error_code
        {
            NONE,
            CONVERSION,
            NUMBER
        };

        class inetaddress
        {
            public:
                static const int MAX_STR_LEN = 64;

            public:
                inetaddress () = default;
                inetaddress (inetaddress const &other) = default;
                inetaddress &operator= (inetaddress const &other) = default;
                ~inetaddress () = default;

            public:
                inetaddress (std::string const &addr)
                {
                    // TODO: strip port number
                    // PORT: IPv4 family, WSAStringToAddress
                    int result = inet_pton (AF_INET, addr.c_str(), &address_.sin_addr);
                    if (result <= 0) errorcode_ = error_code::CONVERSION;
                }

                operator std::string () const 
                {
                    char const *result;
                    char buf [MAX_STR_LEN];
                    
                    // TODO: attach port number
                    // PORT: IPv4 family, WSAStringToAddress
                    
                    result = inet_ntop (AF_INET, &address_.sin_addr, buf, sizeof(buf));
                    return std::string {(result != nullptr)? buf : ""};
                }

            public:
                error_code error () const { return errorcode_; }
                operator bool () const { return errorcode_ == error_code::NONE; }

            public:
                bool has_same_port (inetaddress const &other) const
                {
                    return address_.sin_port == other.address_.sin_port;
                }

                bool has_same_host (inetaddress const &other) const
                {
                    return address_.sin_addr.s_addr == other.address_.sin_addr.s_addr;
                }

                bool operator== (inetaddress const &other) const 
                {
                    return has_same_port (other) && has_same_host (other);
                }

            public:
                uint16_t port () const { return ntohs (address_.sin_port); }
                void set_port (uint16_t port) { address_.sin_port = htons (port); }

                uint32_t host () const { return ntohl (address_.sin_addr.s_addr); }
                void set_host (uint32_t host) { address_.sin_addr.s_addr = htonl (host); }

            private:
                sockaddr_in address_;
                error_code  errorcode_ = error_code::NONE;
        };

    public:
        socket () = default;
        socket (socket const &other) = default;
        socket &operator= (socket const &socket) = default;
        ~socket () = default;

    private:
};

int main (int argc, char **argv)
{
    scoped_thread network { std::thread { [] { } } };

    bool quit = false;
    std::string command;

    while (!quit && cin >> command)
    {
        if (command == "quit")
            quit = true;
    }

    return 0;
}
