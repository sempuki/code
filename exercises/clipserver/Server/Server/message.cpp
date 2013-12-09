
#include "message.hpp"

#include <json/json.h>

namespace msg {
    
    bool try_write_message (Json::Value const &value, mem::buffer<uint8_t> buf, size_t &written)
    {
        Json::FastWriter writer;
        std::string str = writer.write (value);
        std::string::size_type size = str.size();

        bool proceed = (size > 0) && (buf.bytes >= size);
        if (proceed) std::copy (begin (str), end (str), begin (buf));
        written = proceed? size : 0;

        return proceed;
    }

    bool try_read_message (mem::buffer<uint8_t const> buf, Json::Value &value)
    {
        Json::Reader reader;
        mem::buffer<char const> charbuf {buf};

        return reader.parse (begin (charbuf), end (charbuf), value, false);
    }
    
    // ----- Header

    mem::buffer<uint8_t const> generate_header (mem::buffer<uint8_t> buf, header const &obj)
    {
        if (size (buf) < sizeof (obj.payload_size))
            assert (false && "write error: generate header");

        auto ptr = reinterpret_cast<decltype (obj.payload_size) *> (begin (buf));
        *ptr = htons (obj.payload_size);

        return {buf.pointer, sizeof (obj.payload_size)};
    }

    bool try_parse_header (mem::buffer<uint8_t const> buf, header &obj, size_t &consumed)
    {
        bool proceed = size (buf) >= sizeof (obj.payload_size);

        if (proceed)
        {
            auto ptr = reinterpret_cast<decltype (obj.payload_size) const *> (begin (buf));
            obj.payload_size = htons (*ptr);
        }

        consumed = proceed? sizeof (obj.payload_size) : 0;

        return proceed;
    }

    // ----- Authenticate

    mem::buffer<uint8_t const> generate_authenticate_request (mem::buffer<uint8_t> buf, authenticate_request const &obj)
    {
        Json::Value root;
        root["message_id"] = authenticate_request::id;
        root["user_id"] = (Json::Value::UInt64) obj.user_id;
        root["device_id"] = (Json::Value::UInt64) obj.device_id;
        root["secret"] = (Json::Value::UInt64) obj.secret;

        size_t written;
        if (!try_write_message (root, buf, written))
            std::cout << "write error: generate authenticate request" << std::endl;
        
        return {buf.pointer, written};
    }

    mem::buffer<uint8_t const> generate_authenticate_response (mem::buffer<uint8_t> buf, authenticate_response const &obj)
    {
        Json::Value root;
        root["message_id"] = authenticate_response::id;
        root["token"] = (Json::Value::UInt64) obj.token;
        root["message"] = obj.message;
        
        size_t written;
        if (!try_write_message (root, buf, written))
            std::cout << "write error: generate authenticate response" << std::endl;
        
        return {buf.pointer, written};
    }

    bool try_parse_authenticate_request (Json::Value root, authenticate_request &obj)
    {
        bool proceed = 
            root.isMember("user_id") && root["user_id"].isUInt64() &&
            root.isMember("device_id") && root["user_id"].isUInt64(); 

        if (proceed)
        {
            obj.user_id = root["user_id"].asUInt64();
            obj.device_id = root["device_id"].asUInt64();
        }

        return proceed;
    }

    bool try_parse_authenticate_response (Json::Value root, authenticate_response &obj)
    {
        bool proceed = 
            root.isMember("token") && root["token"].isUInt64() && 
            root.isMember("message") && root["message"].isUInt64();

        if (proceed)
        {
            obj.token = root["token"].asUInt64();
            obj.message = root["message"].asUInt();
        }

        return proceed;
    }

    // ----- RefreshIndex

