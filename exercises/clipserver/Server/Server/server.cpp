
#include "standard.hpp"
#include "buffer.hpp"
#include "socket.hpp"
#include "message.hpp"
#include "content.hpp"
#include "util.hpp"

#include <json/json.h>

namespace app
{
    struct connection
    {
        net::socket *remote = 0;
        uint64_t device_id = 0;
        uint64_t user_id = 0;
        uint64_t token = 0;

        connection () = default;
        connection (connection const &) = default;
        connection (net::socket *remote, uint64_t device_id, uint64_t user_id, uint64_t token) : 
            remote {remote}, device_id {device_id}, user_id {user_id}, token {token} {}

        bool operator== (connection const &other) const { return device_id == other.device_id; }
        bool operator< (connection const &other) const { return device_id < other.device_id; }
    };

    struct account
    {
        uint64_t id = 0;
        uint64_t secret = 0;

        std::array<content::dataset, content::dataset::NUM_DATASETS> datasets;
        std::set<connection> connections;

        account () = default;
        account (uint64_t id, uint64_t secret) :
            id {id}, secret {secret} {}
    };

    std::recursive_mutex glb_lock;
    std::map<uint64_t, app::account> glb_accounts;                          // user_id -> account
    std::map<uint64_t, uint64_t> glb_token_to_device_id;                    // token -> device_id
    std::map<uint64_t, uint64_t> glb_token_to_user_id;                      // token -> user_id
    std::map<uint64_t, app::connection> glb_connections;                    // device_id -> connection
    std::multimap<uint64_t, app::connection> glb_content_id_to_connection;  // content_id -> connection

    struct receiver : public msg::receiver
    {
        void on_accept (net::socket &socket) override
        {
            std::cout << "on_accept: " << socket.get_handle() << std::endl;
            socket.set_receive_timeout (std::chrono::minutes {5});
        }

        void on_close (net::socket &socket) override
        {
            std::lock_guard<std::recursive_mutex> lck {glb_lock};
            std::cout << "on_close " << std::endl;

            // find map pair using socket handle value
            auto conn = std::begin (glb_connections);
            auto end = std::end (glb_connections);
            for (; conn != end && socket != *conn->second.remote; ++conn);
            bool success = conn != end;

            // remove connections from data structures on exit
            if (success) 
            {
                auto connection = conn->second;
                
                success = glb_accounts[connection.user_id].connections.erase (connection) == 1 && success;
                success = glb_token_to_device_id.erase (connection.token) == 1 && success;
                success = glb_token_to_user_id.erase (connection.token) == 1 && success;
                
                auto pending = std::begin (glb_content_id_to_connection);
                auto end = std::end (glb_content_id_to_connection);
                while (pending != end)
                {
                    if (pending->second == connection)
                    {
                        auto dead = pending++;
                        glb_content_id_to_connection.erase (dead);
                    }
                    else pending++;
                }

                glb_connections.erase (conn);
            }

            if (!success) 
                std::cout << "warning: unable to remove connection!" << std::endl;
        }

        void on_receive (net::socket &socket, msg::authenticate_response const &msg) override {} // client-only
        void on_receive (net::socket &socket, msg::refresh_index_response const &msg) override {} // client-only

        void on_receive (net::socket &socket, msg::authenticate_request const &msg) override
        {
            std::lock_guard<std::recursive_mutex> lck {glb_lock};
            std::cout << "received auth request: " << msg.user_id << std::endl;
            std::cout << "from device: " << msg.device_id << std::endl;

            auto user_id = msg.user_id;
            auto device_id = msg.device_id;
            auto secret = msg.secret;

            if (glb_accounts.count (user_id) == 0)
            {
                std::cout << "creating new account" << std::endl;
                glb_accounts[user_id] = {user_id, secret}; 
            }

            // all connections are authenticated and given tokens
            auto &account = glb_accounts[user_id];
            uint64_t token = util::generate_rand63();
            std::cout << "using token: " << token << std::endl;

            // track current connections
            app::connection connection { &socket, device_id, user_id, token };
            glb_connections[device_id] = connection;
            account.connections.insert (connection);

            // maintain token mappings
            glb_token_to_device_id[token] = device_id;
            glb_token_to_user_id[token] = user_id;

            // respond successfully
            msg::authenticate_response response {token, 1}; 
            std::error_code error = msg::send (socket, response);
            if (error) on_error (socket);
        }

