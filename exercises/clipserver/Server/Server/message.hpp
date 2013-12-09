#ifndef _MESSAGE_HPP_
#define _MESSAGE_HPP_

#include "standard.hpp"

#include "content.hpp"
#include "buffer.hpp"
#include "socket.hpp"

namespace msg {

    enum type
    {
        HEADER = 100,
        AUTHENTICATE_REQUEST,
        AUTHENTICATE_RESPONSE,
        REFRESH_INDEX_REQUEST,
        REFRESH_INDEX_RESPONSE,
        NOTIFY_AVAILABLE_REQUEST,
        SYNC_REQUEST,
        SYNC_RESPONSE,
        PING,
    };

    // ----- Header

    struct header 
    {
        uint16_t payload_size = 0;
        uint64_t checksum = 0; // TODO

        header () = default;
        header (uint64_t payload_size) :
            payload_size {static_cast<uint16_t> (payload_size)} {}
    };

    // ----- Authenticate

    struct authenticate_request
    {
        static const type id = AUTHENTICATE_REQUEST;

        uint64_t user_id = 0;
        uint64_t device_id = 0;
        uint64_t secret = 0;

        authenticate_request () = default;
        authenticate_request (uint64_t user_id, uint64_t device_id, uint64_t secret) :
            user_id {user_id}, device_id {device_id}, secret {secret} {}
    };

    struct authenticate_response
    {
        static const type id = AUTHENTICATE_RESPONSE;

        uint64_t token = 0;
        uint16_t message = 0; 

        authenticate_response () = default;
        authenticate_response (uint64_t token, uint16_t message = 0) :
            token {token}, message {message} {}
    };

    std::error_code send (net::socket &socket, authenticate_request const &request);
    std::error_code send (net::socket &socket, authenticate_response const &response);

    // ----- RefreshIndex

    struct refresh_index_request
    {
        static const type id = REFRESH_INDEX_REQUEST;

        uint64_t token = 0; 
        uint16_t dataset_id = 0;

        refresh_index_request () = default;
        refresh_index_request (uint64_t token, uint16_t dataset_id) :
            token {token}, dataset_id {dataset_id} {}
    };

    struct refresh_index_response
    {
        static const type id = REFRESH_INDEX_RESPONSE;

        std::vector<content::node> contents;
        uint16_t message = 0; 

        refresh_index_response () = default;
        refresh_index_response (std::vector<content::node> const &contents, uint16_t message = 0) :
            contents {contents}, message {message} {}
    };

    std::error_code send (net::socket &socket, refresh_index_request const &request);
    std::error_code send (net::socket &socket, refresh_index_response const &response);

    // ----- NotifyAvailable
    
    struct notify_available_request 
    {
        static const type id = NOTIFY_AVAILABLE_REQUEST;

        uint64_t token = 0; 
        uint16_t dataset_id = 0;
        content::node content;

        notify_available_request () = default;
        notify_available_request (uint64_t token, uint16_t dataset_id, content::node content) :
            token {token}, dataset_id {dataset_id}, content {content} {}
    };

    std::error_code send (net::socket &socket, notify_available_request const &request);

    // ----- Sync
    
    struct sync_request
    {
        static const type id = SYNC_REQUEST;

        uint64_t token = 0; 
        uint64_t content_id = 0;
        uint16_t dataset_id = 0;

        sync_request () = default;
        sync_request (uint64_t token, uint64_t content_id) :
            token {token}, content_id {content_id} {}
    };

    struct sync_response
    {
        static const type id = SYNC_RESPONSE;

        uint64_t token = 0; 
        uint16_t dataset_id = 0;
        content::buffer content;
        uint16_t message = 0; 

        sync_response () = default;
        sync_response (uint64_t token, uint16_t dataset_id, content::buffer content, uint16_t message = 0) :
            token {token}, dataset_id {dataset_id}, content {content}, message {message} {}
    };

    std::error_code send (net::socket &socket, sync_request const &request);
    std::error_code send (net::socket &socket, sync_response const &response);
    
    // ----- Link Management

    struct ping
    {
        static const type id = PING;
        uint64_t token = 0; 

        ping () = default;
        ping (uint64_t token) : token {token} {}
    };
    
    std::error_code send (net::socket &socket, ping const &message);
    
    // ----- Receiver
    
    class receiver
    {
        public:
            void accept (net::socket &socket);
            void dispatch (net::socket &socket);
            void close (net::socket &socket);
            bool is_closed () const { return closed_; }
            std::error_code error () const { return error_; }

        protected:
            virtual void on_receive (net::socket &socket, authenticate_request const &msg) = 0;
            virtual void on_receive (net::socket &socket, authenticate_response const &msg) = 0;
            virtual void on_receive (net::socket &socket, refresh_index_request const &msg) = 0;
            virtual void on_receive (net::socket &socket, refresh_index_response const &msg) = 0;
            virtual void on_receive (net::socket &socket, notify_available_request const &msg) = 0;
            virtual void on_receive (net::socket &socket, sync_request const &msg) = 0;
            virtual void on_receive (net::socket &socket, sync_response const &msg) = 0;
            virtual void on_receive (net::socket &socket, ping const &msg) {}

        protected:
            virtual void on_interrupt (net::socket &socket) = 0;
            virtual void on_error (net::socket &socket) = 0 { error_ = socket.error(); }

        protected:
            virtual void on_accept (net::socket &socket) = 0;
            virtual void on_close (net::socket &socket) = 0;

        private:
            std::error_code error_;
            std::atomic<bool> closed_ {false};
    };

    // ----- Byte Block

    std::error_code send (net::socket &socket, mem::buffer<uint8_t const> buf);
    std::error_code recv (net::socket &socket, mem::buffer<uint8_t> buf);
    std::error_code recv_size (net::socket &socket, size_t &size);
}

#endif
