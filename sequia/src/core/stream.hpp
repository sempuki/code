#ifndef CORE_STREAM_HPP_
#define CORE_STREAM_HPP_

namespace sequia { namespace core {

    // type stream ============================================================
    
    template <typename E, typename IO>
    class stream : private IO
    {
        public:
            stream (memory::buffer<E> const &buf)
                : buf_ {buf}
            {}

            explicit operator bool () const { return !error_; }

        public:
            template <typename T>
            stream &operator<< (T const &item)
            {
                ASSERTF (!full(), "writing to a full stream");

                error_ = error_ || wrpos_ == size (buf_);

                if (!error_)
                {
                    IO::insert (buf_.items[wrpos_], item);

                    ++wrpos_ %= size (buf_);
                    ++size_;
                }

                return *this;
            }

            template <typename T>
            stream &operator>> (T &item)
            {
                ASSERTF (!empty(), "reading from an empty stream");

                error_ = error_ || rdpos_ == wrpos_;

                if (!error_)
                {
                    IO::extract (buf_.items[rdpos_], item);

                    ++rdpos_ %= size (buf_);
                    --size_;
                }

                return *this;
            }

            bool full () const { return size_ == size (buf_); }
            bool empty () const { return size_ == 0; }

            size_t capacity () const { return size (buf_); }
            size_t occupied () const { return size_; }
            size_t vacant () const { return size (buf_) - size_; } 

            void reset () { error_ = false; size_ = rdpos_ = wrpos_ = 0; }

        private:
            memory::buffer<E> buf_;
            size_t size_ = 0, rdpos_ = 0, wrpos_ = 0;
            bool error_ = false;
    };
    
    // TODO: pow2_size_buffers?

    // byte stream ============================================================
    
    template <typename IO>
    class stream <uint8_t, IO> : private IO
    {
        public:
            stream (memory::bytebuffer const &buf)
                : buf_ {buf}
            {}

            explicit operator bool () const { return !error_; }

        public:
            template <typename T>
            stream &operator<< (T const &item)
            {
                ASSERTF (!full(), "writing to a full stream");

                memory::bytebuffer wrbuf {begin (buf_) + wrpos_, size (buf_)};
                error_ = error_ || IO::can_insert (wrbuf, item) == false;

                if (!error_)
                {
                    IO::insert (wrbuf, item);
                    wrpos_ += IO::commit_size (wrbuf, item);
                }

                return *this;
            }

            template <typename T>
            stream &operator>> (T &item)
            {
                ASSERTF (!empty(), "reading from an empty stream");

                memory::bytebuffer rdbuf {begin (buf_) + rdpos_, wrpos_};
                error_ = error_ || IO::can_extract (rdbuf, item) == false;

                if (!error_)
                {
                    IO::extract (rdbuf, item);
                    rdpos_ += IO::commit_size (rdbuf, item);
                }

                return *this;
            }

            bool full () const { return wrpos_ == size (buf_); }
            bool empty () const { return wrpos_ == rdpos_; }

            size_t capacity () const { return size (buf_); }
            size_t occupied () const { return wrpos_ - rdpos_; }
            size_t vacant () const { return size (buf_) - wrpos_; } 

            void reset () { error_ = false; rdpos_ = wrpos_ = 0; }

        private:
            memory::bytebuffer buf_;
            size_t rdpos_ = 0, wrpos_ = 0;
            bool error_ = false;
    };

    // bit stream =============================================================
    
    template <typename IO>
    class stream <bool, IO> : private IO
    {
        public:
            stream (memory::bitbuffer const &buf)
                : buf_ {buf}
            {}

            explicit operator bool () const { return !error_; }

        public:
            template <typename T>
            stream &operator<< (T const &item)
            {
                ASSERTF (!full(), "writing to a full stream");

                memory::bitbuffer wrbuf {buf_.base, buf_.limit, wrpos_};
                error_ = error_ || IO::can_insert (wrbuf, item) == false;

                if (!error_)
                {
                    IO::insert (wrbuf, item);
                    rdpos_ += IO::commit_size (wrbuf, item);
                }

                return *this;
            }

            template <typename T>
            stream &operator>> (T &item)
            {
                ASSERTF (!empty(), "reading from an empty stream");

                memory::bitbuffer rdbuf {buf_.base, wrpos_, rdpos_};
                error_ = error_ || IO::can_extract (rdbuf, item) == false;

                if (!error_)
                {
                    IO::extract (rdbuf, item);
                    rdpos_ += IO::commit_size (rdbuf, item);
                }

                return *this;
            }

            bool full () const { return wrpos_ == size (buf_); }
            bool empty () const { return wrpos_ == rdpos_; }

            size_t capacity () const { return size (buf_); }
            size_t occupied () const { return wrpos_ - rdpos_; }
            size_t vacant () const { return size (buf_) - wrpos_; } 

            void reset () { error_ = false; rdpos_ = wrpos_ = 0; }

        private:
            memory::bitbuffer buf_;
            size_t rdpos_ = 0, wrpos_ = 0;
            bool error_ = false;
    };

    // default IO policies ----------------------------------------------------

    template <typename E>
    using basicstream = stream <E, policy::data::mapper::native>;
    using bytestream = stream <uint8_t, policy::data::mapper::native>;
    using bitstream = stream <bool, policy::data::mapper::native>;

    // serialization ----------------------------------------------------------
    
    template <typename Stream, typename T>
    bool serialize (Stream &stream, T arg)
    {
        return (bool) (stream << arg);
    }

    template <typename Stream, typename T, typename ...Types>
    bool serialize (Stream &stream, T arg, Types ...args)
    {
        return (stream << arg) && serialize (stream, std::forward<Types> (args)...);
    }

    template <typename Stream, typename T>
    bool deserialize (Stream &stream, T arg)
    {
        return (bool) (stream >> arg);
    }

    template <typename Stream, typename T, typename ...Types>
    bool deserialize (Stream &stream, T arg, Types ...args)
    {
        return (stream >> arg) && deserialize (stream, std::forward<Types> (args)...);
    }
} }

#endif
