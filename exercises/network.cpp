#include <iostream>
#include <sstream>
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

namespace io { namespace net {

namespace detail 
{
    bool try_string_to_inet4 (std::string const &str, sockaddr_in &addr)
    {
        using std::string;
        using std::stoul;

        string host;
        size_t portmark = str.rfind (':');

        host = str.substr (0, portmark);
        if (portmark != string::npos) 
            addr.sin_port = htons (stoul (str.substr (portmark + 1)));

        int result = inet_pton (AF_INET, host.c_str(), &addr.sin_addr);
        bool success = result == 1;

        return success;
    }

    bool try_inet4_to_string (sockaddr_in const &addr, std::string &str)
    {
        using std::to_string;

        char buf [INET6_ADDRSTRLEN];

        char const *result = inet_ntop (AF_INET, &addr.sin_addr, buf, sizeof(buf));
        bool success = result != nullptr;

        if (success)
        {
            str = buf; 
            str += ':';
            str += to_string (ntohs (addr.sin_port));
        }
        
        return success;
    }
}


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
                inetaddress () = default;
                inetaddress (inetaddress const &other) = default;
                inetaddress &operator= (inetaddress const &other) = default;
                ~inetaddress () = default;

            public:
                inetaddress (std::string const &addr)
                {
                    if (!detail::try_string_to_inet4 (addr, address_))
                        errorcode_ = error_code::CONVERSION;
                }

                operator std::string () const 
                {
                    std::string result;
                    detail::try_inet4_to_string (address_, result);
                    return result;
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

}}

int main (int argc, char **argv)
{
    io::net::socket::inetaddress addr {"127.0.0.1:8080"};
    cout << (std::string) addr << endl;
    cout << addr.host() << endl;
    cout << addr.port() << endl;

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
