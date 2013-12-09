#ifndef _CONTENT_HPP_
#define _CONTENT_HPP_

#include "standard.hpp"
#include "buffer.hpp"

namespace content {

    struct buffer 
    {
        uint64_t id = 0;
        uint32_t size = 0;
        uint8_t const *bytes = nullptr;
        
        buffer () = default;
        buffer (uint64_t id, uint32_t size, uint8_t const *bytes = nullptr) : 
            id {id}, size {size}, bytes {bytes} {} 
    };

    struct node 
    {
        uint64_t id = 0;                // content_id
        std::vector<std::string> type;  // content_type
        std::set<uint64_t> devices;     // content locations (by device_id)
        std::string description;        // content description
        content::buffer buffer;         // content byte stream

        node () = default;
        node (uint64_t id, std::vector<std::string> const &type, std::string description) :
            id {id}, type {type}, description {description} {}
        node (uint64_t id, std::vector<std::string> const &type, std::string description, content::buffer buffer) :
            id {id}, type {type}, description {description}, buffer {buffer} {}

        operator bool () const { return id != 0 && type.size() > 0 && devices.size() > 0; }
    };

    struct dataset
    {
        enum type 
        {
            NONE,
            CLIPBOARD,
            NUM_DATASETS
        };

        std::map<uint64_t, node> contents;
    };
}

#endif