    mem::buffer<uint8_t const> generate_refresh_index_request (mem::buffer<uint8_t> buf, refresh_index_request const &obj)
    {
        Json::Value root;
        root["message_id"] = refresh_index_request::id;
        root["token"] = (Json::Value::UInt64) obj.token;
        root["dataset_id"] = obj.dataset_id;
        
        size_t written;
        if (!try_write_message (root, buf, written))
            std::cout << "write error: generate refresh index request" << std::endl;

        return {buf.pointer, written};
    }

    mem::buffer<uint8_t const> generate_refresh_index_response (mem::buffer<uint8_t> buf, refresh_index_response const &obj)
    {
        Json::Value root, list;
        root["message_id"] = refresh_index_response::id;
        root["message"] = obj.message;

        for (auto index : obj.contents)
        {
            Json::Value content;
            content["id"] = (Json::Value::UInt64) index.id;
            for (auto const &type : index.type)
                content["content_type"].append (type);
            content["description"] = index.description;
            list.append (content);
        }

        root["contents"] = list;

        size_t written;
        if (!try_write_message (root, buf, written))
            std::cout << "write error: generate refresh index response" << std::endl;

        return {buf.pointer, written};
    }

    bool try_parse_refresh_index_request (Json::Value root, refresh_index_request &obj)
    {
        bool proceed = 
            root.isMember("token") && root["token"].isUInt64() && 
            root.isMember("dataset_id") && root["dataset_id"].isUInt();

        if (proceed)
        {
            obj.token = root["token"].asUInt64();
            obj.dataset_id = root["dataset_id"].asUInt();
        }

        return proceed;
    }

    bool try_parse_refresh_index_response (Json::Value root, refresh_index_response &obj)
    {
        std::vector<std::string> content_types;

        bool proceed = 
            root.isMember("message") && root["message"].isUInt() && 
            root.isMember("contents") && root["contents"].isArray();

        if (proceed)
        {
            obj.message = root["message"].asUInt();
            auto const &list = root["contents"];

            for (Json::ArrayIndex i=0; proceed && i < list.size(); ++i)
            {
                auto const &content = list[i];
                proceed = content.isMember("id") && 
                    content.isMember("content_type") && content["content_type"].isArray() && 
                    content.isMember("description");

                auto const &types = root["content_type"];
                for (Json::ArrayIndex j=0; proceed && j < types.size(); ++j)
                    content_types.push_back (types.asString());

                obj.contents.push_back ({content["id"].asUInt64(), content_types, content["description"].asString()});
            }
        }

        return proceed;
    }

    // ----- NotifyAvailable

    mem::buffer<uint8_t const> generate_notify_available_request (mem::buffer<uint8_t> buf, 
            notify_available_request const &obj)
    {
        Json::Value root;
        root["message_id"] = notify_available_request::id;
        root["token"] = (Json::Value::UInt64) obj.token;
        root["dataset_id"] = obj.dataset_id;
        root["content_id"] = (Json::Value::UInt64) obj.content.id;
        for (auto const &type : obj.content.type)
            root["content_type"].append (type);
        root["description"] = obj.content.description;
        
        size_t written;
        if (!try_write_message (root, buf, written))
            std::cout << "write error: generate notify available request" << std::endl;

        return {buf.pointer, written};
    }

    bool try_parse_notify_available_request (Json::Value root, notify_available_request &obj)
    {
        std::vector<std::string> content_types;

        bool proceed = 
            root.isMember("token") && root["token"].isUInt64() && 
            root.isMember("dataset_id") && root["dataset_id"].isUInt() && 
            root.isMember("content_id") && root["content_id"].isUInt64() && 
            root.isMember("content_type") && root["content_type"].isArray() && 
            root.isMember("description") && root["description"].isString();

        if (proceed)
        {
            obj.token = root["token"].asUInt64();
            obj.dataset_id = root["dataset_id"].asUInt();

            auto const &types = root["content_type"];
            for (Json::ArrayIndex i=0; proceed && i < types.size(); ++i)
                content_types.push_back (types[i].asString());

            obj.content = {root["content_id"].asUInt64(), content_types, root["description"].asString()};
        }

        return proceed;
    }

