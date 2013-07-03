#include <iostream>
#include <cstdint>
#include <functional>
#include <thread>
#include <mutex>
#include <deque>

namespace thread
{
    template <typename Type>
    class queue
    {
        public:
            size_t size () const 
            {
                std::lock_guard<std::mutex> lck (mtx_);
                return queue_.size ();
            }

            bool empty () const 
            {
                std::lock_guard<std::mutex> lck (mtx_);
                return queue_.empty ();
            }

            void push (Type value)
            {
                std::lock_guard<std::mutex> lck (mtx_);
                queue_.push_back (value);
            }

            Type pop ()
            {
                std::lock_guard<std::mutex> lck (mtx_);
                Type value = queue_.front (), queue_.pop ();
                return value;
            }

        private:
            std::mutex mtx_;
            std::deque<Type> queue_;
    };
};

namespace rpc
{
    struct message
    {
        int const session_id;
        int const method_id;

        uint8_t  *data;
        size_t    size;
    };

    struct serializable
    {
        size_t serialize   (rpc::message message) { return 0; }
        size_t deserialize (rpc::message message) { return 0; }
    };

    struct hello
    {
        constexpr static int id = 1;

        struct request : public serializable {};
        struct response : public serializable {};

        struct handler
        {
            response operator() (request)
            {
                std::cout << "hello world!" << std::endl;
            }
        };
    };

    namespace client
    {
        struct state
        {
            rpc::message request;
            std::promise<rpc::message> response;
        };

        struct channel
        {
            thread::queue<client::state> available;
            thread::queue<client::state> pending_send;
            thread::queue<client::state> pending_recv;

            template <typename Type>
            auto send (typename Type::request request) -> typename Type::response
            {
                typename Type::response response;
                client::state state = available.pop ();

                size_t bytessent = request.serialize (state.message);

                std::future<rpc::message> reply = state.response.get_future ();
                pending_send.push (state);
                reply.wait ();

                size_t bytesrecv = response.deserialize (reply.get());

                available.push (state);

                return response;
            }

            void dispatch (std::deque<rpc::message> &send, std::deque<rpc::message> &recv)
            {
                while (!pending_send.empty())
                {
                    client::state state = pending_send.pop ();
                    pending_recv.push (state);
                    send.push_back (state);
                }

                while (!recv.empty())
                {
                    // find existing method and session id in pending_recv
                }
            }
        };
    }
}


int main (int argc, char **argv)
{
    // simulate client/server with thread-safe message queue

    bool is_server = true;
    if (is_server)
    {
        // listen to incoming connection, and enqueue messages to shared queue
        // dispatch: match id to task, deserialize arguments, do computation, serialize result
        // move task to reply queue along with "session/call" id
    }
    else
    {
        // create two threads, have each rpc call create a packaged task, and block on future
        // have dispatch thread poll queue and multiplex calls over connection, set promise 
    }

    return 0;
}
