#ifndef _SCOPED_ALLOCATOR_HPP_
#define _SCOPED_ALLOCATOR_HPP_

namespace ceres
{
    namespace memory
    {
        namespace allocator
        {
            //=========================================================================
            // Implements scoped allocate-on-construction semantics
            // Fulfills stateful allocator concept
            // Fulfills composable allocator concept

            namespace impl
            {
                template <typename Base, typename State, typename Type>
                class scoped : public Base
                {
                    public:
                        // default constructor
                        scoped () :
                            Base {} { common_initialization (); }    

                        // copy constructor
                        scoped (scoped const &copy) :
                            Base {copy} { common_initialization (); }

                        // stateful constructor
                        explicit scoped (State const &state) :
                            Base {state} { common_initialization (); }

                        // destructor
                        ~scoped () { common_finalization (); }

                    private:
                        void common_initialization ()
                        {
                            buffer<Type> &mem = Base::access_state().arena;
                            mem.items = Base::allocate (mem.size);
                        }

                        void common_finalization ()
                        {
                            buffer<Type> &mem = Base::access_state().arena;
                            Base::deallocate (mem.items, mem.size);
                        }
                };
            }

            template <typename Base>
            struct scoped
            {
                template <typename T>
                using state_type = get_state_type <Base, T>;

                template <typename S, typename T>
                using concrete_type = impl::scoped <get_concrete_type <Base, S, T>, S, T>;
                
                using propagate_on_container_copy_assignment = std::true_type;
                using propagate_on_container_move_assignment = std::true_type;
                using propagate_on_container_swap = std::true_type;
            };
        }
    }
}

#endif