    // ----- Sync

    mem::buffer<uint8_t const> generate_sync_request (mem::buffer<uint8_t> buf, sync_request const &obj)
    {
        Json::Value root;
        root["message_id"] = sync_request::id;
        root["token"] = (Json::Value::UInt64) obj.token;
        root["content_id"] = (Json::Value::UInt64) obj.content_id;
        root["dataset_id"] = obj.dataset_id;
        
        size_t written;
        if (!try_write_message (root, buf, written))
            std::cout << "write error: generate sync request" << std::endl;

        return {buf.pointer, written};
    }

    mem::buffer<uint8_t const> generate_sync_response (mem::buffer<uint8_t> buf, sync_response const &obj)
    {
        Json::Value root;
        root["message_id"] = sync_response::id;
        root["token"] = (Json::Value::UInt64) obj.token;
        root["dataset_id"] = obj.dataset_id;
        root["content_id"] = (Json::Value::UInt64) obj.content.id;
        root["content_size"] = obj.content.size;
        root["message"] = obj.message;
        
        size_t written;
        if (!try_write_message (root, buf, written))
            std::cout << "write error: generate sync response" << std::endl;

        return {buf.pointer, written};
    }

    mem::buffer<uint8_t const> generate_ping (mem::buffer<uint8_t> buf, ping const &obj)
    {
        Json::Value root;
        root["message_id"] = ping::id;
        root["token"] = (Json::Value::UInt64) obj.token;
        
        size_t written;
        if (!try_write_message (root, buf, written))
            std::cout << "write error: generate ping" << std::endl;

        return {buf.pointer, written};
    }

    bool try_parse_sync_request (Json::Value root, sync_request &obj)
    {
        bool proceed = 
            root.isMember("token") && root["token"].isUInt64() &&      
            root.isMember("content_id") && root["content_id"].isUInt64() &&
            root.isMember("dataset_id") && root["dataset_id"].isUInt();

        if (proceed)
        {
            obj.token = root["token"].asUInt64();
            obj.content_id = root["content_id"].asUInt64();
            obj.dataset_id = root["dataset_id"].asUInt();
        }

        return proceed;
    }

    bool try_parse_sync_response (Json::Value root, sync_response &obj)
    {
        bool proceed = 
            root.isMember("token") && root["token"].isUInt64() && 
            root.isMember("dataset_id") && root["dataset_id"].isUInt() && 
            root.isMember("content_id") && root["content_id"].isUInt64() && 
            root.isMember("content_size") && root["content_size"].isUInt() &&
            root.isMember("message") && root["message"].isUInt();

        if (proceed)
        {
            obj.token = root["token"].asUInt64();
            obj.dataset_id = root["dataset_id"].asUInt();
            obj.content = {root["content_id"].asUInt64(), root["content_size"].asUInt()};
            obj.message = root["message"].asUInt();
        }

        return proceed;
    }

    bool try_parse_ping (Json::Value root, ping &obj)
    {
        bool proceed = root.isMember("token") && root["token"].isUInt64();

        if (proceed)
            obj.token = root["token"].asUInt64();

        return proceed;
    }

    template <typename MessageType, typename MessageGenerator>
    std::error_code generic_send (net::socket &socket, MessageType const &request, MessageGenerator generator)
    {
        size_t const HDR_BUFFER_SIZE = 128, MSG_BUFFER_SIZE = 4096;
        uint8_t hdrmem [HDR_BUFFER_SIZE], msgmem [MSG_BUFFER_SIZE];

        auto message = generator ({msgmem, MSG_BUFFER_SIZE}, request);
        auto header = generate_header ({hdrmem, HDR_BUFFER_SIZE}, {size (message)});
        
        std::error_code error;
        net::socket::size_type sent;
        bool proceed = socket.is_open();
            
        proceed = proceed && !(error = socket.send (header, sent)) && sent == size (header);
        proceed = proceed && !(error = socket.send_all (message, sent)) && sent == size (message);

        if (!proceed) 
            std::cout << "send error: " << error.value() << std::endl;

        return error;
    }

    
    std::error_code send (net::socket &socket, authenticate_request const &request)
    {
        return generic_send (socket, request, generate_authenticate_request);
    }

