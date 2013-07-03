#ifndef _DATA_FILE_HEAP_DESCRIPTION_HPP_
#define _DATA_FILE_HEAP_DESCRIPTION_HPP_

namespace ceres { namespace data { namespace file {

    struct heap_description : public io::file::chunk
    {
        core::name name;
        size_t page_size, min_pages, max_pages;

        heap_description () :
            page_size {0}, min_pages {0}, max_pages {0} {}

        heap_description (char const *name, size_t page, size_t min, size_t max) :
            name {name}, page_size {page}, min_pages {min}, max_pages {max} {}

        chunk &operator<< (std::istream &stream)
        {
            using io::file::format::setdelim;
            using io::file::format::cleardelim;
            using io::file::format::delim;
            using io::file::format::operator>>;

            std::string const token_page_size {"page_size"};
            std::string const token_min_pages {"min_pages"};
            std::string const token_max_pages {"max_pages"};
            std::string heap;

            stream >> setdelim {'['} >> setdelim {']'} >> setdelim {'='};

            stream >> delim >> heap >> delim;
            stream >> token_page_size >> delim >> page_size >> std::ws;
            stream >> token_min_pages >> delim >> min_pages >> std::ws;
            stream >> token_max_pages >> delim >> max_pages >> std::ws;

            name = heap.c_str();

            stream >> cleardelim {'['} >> cleardelim {']'} >> cleardelim {'='};

            return *this;
        }

        chunk const &operator>> (std::ostream &stream) const
        {
            stream << '[' << name << ']' << std::endl;
            stream << "page_size = " << page_size << std::endl;
            stream << "min_pages = " << min_pages << std::endl;
            stream << "max_pages = " << max_pages << std::endl;
            stream << std::endl;

            return *this;
        }
    };

} } }

#endif
