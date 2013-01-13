#ifndef _DATA_FILE_HEAP_DESCRIPTION_HPP_
#define _DATA_FILE_HEAP_DESCRIPTION_HPP_

namespace sequia { namespace data { namespace file {

    struct heap_description : public io::file::chunk
    {
        core::name name;
        size_t min, max;

        chunk &operator<< (std::istream &stream)
        {
            // TODO: whitespace
            stream >> name;
            stream >> min;
            stream >> max;

            return *this;
        }

        chunk &operator>> (std::ostream &stream)
        {
            // TODO: whitespace
            stream << name;
            stream << min;
            stream << max;

            return *this;
        }
    };

} } }

#endif