    std::error_code send (net::socket &socket, authenticate_response const &response)
    {
        return generic_send (socket, response, generate_authenticate_response);
    }

    std::error_code send (net::socket &socket, refresh_index_request const &request)
    {
        return generic_send (socket, request, generate_refresh_index_request);
    }

    std::error_code send (net::socket &socket, refresh_index_response const &response)
    {
        return generic_send (socket, response, generate_refresh_index_response);
    }

    std::error_code send (net::socket &socket, notify_available_request const &request)
    {
        return generic_send (socket, request, generate_notify_available_request);
    }

    std::error_code send (net::socket &socket, sync_request const &request)
    {
        return generic_send (socket, request, generate_sync_request);
    }

    std::error_code send (net::socket &socket, sync_response const &response)
    {
        return generic_send (socket, response, generate_sync_response);
    }
    
    std::error_code send (net::socket &socket, ping const &message)
    {
        return generic_send (socket, message, generate_ping);
    }

    // ----- Receiver

    void receiver::accept (net::socket &socket) 
    {
        on_accept (socket);
    }

    void receiver::close (net::socket &socket) 
    {
        on_close (socket);
        std::error_code error = socket.close();
        closed_ = true; 
    }

    // NOTE: based on known fixed header size and reading exactly required amount
    // if we instead pulled as much as available in a batch and processed from a loop 
    // in user-space, we could avoid context-switch overhead on syscalls, however this 
    // would involve more complex buffer management and likely state over multiple calls

