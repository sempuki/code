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
        public 
            using const_iterator = std::deque<Type>::const_iterator;

            const_iterator begin () const { return std::begin (queue_); }
            const_iterator end () const { return std::end (queue_); }
            std::mutex mutex () const { return mtx_; }

        public:
            size_t size () const 
            {
                std::lock_guard<std::mutex> lck {mtx_};
                return queue_.size ();
            }

            bool empty () const 
            {
                std::lock_guard<std::mutex> lck {mtx_};
                return queue_.empty ();
            }

            void push (Type value)
            {
                std::lock_guard<std::mutex> lck {mtx_};
                queue_.push_back (value);
            }

            void push (const_iterator begin, const_iterator end)
            {
                std::lock_guard<std::mutex> lck {mtx_};
                queue_.insert (std::end (queue_), begin, end);
            }

            Type pop ()
            {
                std::lock_guard<std::mutex> lck {mtx_};
                Type value = queue_.front (), queue_.pop ();
                return value;
            }

            void resize (size_t size)
            {
                std::lock_guard<std::mutex> lck {mtx_};
                queue_.resize (size);
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
        int const method_id;
        int const session_id;

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
                client::state state = available.pop (); // TODO: block if none available

                size_t bytessent = request.serialize (state.message);

                std::future<rpc::message> reply = state.response.get_future ();
                pending_send.push (state);
                reply.wait ();

                size_t bytesrecv = response.deserialize (reply.get());

                available.push (state);

                return response;
            }

            void dispatch (thread::queue<rpc::message> &send, thread::queue<rpc::message> &recv)
            {
                while (!pending_send.empty())
                {
                    client::state state = pending_send.pop ();
                    pending_recv.push (state);
                    send.push (state);
                }

                while (!recv.empty())
                {
                    rpc::message message = recv.pop ();

                    auto matching = [message] (rpc::message m) 
                    { 
                        return message.method_id == m.method_id &&
                            message.session_id == m.session_id; 
                    };

                    thread::queue<Type>::const_iterator expected = 
                        std::find_if (std::begin (pending_recv), std::end (pending_recv), matching);

                    if (expected != std::end (pending_recv))
                        expected.response.set_value (message);

                    else; // TODO: received a reply we weren't expecting
                }
            }
        };
    }
}

thread::queue<rpc::message> network_buffer_up;
thread::queue<rpc::message> network_buffer_down;

static void client_sender (rpc::client::channel &client)
{
    client.send (rpc::hello {});
};

static void client_dispatcher (rpc::client::channel &client)
{
    while (true)
    {
        { 
            // simulate network connectivity / flush up -> back down
            std::lock_guard<std::mutex> lck {network_buffer_up.mutex ()};
            network_buffer_down.push (std::begin (network_buffer_up), std::end (network_buffer_up));
            network_buffer_up.resize (0);
        }

        client.dispatch (network_buffer_up, network_buffer_down);
        std::this_thread::sleep_for (std::milliseconds {500});
    }
}

int main (int argc, char **argv)
{
        // listen to incoming connection, and enqueue messages to shared queue
        // dispatch: match id to task, deserialize arguments, do computation, serialize result
        // move task to reply queue along with "session/call" id

    rpc::client::channel client;
    std::thread {client_sender, client}.detach();
    std::thread {client_dispatcher, client}.detach();

    return 0;
}
