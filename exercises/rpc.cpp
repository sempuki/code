#include <iostream>
#include <cstdint>
#include <functional>
#include <algorithm>
#include <thread>
#include <future>
#include <mutex>
#include <chrono>
#include <vector>
#include <deque>

namespace thread
{
    template <typename Type>
    class queue
    {
            using const_iterator = typename std::deque<Type>::const_iterator;
        public:

            const_iterator begin () const { return std::begin (queue_); }
            const_iterator end () const { return std::end (queue_); }
            std::recursive_mutex &mutex () const { return mtx_; }

        public:
            size_t size () const 
            {
                std::lock_guard<std::recursive_mutex> lck {mtx_};
                return queue_.size ();
            }

            bool empty () const 
            {
                std::lock_guard<std::recursive_mutex> lck {mtx_};
                return queue_.empty ();
            }
            
            void push (Type value)
            {
                std::lock_guard<std::recursive_mutex> lck {mtx_};
                queue_.push_back (value);
            }

            template <typename Iterator>
            void push (Iterator begin, Iterator end)
            {
                std::lock_guard<std::recursive_mutex> lck {mtx_};
                queue_.insert (std::end (queue_), begin, end);
            }

            Type pop ()
            {
                std::lock_guard<std::recursive_mutex> lck {mtx_};
                Type value = queue_.front (); queue_.pop ();
                return value;
            }

            void resize (size_t size)
            {
                std::lock_guard<std::recursive_mutex> lck {mtx_};
                queue_.resize (size);
            }

        private:
            mutable std::recursive_mutex mtx_;
            std::deque<Type> queue_;
    };
};

namespace rpc
{
    struct message
    {
        int method_id;
        int session_id;

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
            thread::queue<client::state *> pending_send;
            thread::queue<client::state *> pending_recv;

            template <typename Type>
            auto send (typename Type::request request) -> typename Type::response
            {
                static int session = 0;

                typename Type::response response;
                client::state state {{Type::id, ++session, nullptr, 0}, {}};

                size_t bytessent = request.serialize (state.request);
                std::cout << "sending method id: " << state.request.method_id 
                          << " session id: " << state.request.session_id << std::endl;

                std::future<rpc::message> reply = state.response.get_future ();
                pending_send.push (&state);
                reply.wait ();

                size_t bytesrecv = response.deserialize (reply.get());

                return response;
            }

            void dispatch (thread::queue<rpc::message> &send, thread::queue<rpc::message> &recv)
            {
                std::cout << "dispatching" << std::endl;
                std::vector<rpc::client::state *> sending;
                std::vector<rpc::client::state *> waiting;
                std::vector<rpc::message> outgoing;
                std::vector<rpc::message> incoming;
                
                if (!pending_send.empty())
                { 
                    std::cout << "have pending send: " << pending_send.size() << std::endl;
                    std::lock_guard<std::recursive_mutex> lck {pending_send.mutex()};
                    for (auto state : pending_send)
                        sending.push_back (state);
                    pending_send.resize (0);
                }

                if (!pending_recv.empty())
                {
                    std::cout << "have pending recv: " << pending_recv.size() << std::endl;
                    std::lock_guard<std::recursive_mutex> lck {pending_recv.mutex()};
                    for (auto state : pending_recv)
                        waiting.push_back (state);
                    pending_recv.resize (0);
                }

                if (!recv.empty())
                {
                    std::lock_guard<std::recursive_mutex> lck {recv.mutex()};
                    for (auto message : recv)
                        incoming.insert (std::end (incoming), message);
                    recv.resize (0);
                }

                for (auto state : sending) 
                    outgoing.insert (std::end (outgoing), state->request);

                pending_recv.push (std::begin (sending), std::end (sending));
                send.push (std::begin (outgoing), std::end (outgoing));

                for (auto reply : incoming)
                {
                    bool found = false;
                    for (auto client : waiting)
                    {
                        if (reply.method_id == client->request.method_id && 
                            reply.session_id == client->request.session_id) 
                        {
                            std::cout << "found client waiting for method id: " 
                                << reply.method_id << " session id: " 
                                << reply.session_id << std::endl;

                            client->response.set_value (reply);
                            found = true;
                            break;
                        }
                    }
                }
            }
        };
    }
}

thread::queue<rpc::message> network_buffer_up;
thread::queue<rpc::message> network_buffer_down;

static void client_sender (rpc::client::channel &client)
{
    std::cout << "client sender" << std::endl;
    client.send<rpc::hello> (rpc::hello::request {});
    std::cout << "got return" << std::endl;
};

static void client_dispatcher (rpc::client::channel &client)
{
    std::cout << "client dispatcher" << std::endl;
    while (true)
    {
        { 
            // simulate network connectivity / flush up -> back down
            std::lock_guard<std::recursive_mutex> lck {network_buffer_up.mutex ()};
            network_buffer_down.push (std::begin (network_buffer_up), std::end (network_buffer_up));
            network_buffer_up.resize (0);
        }

        client.dispatch (network_buffer_up, network_buffer_down);
        std::this_thread::sleep_for (std::chrono::milliseconds {500});
    }
}

int main (int argc, char **argv)
{
        // listen to incoming connection, and enqueue messages to shared queue
        // dispatch: match id to task, deserialize arguments, do computation, serialize result
        // move task to reply queue along with "session/call" id

    rpc::client::channel client;
    std::thread application {client_sender, std::ref (client)};
    std::thread dispatcher {client_dispatcher, std::ref (client)};

    application.join();
    dispatcher.join();

    return 0;
}
