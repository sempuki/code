#ifndef _IO_FILE_FORMAT_HPP_
#define _IO_FILE_FORMAT_HPP_

namespace ceres { namespace io { namespace file { namespace format {

    //=========================================================================
    // Facilitates processing of primitive ASCII file formats like INI, CSV

    //-------------------------------------------------------------------------
    // Character type facet with per-instance lookup table

    class ctype_char_local : public std::ctype<char>
    {
        public:
            ctype_char_local () : 
                std::ctype<char> {table_}
            {
                // classic_table is the static lookup table for "C" locale
                std::copy (classic_table(), classic_table() + table_size, table_);
            }

        private:
            mask table_ [table_size];
    };

    //-------------------------------------------------------------------------
    // IO manipulator to set delimiting characters
    
    class setdelim
    {
        public:
            setdelim (char const delim) : ch {delim} {}

            void operator() (std::istream &stream)
            {
                auto current = stream.getloc();

                // facets are reference counted, unique id-indexed, dynamic type-cast
                if (!std::has_facet<ctype_char_local> (current))
                {
                    stream.imbue (std::locale (current, new ctype_char_local));
                    current = stream.getloc();
                }

                auto &facet = std::use_facet<ctype_char_local> (current);
                auto table = const_cast <ctype_char_local::mask *> (facet.table());

                // modifying the table is "ok" because it's only local to this stream
                table[ch] |= ctype_char_local::space; // space is the default delimiter
            }

        private:
            char const ch;
    };

    std::istream &operator>> (std::istream &stream, setdelim delim)
    {
        delim (stream);
        return stream;
    }

    //-------------------------------------------------------------------------
    // IO manipulator to clear delimiting characters
    
    class cleardelim
    {
        public:
            cleardelim (char const delim) : ch {delim} {}

            void operator() (std::istream &stream)
            {
                auto current = stream.getloc();

                // facets are reference counted, unique id-indexed, dynamic type-cast
                if (!std::has_facet<ctype_char_local> (current))
                {
                    stream.imbue (std::locale (current, new ctype_char_local));
                    current = stream.getloc();
                }

                auto &facet = std::use_facet<ctype_char_local> (current);
                auto table = const_cast <ctype_char_local::mask *> (facet.table());

                // modifying the table is "ok" because it's only local to this stream
                table[ch] &= ~ctype_char_local::space; // space is the default delimiter
            }

        private:
            char const ch;
    };

    std::istream &operator>> (std::istream &stream, cleardelim delim)
    {
        delim (stream);
        return stream;
    }

    //-------------------------------------------------------------------------
    // IO manipulator to consume one delimiter (as "whitespace")

    std::istream &delim (std::istream &stream)
    {
        return std::ws (stream); // space is the default delimiter
    }

    //-------------------------------------------------------------------------
    // Consume constant single or multiple-character sequences
    
    std::istream &operator>> (std::istream &stream, std::istream::char_type const ch)
    {
        if (stream && stream.get() != ch)
            stream.setstate (std::ios_base::failbit);

        return stream;
    }

    std::istream &operator>> (std::istream &stream, std::string const &str)
    {
        for (std::istream::char_type const ch : str)
            if (!(stream >> ch))
                break;

        return stream;
    }

} } } }

#endif