    void receiver::dispatch (net::socket &socket)
    {
        using std::chrono::system_clock;
        auto now = system_clock::to_time_t (system_clock::now());
        std::cout << "\n " << std::ctime (&now) << "----------------------------------------" << std::endl;

        size_t const MSG_BUFFER_SIZE = 2048;
        uint8_t stackmem [MSG_BUFFER_SIZE];

        msg::header header;
        net::socket::size_type received;

        std::error_code error;
        bool proceed = socket.is_open();

        size_t const HEADER_SIZE = sizeof (header.payload_size);
        mem::buffer<uint8_t> headerbuf {stackmem, HEADER_SIZE};

        // receive header to discover upcoming payload message size
        proceed = proceed && !(error = socket.receive_all (headerbuf, received));
        proceed = proceed && received == HEADER_SIZE;
        if (!proceed) std::cout << "failed to receive header" << std::endl;

        // parse header to discover upcoming payload message size
        size_t headersize;
        proceed = proceed && try_parse_header (headerbuf, header, headersize);
        if (!proceed) std::cout << "failed to parse header" << std::endl;

        Json::Value deserialized;
        {
            std::unique_ptr<uint8_t> heapmem;
            mem::buffer<uint8_t> payloadbuf; 

            // allocate buffer suitable for expected payload size
            if (header.payload_size > sizeof (stackmem))
            {
                heapmem.reset (new uint8_t [header.payload_size]);
                payloadbuf = {heapmem.get(), header.payload_size};
            }
            else
                payloadbuf = {stackmem, header.payload_size};

            // receive json payload to discover message type
            proceed = proceed && !(error = socket.receive_all (payloadbuf, received));
            proceed = proceed && received == header.payload_size;
            if (!proceed) std::cout << "failed to receive payload" << std::endl;

            // parse json payload to discover message type
            proceed = proceed && try_read_message (payloadbuf, deserialized);
            proceed = proceed && deserialized.isMember ("message_id");
            if (!proceed) std::cout << "failed to parse payload" << std::endl;
        }

        if (proceed)
        {
            // dispatch json payload to discover message contents
            int msgid = deserialized["message_id"].asInt();
            std::cout << "received message: " << msgid << std::endl;
            switch (msgid)
            {
                case AUTHENTICATE_REQUEST:
                    {
                        authenticate_request msg;
                        if ((proceed = try_parse_authenticate_request (deserialized, msg)))
                            on_receive (socket, msg);
                    }
                    break;

                case AUTHENTICATE_RESPONSE:
                    {
                        authenticate_response msg;
                        if ((proceed = try_parse_authenticate_response (deserialized, msg)))
                            on_receive (socket, msg);
                    }
                    break;

                case REFRESH_INDEX_REQUEST:
                    {
                        refresh_index_request msg;
                        if ((proceed = try_parse_refresh_index_request (deserialized, msg)))
                            on_receive (socket, msg);
                    }
                    break;

                case REFRESH_INDEX_RESPONSE:
                    {
                        refresh_index_response msg;
                        if ((proceed = try_parse_refresh_index_response (deserialized, msg)))
                            on_receive (socket, msg);
                    }
                    break;

                case NOTIFY_AVAILABLE_REQUEST:
                    {
                        notify_available_request msg;
                        if ((proceed = try_parse_notify_available_request (deserialized, msg)))
                            on_receive (socket, msg);
                    }
                    break;

                case SYNC_REQUEST:
                    {
                        sync_request msg;
                        if ((proceed = try_parse_sync_request (deserialized, msg)))
                            on_receive (socket, msg);
                    }
                    break;

                case SYNC_RESPONSE:
                    {
                        sync_response msg;
                        if ((proceed = try_parse_sync_response (deserialized, msg)))
                            on_receive (socket, msg);
                    }
                    break;

                case PING:
                    {
                        ping msg;
                        if ((proceed = try_parse_ping (deserialized, msg)))
                            on_receive (socket, msg);
                    }
                    break;

                default:
                    std::cerr << "can't find handler for message" << std::endl;
                    break;
            }

            if (!proceed)
                std::cout << "failed to parse message (malformed message)" << std::endl;
        }
        else
            std::cout << "failed to deliver received message" << std::endl;

        if (socket.receive_interrupted())
            on_interrupt (socket);

        if (socket.error())
            on_error (socket);
    }

    // ----- Byte Block

    std::error_code send (net::socket &socket, mem::buffer<uint8_t const> buf)
    {
        //uint32_t header = htonl (buf.bytes);
        //auto headersize = sizeof (header);
        auto contentbytes = buf.pointer;
        auto contentsize = buf.bytes;

        std::error_code error;
        net::socket::size_type sent = 0;
        bool proceed = !socket.error();

        //proceed = proceed && !(error = socket.send ({&header, headersize}, sent)) 
        //    && (size_t) sent == headersize;

        proceed = proceed && !(error = socket.send_all ({contentbytes, contentsize}, sent)) 
            && (size_t) sent == contentsize;

        if (!proceed) assert (false && "write error: byte block");

        return error;
    }

    std::error_code recv (net::socket &socket, mem::buffer<uint8_t> buf)
    {
        std::error_code error;
        net::socket::size_type received = 0;
        bool proceed = socket.is_open();

        proceed = proceed && !(error = socket.receive_all (buf, received)) 
            && (size_t) received == size (buf);

        if (!proceed) assert (false && "read error: byte block");

        return error;
    }

    std::error_code recv_size (net::socket &socket, uint32_t &size)
    {
        std::error_code error;
        net::socket::size_type received = 0;
        bool proceed = socket.is_open();

        uint32_t header = 0;
        auto headersize = sizeof (header);

        proceed = !(error = socket.receive ({&header, headersize}, received)) 
            && (size_t) received >= headersize;

        size = proceed? ntohl (header) : 0;

        if (!proceed) assert (false && "read error: byte block");

        return error;
    }
}
