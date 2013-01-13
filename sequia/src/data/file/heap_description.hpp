#ifndef _DATA_FILE_HEAP_DESCRIPTION_HPP_
#define _DATA_FILE_HEAP_DESCRIPTION_HPP_

namespace sequia { namespace data { namespace file {

    struct heap_description : public io::file::chunk
    {
        core::name name;
        size_t page, min, max;

        heap_description (char const *name, size_t page, size_t min, size_t max) :
            name {name}, page {page}, min {min}, max {max} {}

        chunk &operator<< (std::istream &stream)
        {
            using io::file::format::setdelim;
            using io::file::format::cleardelim;
            using io::file::format::delim;
            using io::file::format::operator>>;

            std::string const page_size {"page_size"};
            std::string const min_pages {"min_pages"};
            std::string const max_pages {"max_pages"};
            std::string token;

            stream >> setdelim {'['} >> setdelim {']'} >> setdelim {'='};

            stream >> delim >> token >> delim;
            stream >> page_size >> delim >> page >> std::ws;
            stream >> min_pages >> delim >> min >> std::ws;
            stream >> max_pages >> delim >> max >> std::ws;

            name = token.c_str();

            stream >> cleardelim {'['} >> cleardelim {']'} >> cleardelim {'='};

            return *this;
        }

        chunk const &operator>> (std::ostream &stream) const
        {
            stream << '[' << name << ']' << std::endl;
            stream << "page_size = " << page << std::endl;
            stream << "min_pages = " << min << std::endl;
            stream << "max_pages = " << max << std::endl;
            stream << std::endl;

            return *this;
        }
    };

} } }

#endif
