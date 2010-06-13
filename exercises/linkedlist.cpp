/* linkedlist.cpp -- 
 *
 *			Ryan McDougall
 */

#include <iostream>
#include <iomanip>
#include <iterator>
#include <algorithm>
#include <tr1/functional>

using namespace std;
using namespace std::tr1;
using namespace std::tr1::placeholders;

template <typename T>
struct safe_delete { void operator() (T* &ptr) { delete ptr; ptr = 0; } };

template <typename T>
struct Node
{
    T value;
    Node <T> *next;

    Node (Node <T> *p = 0) : next (p) {}
    Node (T v, Node <T> *p = 0) : value (v), next (p) {}
};

template <typename T>
class ListIterator : public std::iterator <std::forward_iterator_tag, T>
{
    public:
        typedef ListIterator <T> iterator;
        typedef Node <T> node;

        ListIterator (node *p = 0) : p_ (p) {}

        operator node *() const { return p_; }
        iterator operator= (node *p) { p_ = p; }

        T &operator* () const { return p_->value; }
        T *operator-> () const { return &(p_->value); }

        bool operator== (const iterator &rhs) const { return p_ == rhs.p_; }
        bool operator!= (const iterator &rhs) const { return p_ != rhs.p_; }

        node *next () { return p_->next; }
        void next (node *n) { p_->next = n; }

        iterator &operator++ () { p_ = next(); return *this; }
        iterator operator++ (int) { iterator t (*this); p_ = next(); return t; }

    private:
        node *p_;
};

template <typename T>
class List
{
    public:
        typedef ListIterator <T> iterator;
        typedef Node <T> node;

        List () : 
            sen_ (new node), 
            head_ (sen_),
            tail_ (sen_), 
            size_ (0) 
        {}

        List (iterator b, iterator e)
        {
            for (; b != e; ++b)
            {

            }
        }

        iterator begin () { return head_; }
        iterator end () { return iterator(); }

        T &front () { return *head_; }
        T &back () { return *tail_; }

        // erase
        // reverse
        // sort / merge

        iterator previous (iterator pos)
        {
            iterator prev (sen_); 

            for (iterator i (begin()); (i != pos) && (i != end()); ++i)
                prev = i;

            return prev;
        }

        void insert (iterator pos, const T &v) 
        {
            insert_before_ (pos, v);
        }

        void insert_after (iterator pos, const T &v) 
        { 
            insert_after_ (pos, v);
        }

        void insert_after (iterator pos, iterator begin, iterator end)
        { 
            for_each (begin, end, bind (&List::insert_after_, this, pos, _1));
        }

        void splice_after (iterator pos, iterator begin, iterator end)
        {
            end.next (pos.next());
            pos.next (begin);
        }

    private:
        void initial_insert_ (const T &v)
        {
            node *n = new node (v, 0); 
            head_ = tail_ = n;
            sen_.next (n);
            size_ = 1;
        }
        
        void insert_before_ (iterator pos, const T &v) 
        { 
            if (!size_)
                initial_insert_ (v);
            else
            {
                iterator prev (previous (pos));
                prev.next (new node (v, prev.next()));
                ++ size_;
                
                if (pos == head_) head_ = sen_.next();
            }
        }

        void insert_after_ (iterator pos, const T &v)
        {
            if (!size_)
                initial_insert_ (v);
            else
            {
                pos.next (new node (v, pos.next()));
                ++ size_;

                if (pos == tail_) tail_ = tail_.next();
            }
        }

    private:
        iterator    sen_;
        iterator    head_;
        iterator    tail_;
        size_t      size_;
};

//=============================================================================
// Main entry point
int
main (int argc, char** argv)
{
    List <int> list1;

    list1.insert (list1.begin(), 6);
    list1.insert (list1.begin(), 4);
    list1.insert (list1.end(), 7);
    list1.insert_after (list1.begin(), 5);

    List <int> list2 (list1.begin(), list2.begin());

    copy (list1.begin(), list1.end(), ostream_iterator <int> (cout, " "));
    copy (list2.begin(), list2.end(), ostream_iterator <int> (cout, " "));
    
    return 0;
}
