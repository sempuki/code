#include <iostream>
#include <iterator>

using std::cout;
using std::endl;

namespace list
{
    template <typename T>
    class linked
    {
        public:
            struct node
            {
                T     value;
                node *next = nullptr;

                node (T v, node *n) : 
                    value {std::move (v)}, next {n} {}
            };

            class iterator : public std::iterator <std::forward_iterator_tag, T>
            {
                friend class linked;

                public:
                    iterator () = default;
                    iterator (iterator const &) = default;
                    iterator (node *n) : ptr_ {n} {}

                    operator bool () { return ptr_ != nullptr; }

                    bool operator== (iterator const &rhs) { return ptr_ == rhs.ptr_; }
                    bool operator!= (iterator const &rhs) { return ptr_ != rhs.ptr_; }

                    T &operator* () { return ptr_->value; }
                    T *operator-> () { return &(ptr_->value); }

                    iterator &operator++ () 
                    { 
                        ptr_ = ptr_->next; 
                        return *this; 
                    }

                    iterator operator++ (int) 
                    { 
                        iterator copy {*this}; 
                        ptr_ = ptr_->next; 
                        return copy; 
                    }

                private:
                    node  *node_ptr () { return ptr_; }
                    node **next_ptr () { return &ptr_->next; }

                private:
                    node *ptr_ = nullptr;
            };

            // TODO: move constructor/swap 

        public:
            size_t size () const { return size_; }
            bool empty () const { return head_ == nullptr; }
            T &front () { return head_->value; }

        public:
            iterator begin () { return iterator {head_}; }
            iterator end () { return iterator {nullptr}; }

        public:
            iterator previous (iterator position)
            {
                iterator prev; 

                for (auto iter = begin(); iter != end(); ++iter)
                    if (iter != position)
                        prev = iter;
                    else break;

                return prev;
            }

        public:
            iterator push_front (T const &value)
            {
                node **head = &head_;
                return iterator {insert_ (head, value)};
            }

            iterator insert_after (iterator position, T const &value)
            {
                node **head = position.next_ptr ();
                return iterator {insert_ (head, value)};
            }

            iterator insert_before (iterator position, T const &value)
            {
                return insert_after (previous (position), value);
            }

        public:
            void erase (iterator begin, iterator end)
            {
                iterator prev = previous (begin);

            }

        private:
            node *insert_ (node **head, T const &value)
            {
                ++size_;
                *head = new node {value, *head};
                return *head;
            }

            void erase_ ()
            {
            }

        private:
            node  *head_ = nullptr;
            size_t size_ = 0;
    };
}

int main (int argc, char **argv)
{
    list::linked<int> list;

    list.push_front (1);
    list.push_front (2);
    list.push_front (3);

    list.insert_before (std::end (list), 4);

    for (auto v : list)
        cout << v << endl;

    return 0;
}
