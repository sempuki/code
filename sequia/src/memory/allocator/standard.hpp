#ifndef _STANDARD_ALLOCATOR_HPP_
#define _STANDARD_ALLOCATOR_HPP_

namespace sequia
{
    namespace memory
    {
        namespace allocator
        {
            //=========================================================================
            // Implements fixed-buffer semantics
            // Fulfills stateful allocator concept
            // Fulfills rebindable allocator concept
            // Fulfills terminal allocator concept

            template <typename T, typename State = base_state<T>>
            class standard : public std::allocator<T>
            {
                protected:
                    using base_type = std::false_type;

                public:
                    using value_type = T;
                    using state_type = State;

                    using propagate_on_container_copy_assignment = std::false_type;
                    using propagate_on_container_move_assignment = std::false_type;
                    using propagate_on_container_swap = std::false_type;

                public:
                    template <typename U>
                    struct rebind 
                    { 
                        using other = standard<U>; 
                    };

                public:
                    // stateful copy constructor
                    template <typename Allocator>
                    standard (Allocator const &copy) :
                        standard {copy.state()} {}

                    // stateful constructor
                    explicit standard (state_type const &state) :
                        state_ {state} {}

                    // destructor
                    ~standard () = default;

                public:
                    state_type const &state() const { return state_; }

                private:
                    state_type  state_;
            };
        }
    }
}

#endif
