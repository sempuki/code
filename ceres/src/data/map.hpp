#ifndef DATA_MAP_HPP_
#define DATA_MAP_HPP_

namespace ceres { namespace data { namespace map {

    //-------------------------------------------------------------------------

#ifdef DEBUG
    std::ostream &operator<< (std::ostream &stream, core::name const &obj)
    {
        stream << obj.string();

        return stream;
    } 
#endif

    std::istream &operator>> (std::istream &stream, core::name &obj)
    {
        std::string s;

        stream >> s;
        obj = s.c_str();

        return stream;
    } 

    //-------------------------------------------------------------------------

    std::ostream &operator<< (std::ostream &stream, io::file::chunk const &obj)
    {
        obj >> stream;
        return stream;
    } 

    std::istream &operator>> (std::istream &stream, io::file::chunk &obj)
    {
        obj << stream;
        return stream;
    } 

    namespace native
    {
        // buffer .............................................................

        template <typename T>
        size_t commit_size (memory::bytebuffer const &buf, T value)
        {
            return sizeof (value);
        }

        template <typename T>
        bool can_insert (memory::bytebuffer const &buf, T value)
        {
            return buf.bytes >= commit_size (buf, value);
        }

        template <typename T>
        memory::bytebuffer &operator<< (memory::bytebuffer &buf, T value)
        {
            memory::buffer<T> typed = buf;
            *begin (typed) = value; 
            return buf.reset (offset (typed, 1));
        }

        template <typename T>
        bool can_extract (memory::bytebuffer const &buf, T value)
        {
            return buf.bytes >= commit_size (buf, value);
        }

        template <typename T>
        memory::bytebuffer &operator>> (memory::bytebuffer &buf, T &value)
        {
            memory::buffer<T> typed = buf;
            value = *begin (typed); 
            return buf.reset (offset (typed, 1));
        }

        // bitbuffer  .........................................................

        template <typename T>
        size_t commit_size (memory::bitbuffer const &buf, T value)
        {
            return sizeof (value) * 8;
        }

        template <typename T>
        bool can_insert (memory::bitbuffer const &buf, T value)
        {
            return size (buf) >= commit_size (buf, value);
        }

        template <typename T>
        memory::bitbuffer &operator<< (memory::bitbuffer &buf, T value)
        {
            data::encoding::bit::value parsed {value};

            return buf;
        }
        
        template <typename T>
        bool can_extract (memory::bitbuffer const &buf, T value)
        {
            return size (buf) >= commit_size (buf, value);
        }

        template <typename T>
        memory::bitbuffer &operator>> (memory::bitbuffer &buf, T &value)
        {
            data::encoding::bit::value parsed;

            return buf;
        }
    }

    namespace network
    {
        // buffer .............................................................

        template <typename T>
        size_t commit_size (memory::bytebuffer &buf, T value)
        {
            return sizeof (value);
        }

        template <typename T>
        bool can_insert (memory::bytebuffer &buf, T value)
        {
            return buf.bytes >= commit_size (buf, value);
        }

        template <typename T>
        memory::bytebuffer &operator<< (memory::bytebuffer &buf, T value)
        {
            memory::buffer<T> typed = buf;
            *begin (typed) = make_network_byte_order (value); 
            return buf.reset (offset (typed, 1));
        }

        template <typename T>
        bool can_extract (memory::bytebuffer &buf, T value)
        {
            return buf.bytes >= commit_size (buf, value);
        }

        template <typename T>
        memory::bytebuffer &operator>> (memory::bytebuffer &buf, T &value)
        {
            memory::buffer<T> typed = buf;
            value = make_host_byte_order (*begin (typed)); 
            return buf.reset (offset (typed, 1));
        }
    }

} } }

#endif