        void on_receive (net::socket &socket, msg::refresh_index_request const &msg) override
        {
            std::lock_guard<std::recursive_mutex> lck {glb_lock};
            std::cout << "refresh index request: " << msg.token << std::endl;

            auto token = msg.token;
            auto dataset_id = (size_t) msg.dataset_id;
            uint64_t user_id = 0;

            msg::refresh_index_response response;

            if (glb_token_to_user_id.count (token))
            {
                auto &account = glb_accounts[glb_token_to_user_id[token]];
                auto &dataset = account.datasets[dataset_id];

                for (auto const &pair : dataset.contents)
                    response.contents.push_back (pair.second);

                response.message = 1;
            }
            else
                response.message = 0;

            std::error_code error = msg::send (socket, response);
            if (error) on_error (socket);
        }

        void on_receive (net::socket &socket, msg::notify_available_request const &msg) override
        {
            std::lock_guard<std::recursive_mutex> lck {glb_lock};
            std::cout << "notify available request: " << msg.token << std::endl;

            auto token = msg.token;
            auto dataset_id = msg.dataset_id;
            auto content_id = msg.content.id;
            auto content_type = msg.content.type;
            auto description = msg.content.description;

            // forward notify to all connected clients
            if (glb_token_to_user_id.count (token) && 
                glb_token_to_device_id.count (token))
            {
                auto user_id = glb_token_to_user_id[token];
                auto device_id = glb_token_to_device_id[token];
                auto &account = glb_accounts[user_id];
                auto &dataset = account.datasets[dataset_id];
                auto &content = dataset.contents[content_id];
                if (!content) content = {content_id, content_type, description};
                content.devices.insert (device_id);

                for (auto connection : account.connections)
                {
                    std::cout << "this device_id: " << device_id << std::endl;
                    std::cout << "checking device_id: " << connection.device_id << std::endl;
                    if (device_id != connection.device_id)
                    {
                        std::cout << "socket: " << connection.remote->get_handle() << std::endl;
                        auto &remote = *connection.remote;
                        std::error_code error = msg::send (remote, msg);
                        if (error) on_error (remote);
                    }
                }
            }

            std::cout << "content: " << content_id << " : " << std::endl;
        }

        void on_receive (net::socket &socket, msg::sync_request const &msg) override
        {
            std::lock_guard<std::recursive_mutex> lck {glb_lock};
            std::cout << "sync request: " << msg.token << std::endl;

            auto token = msg.token;
            auto content_id = msg.content_id;
            auto dataset_id = msg.dataset_id;
            auto user_id = glb_token_to_user_id[token];
            auto device_id = glb_token_to_device_id[token];

            if (glb_token_to_user_id.count (token))
            {
                auto &account = glb_accounts[glb_token_to_user_id[token]];
                auto &dataset = account.datasets[dataset_id];
                if (dataset.contents.count (content_id))
                {
                    auto const &node = dataset.contents[content_id];
                    auto bytes = node.buffer.bytes;
                    auto size = node.buffer.size;

                    if (bytes != nullptr && size > 0)
                    {
                        std::cout << "content is cached on server: " << content_id << std::endl;

                        bool proceed = true;
                        std::error_code error;
                        msg::sync_response response;

                        response.token = 0; // TODO: who authenticates this
                        response.dataset_id = content::dataset::CLIPBOARD;
                        response.content = node.buffer;
                        response.message = proceed;

                        proceed = proceed && !(error = msg::send (socket, response));
                        proceed = proceed && !(error = msg::send (socket, {bytes, size}));
                        if (error) on_error (socket);
                    }
                    else
                    {
                        std::cout << "forwarding request: " << content_id << std::endl;
                        // we have to forward the request
                        // remember who initiated the request
                        std::pair<uint64_t, connection> entry = {content_id, {&socket, device_id, user_id, token}};
                        glb_content_id_to_connection.insert (entry);

                        // ask the device(s) that notified of this content
                        for (auto id : node.devices)
                        {
                            if (id != device_id)
                            {
                                std::cout << "to device: " << id << std::endl;
                                auto &connection = glb_connections[id];
                                auto &remote = *connection.remote;
                                std::error_code error = msg::send (remote, msg);
                                if (error) on_error (remote);
                            }
                        }
                    }
                }
            }
        }

