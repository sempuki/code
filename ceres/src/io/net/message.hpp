#ifndef _IO_NET_MESSAGE_HPP_
#define _IO_NET_MESSAGE_HPP_

namespace ceres { namespace io { namespace net {

    class message
    {
        public:
            explicit stream (int id, memory::buffer<uint8_t> const buf) 
            {
            }

        private:
            int id_;
            memory::buffer<uint8_t> header_;
            memory::buffer<uint8_t> body_;
            memory::buffer<uint8_t> footer_;
    };

} } }

#endif
