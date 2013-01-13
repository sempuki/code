#ifndef _IO_FILE_CHUNK_HPP_
#define _IO_FILE_CHUNK_HPP_

namespace sequia { namespace io { namespace file {

    struct chunk
    {
        virtual chunk &operator<< (std::istream &stream) = 0;
        virtual chunk const &operator>> (std::ostream &stream) const = 0;
    };

} } }

#endif
