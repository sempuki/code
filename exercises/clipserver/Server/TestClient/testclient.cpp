
#include "standard.hpp"
#include "buffer.hpp"
#include "socket.hpp"
#include "message.hpp"
#include "content.hpp"
#include "util.hpp"

#include <json/json.h>

using std::cout;
using std::endl;

namespace app
{
    uint64_t glb_token = 0;
    std::map<uint64_t, content::node> glb_contents;
    std::vector<std::string> glb_content_heap;

    std::string glb_working_path;

    struct receiver : public msg::receiver
    {
        void on_accept (net::socket &socket) override
        {
            std::cout << "receiver accept" << std::endl;
            socket.set_receive_timeout (std::chrono::seconds {60});
        }

        void on_close (net::socket &socket) override
        {
            std::cout << "receiver close" << std::endl;
        }

        void on_receive (net::socket &socket, msg::authenticate_request const &msg) override {} // server-side
        void on_receive (net::socket &socket, msg::refresh_index_request const &msg) override {} //server-side
        void on_receive (net::socket &socket, msg::notify_available_request const &msg) override {} // server-side

        void on_receive (net::socket &socket, msg::authenticate_response const &msg) override 
        {
            std::cout << "authenticate response: " << msg.token << " : " << msg.message << std::endl;
            glb_token = msg.token; // we're always authenticated!

            std::error_code error = msg::send (socket, msg::refresh_index_request {glb_token, content::dataset::CLIPBOARD});
            if (error) on_error (socket);
        }

        void on_receive (net::socket &socket, msg::refresh_index_response const &msg) override
        {
            std::cout << "refresh index response" << std::endl;

            for (auto index : msg.contents)
                std::cout << "content: " << index.description << std::endl;
        }

        void on_receive (net::socket &socket, msg::sync_request const &msg) override
        {
            std::cout << "sync request" << std::endl;
            
            auto content_id = msg.content_id;
            auto dataset_id = msg.dataset_id;

            std::error_code error;
            msg::sync_response response;
            bool proceed = glb_contents.count (content_id) > 0;
            
            response.token = glb_token;
            response.dataset_id = content::dataset::CLIPBOARD;
            response.message = proceed;

            if (proceed) response.content = glb_contents[content_id].buffer;
            auto bytes = proceed? response.content.bytes : nullptr;
            auto size  = proceed? response.content.size : 0;

            proceed = proceed && !(error = msg::send (socket, response));
            proceed = proceed && !(error = msg::send (socket, {bytes, size}));

            if (error) on_error (socket);
        }

        void on_receive (net::socket &socket, msg::sync_response const &msg) override
        {
            std::cout << "sync response" << std::endl;

            auto content_id = msg.content.id;
            auto size = msg.content.size;

            std::cout << "expecting " << content_id << " of size " << size << std::endl;
        }

        void on_interrupt (net::socket &socket) override
        {
            std::error_code error = socket.error();

#ifdef _WIN32 // uhh, thanks windows... :/
            auto timed_out = std::error_condition {WSAETIMEDOUT, std::system_category()};
#else
            auto timed_out = std::make_error_condition (std::errc::timed_out);
#endif
            if (error == timed_out)
            {
                socket.clear_error();
                error = msg::send (socket, msg::ping {glb_token});
                if (error) close (socket);
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

    void handler (net::socket &connector, msg::receiver &receiver)
    {
        msg::authenticate_request request;
        request.user_id = 1;
        request.device_id = 1;
        request.secret = 3;

        std::cout << "user: " << request.user_id << std::endl;

        std::error_code error = msg::send (connector, request);

        receiver.accept (connector);
        while (!error && !receiver.is_closed ())
            receiver.dispatch (connector);

        std::cout << "=======================================" << std::endl;
    }
}
    
namespace ui
{
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

    bool change_working_path (net::socket &socket, std::vector<std::string> const &args)
    {
        bool proceed = args.size() == 1;
        if (proceed) app::glb_working_path = args[0];
        return proceed;
    }

    bool notify (net::socket &socket, std::vector<std::string> const &args)
    {
        bool proceed = args.size() == 1;

        if (proceed)
        {
            app::glb_content_heap.push_back (args[0]);
            auto const &content = app::glb_content_heap.back();
            auto content_id = util::generate_guid63 (content);
            auto content_ptr = reinterpret_cast<uint8_t const *> (content.c_str());
            auto content_size = content.size();

            app::glb_contents[content_id] = {content_id, {"text/plain"}, content};
            app::glb_contents[content_id].buffer = {content_id, (uint32_t) content_size, content_ptr};
            auto const &node = app::glb_contents[content_id];

            msg::send (socket, msg::notify_available_request {app::glb_token, content::dataset::CLIPBOARD, node});
        }

        return proceed;
    }
}

int main (int argc, char **argv)
{
    if (argc != 2)
    {
        std::cout << "usage: <" << argv[0] << "> <ADDRESS>:<PORT>" << std::endl;
        return 0;
    }

    sys::socket::system sockets; // initializes socket system
    net::socket connector {net::socket::type::TCP};
    app::receiver receiver;
    std::thread thread;

    std::error_code error;
    bool proceed = connector && !(error = connector.connect ({argv[1]}));

    if (proceed)
	{
        thread = std::thread {app::handler, std::ref (connector), std::ref (receiver)};
        thread.detach ();
    }
    else
        cout << "had error " << error.message() << endl;
    
    std::string command;
    while (proceed && 
            std::cout << "> " && 
            std::cin >> command)
    {
        proceed = !receiver.is_closed ();
        auto arguments = ui::gather_arguments (std::cin);

        if (command == "quit")
            proceed = false;
        else if (command == "cd")
            proceed = ui::change_working_path (connector, arguments);
        else if (command == "notify")
            proceed = ui::notify (connector, arguments);
        else
            std::cout << "command not found" << std::endl;

        if (!proceed && command != "quit")
            std::cout << command << ": bad arguments" << std::endl;
    }
    
	return 0;
}
