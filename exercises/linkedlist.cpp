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
        iterator operator= (node *p) { p_ = p; return *this; }

        T &operator* () const { return p_->value; }
        T *operator-> () const { return &(p_->value); }

        bool operator== (const iterator &rhs) const { return p_ == rhs.p_; }
        bool operator!= (const iterator &rhs) const { return p_ != rhs.p_; }

        iterator next () { return p_->next; }
        void next (iterator n) { p_->next = n; }

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

        ~List () { erase_ (begin(), end()); }

        size_t size () const { return size_; }
        iterator sentinel () { return sen_; }
        iterator begin () { return head_; }
        iterator tail () { return tail_; }
        iterator end () { return iterator(); }

        void swap (List &rhs)
        {
            using std::swap;

            swap (size_, rhs.size_);
            swap (head_, rhs.head_);
            swap (tail_, rhs.tail_);
        }

        T &front () { return *head_; }
        T &back () { return *tail_; }

        iterator previous (iterator pos)
        {
            iterator prev (sen_); 

            for (iterator i (begin()); (i != pos) && (i != end()); ++i)
                prev = i;

            return prev;
        }

        void push_front (const T &v)
        {
            insert_before_ (head_, v);
        }

        void push_back (const T &v)
        {
            insert_after_ (tail_, v);
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
            for (iterator i (begin); i != end; ++i, ++pos)
                insert_after_ (pos, *i);
        }

        void splice_after (iterator pos, iterator prev)
        {
            splice_after_ (pos, prev, prev.next());
        }

        void splice_after (iterator pos, iterator prev, iterator end)
        {
            splice_after_ (pos, prev, end);
        }

        void erase (iterator pos)
        {
            erase_ (pos, pos.next());
        }

        void erase (iterator begin, iterator end)
        {
            erase_ (begin, end);
        }

        void reverse ()
        {
            using std::swap;

            reverse_ (begin());

            swap (head_, tail_);
            sen_.next (head_);
        }

        void merge (List &rhs)
        {
            //iterator s1 (sen_), s2 (rhs.sen_);

            //for (;;)
            //{
            //    if (*(s2.next()) < *(s1.next()))
            //    {
            //        splice_after (s1, s2);
            //        ++ s2;
            //    }
            //    else
            //        ++ s1;

            //    if (s1 == end())
            //        splice_after (tail_, 
            //}
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

        void splice_after_ (iterator pos, iterator prev, iterator end)
        {
            iterator succ = pos.next();

            pos.next (prev.next());
            prev.next (end.next());
            end.next (succ);

            if (pos == tail_) tail_ = end;
            if (pos == sen_) head_ = sen_.next();
        }
        
        void erase_ (iterator b, iterator e)
        {
            iterator prev (previous (b));
            prev.next (e);
            
            if (b == begin()) head_ = sen_.next();
            if (e == end()) tail_ = prev;
            
            for (; b != e; ++b)
                delete (node *)b;
        }

        iterator reverse_ (iterator head)
        {
            if (head != tail_)
            {
                iterator tail = reverse_ (head.next());
                head.next (tail.next());
                tail.next (head);
            }

            return head;
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
    List <int> a, b;

    a.push_back (4);
    a.push_front (2);
    a.push_back (5);
    a.push_front (1);

    List<int>::iterator i = find (a.begin(), a.end(), 2);
    a.insert_after (i, 3);
    a.erase (i);

    b.insert_after (b.begin(), a.begin(), a.end());
    a.reverse ();
    b.splice_after (b.begin(), a.begin(), a.begin().next());
    //a.swap (b);

    copy (a.begin(), a.end(), ostream_iterator <int> (cout, " ")); cout << endl;
    copy (b.begin(), b.end(), ostream_iterator <int> (cout, " ")); cout << endl;
    
    return 0;
}
