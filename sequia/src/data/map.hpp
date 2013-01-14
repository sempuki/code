#ifndef _DATA_MAP_HPP_
#define _DATA_MAP_HPP_

namespace sequia { namespace data { namespace map {

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

} } }

#endif
