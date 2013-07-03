#ifndef POLICY_DATA_MAPPER_HPP_
#define POLICY_DATA_MAPPER_HPP_

namespace sequia { namespace data { namespace map {

    namespace native 
    {
        // Declaration: Actual mappings are defined later

        template <typename Container, typename Type>
        size_t commit_size (Container const &container, Type const &type);

        template <typename Destination, typename Type>
        bool can_insert (Destination const &destination, Type const &input);

        template <typename Destination, typename Type>
        Destination &operator<< (Destination &destination, Type input);

        template <typename Source, typename Type>
        bool can_extract (Source const &source, Type const &output);

        template <typename Source, typename Type>
        Source &operator>> (Source &source, Type &output);
    }

    namespace network 
    {
        // Declaration: Actual mappings are defined later
        
        template <typename Container, typename Type>
        size_t commit_size (Container const &container, Type const &type);

        template <typename Destination, typename Type>
        bool can_insert (Destination const &destination, Type const &input);

        template <typename Destination, typename Type>
        Destination &operator<< (Destination &destination, Type input);

        template <typename Source, typename Type>
        bool can_extract (Source const &source, Type const &output);

        template <typename Source, typename Type>
        Source &operator>> (Source &source, Type &output);
    }

} } }

namespace sequia { namespace policy { namespace data { namespace mapper {

    struct native
    {
        template <typename Container, typename Type>
        size_t commit_size (Container const &container, Type const &type)
        {
            using sequia::data::map::native::commit_size;
            return commit_size (container, type);
        }

        template <typename Destination, typename Type>
        bool can_insert (Destination const &destination, Type const &input)
        {
            using sequia::data::map::native::can_insert;
            return can_insert (destination, input);
        }

        template <typename Destination, typename Type>
        Destination &insert (Destination &destination, Type input)
        {
            using sequia::data::map::native::operator<<;
            return destination << input;
        }

        template <typename Source, typename Type>
        bool can_extract (Source const &source, Type const &output)
        {
            using sequia::data::map::native::can_extract;
            return can_extract (source, output);
        }

        template <typename Source, typename Type>
        Source &extract (Source &source, Type &output)
        {
            using sequia::data::map::native::operator>>;
            return source >> output;
        }
    };

    struct network
    {
        template <typename Container, typename Type>
        size_t commit_size (Container const &container, Type const &type)
        {
            using sequia::data::map::network::commit_size;
            return commit_size (container, type);
        }

        template <typename Destination, typename Type>
        bool can_insert (Destination const &destination, Type const &input)
        {
            using sequia::data::map::network::can_insert;
            return can_insert (destination, input);
        }

        template <typename Destination, typename Type>
        Destination &insert (Destination &destination, Type input)
        {
            using sequia::data::map::network::operator<<;
            return destination << input;
        }

        template <typename Source, typename Type>
        bool can_extract (Source const &source, Type const &output)
        {
            using sequia::data::map::network::can_extract;
            return can_extract (source, output);
        }

        template <typename Source, typename Type>
        Source &extract (Source &source, Type &output)
        {
            using sequia::data::map::network::operator>>;
            return source >> output;
        }
    };

} } } }

#endif