        void on_receive (net::socket &socket, msg::sync_response const &msg) override
        {
            std::lock_guard<std::recursive_mutex> lck {glb_lock};
            std::cout << "sync response: " << msg.token << std::endl;

            auto token = msg.token;
            auto dataset_id = msg.dataset_id;
            auto content_id = msg.content.id;
            auto content_size = msg.content.size;
            auto message = msg.message;

            if (glb_token_to_user_id.count (token))
            {
                auto &account = glb_accounts[glb_token_to_user_id[token]];
                auto &dataset = account.datasets[dataset_id];
                if (dataset.contents.count (content_id))
                {
                    auto &node = dataset.contents[content_id];

                    std::error_code error;
                    bool proceed = socket.is_open();

                    // allocate heap storage based on expected content size
                    auto bytes = proceed? new uint8_t [content_size] : nullptr;
                    size_t size = proceed? content_size : 0;

                    // receive the full expected content
                    proceed = proceed && !(error = msg::recv (socket, {bytes, size}));
                    node.buffer.bytes = bytes;
                    node.buffer.size = size;

                    // forward to original requestor
                    auto range = glb_content_id_to_connection.equal_range (content_id);
                    for (auto pending = range.first; proceed && pending != range.second; ++pending)
                    {
                        std::cout << "sending to requestor device: " << pending->second.device_id << std::endl;
                        proceed = !(error = msg::send (*pending->second.remote, msg));
                        proceed = !(error = msg::send (*pending->second.remote, {bytes, size}));
                    }

                    glb_content_id_to_connection.erase (content_id);
                }
            }
        }

        void on_interrupt (net::socket &socket) override
        {
            std::error_code error = socket.error();

#ifdef _WIN32 // uhh, thanks windows... :/
            auto timed_out = std::error_condition {WSAETIMEDOUT, std::system_category()};
#else
            auto timed_out = std::make_error_condition (std::errc::timed_out);
#endif
            if (!error) 
            {
                close (socket); // how did we interrupt without an error?
            }
            else if (error == timed_out)
            {
                socket.clear_error();
                error = msg::send (socket, msg::ping {0}); // TODO: who authenticates this
            }
        }

        void on_error (net::socket &socket) override
        {
            msg::receiver::on_error (socket);
            std::error_code error = socket.error();

            if (error)
            {
                std::cout << "on_error: " << error.message() << " (" << error.value() << ")" << std::endl;
                close (socket);
            }
        }
    };

    void handler (net::socket &listener)
    {
        bool proceed = true;
        while (proceed)
        {
            net::socket accepted;
            std::error_code error = listener.accept (accepted);
            proceed = proceed && !error;

            std::cout << "=======================================" << std::endl;

            app::receiver receiver;
            receiver.accept (accepted);
            while (proceed && !receiver.is_closed ())
                receiver.dispatch (accepted);
        }
    }
}

std::vector<std::string> gather_arguments (std::istream &input)
{
    std::string argument, remaining;
    std::vector<std::string> arguments;
    std::getline (input, remaining);
    std::stringstream stream (remaining);

    while (std::getline (stream, argument, (stream.peek () == '"')? (char) stream.get () : ' '))
        if (!argument.empty()) arguments.push_back (argument);

    return arguments;
}

int main (int argc, char **argv)
{
    if (argc > 2)
    {
        std::cout << "usage: <" << argv[0] << "> [interactive]" << std::endl;
        return 0;
    }

    bool interactive = argc == 2 && std::string {argv[1]} == "interactive";

    sys::socket::system sockets; // initializes socket system
    net::socket listener {net::socket::type::TCP}; 
    std::vector<std::thread> threads;
    
    size_t const connect_backlog = 4;
    size_t const expected_overhead = 10;
    size_t const numthreads = expected_overhead * std::thread::hardware_concurrency();

    std::error_code error;
    bool proceed = listener;
    proceed = proceed && !(error = listener.bind ({"0.0.0.0:4242"}));
    proceed = proceed && !(error = listener.listen (connect_backlog));

    for (size_t i=0; proceed && i < numthreads; ++i)
        threads.push_back (std::move (std::thread {app::handler, std::ref (listener)}));

    if (proceed)
    {
	if (interactive)
        {
            for (auto &thread : threads)
                thread.detach();

            std::string command;
            while (proceed &&
                    std::cout << "> " && 
                    std::cin >> command)
            {
                std::cout << "command: " << command << std::endl;
                if (command == "quit")
                    proceed = false;
            }
        }
        else
            for (auto &thread : threads)
                thread.join();
    }
    else
        std::cout << "has error: " << error.message() << std::endl;
    
    return 0;
}

