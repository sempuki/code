#ifndef _STATE_HPP_
#define _STATE_HPP_

namespace traits
{
    namespace state
    {
        struct null_transition {};
    }
}

namespace sequia
{
    namespace state
    {
        template <int StateID, typename ...States>
        struct machine_base 
        {
            void find (int id)
            {
                std::cout << "not found!!" << std::endl;
            }
        };

        template <int StateID, typename State, typename ...States>
        struct machine_base <StateID, State, States...> : 
        public machine_base <StateID+1, States...>
        {
            typedef machine_base <StateID+1, States...> parent_type;

            void find (int id)
            {
                if (id == StateID)
                    std::cout << "found: " << typeid(State).name() << std::endl;
                else
                    parent_type::find (id);
            }
        };

        template <typename ...States>
        class machine : public machine_base <0, States...>
        {
            public:
            private:
                uint8_t mem [core::max_type_size<States...>()];
        };
    }
}

#endif
