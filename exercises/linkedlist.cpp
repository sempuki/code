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
                node **head = previous_ (position.ptr_);
                return iterator {*head};
            }

        public:
            iterator push_front (T const &value)
            {
                node **head = &head_;
                return iterator {insert_ (head, value)};
            }

            iterator insert_after (iterator position, T const &value)
            {
                node **head = position.ptr_;
                return iterator {insert_ (head, value)};
            }

            iterator insert_before (iterator position, T const &value)
            {
                node **head = previous_ (position.ptr_);
                return iterator {insert_ (head, value)};
            }

        public:
            void erase_inclusive (iterator begin, iterator end)
            {
                node **head = previous_ (begin.ptr_);
                node  *tail = end.ptr_;
                erase_ (head, tail);
            }

            void erase_exclusive (iterator begin, iterator end)
            {
                node **head = next_ (begin.ptr_);
                node  *tail = end.ptr_;
                erase_ (head, tail);
            }

        private:
            node **next_ (node *item)
            {
                return &item->next;
            }

            node **previous_ (node *item)
            {
                node **prev = &head_;

                while (*prev && *prev != item)
                    prev = &(*prev)->next;

                return prev;
            }
            // inserts new node after head
            node *insert_ (node **head, T const &value)
            {
                ++size_;
                *head = new node {value, *head};
                return *head;
            }

            // removes nodes after head until tail; ie. (head, tail)
            void erase_ (node **head, node *tail)
            {
                node *curr = *head;
                *head = tail;

                while (curr != tail)
                {
                    auto orphan = curr;
                    curr = curr->next;
                    delete orphan;
                    --size_;
                }
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

    list.erase_inclusive (std::begin (list), std::end (list));

    return 0;
}
