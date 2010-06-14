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

        List () : sentinel_ (new node) {}
        ~List () { erase (begin(), end()); }

        size_t size () const
        {
            size_t size;
            iterator i = begin();
            iterator e = end();
            for (; i != e; ++i)
                ++ size;
            return size;
        }

        void swap (List &rhs)
        {
            std::swap (sentinel_, rhs.sentinel_);
        }

        iterator begin () { return sentinel_.next(); }
        iterator end () { return iterator(); }

        T &front () { return *(begin()); }

        iterator previous (iterator pos)
        {
            iterator prev = sentinel_; 
            iterator i = begin();

            for (; i != pos; ++i)
                prev = i;

            return prev;
        }

        void push_front (const T &v)
        {
            insert (begin(), v);
        }

        void pop_front ()
        {
            erase (begin(), begin().next());
        }

        void insert (iterator pos, const T &v) 
        {
            insert_ (previous (pos), v);
        }

        void insert_after (iterator pos, const T &v) 
        { 
            insert_ (pos, v);
        }

        // note: [begin, end)
        void insert_after (iterator pos, iterator begin, iterator end)
        { 
            for (iterator i (begin); i != end; ++i, ++pos)
                insert_ (pos, *i);
        }

        void splice (iterator pos, List &rhs, iterator s)
        {
            splice_ (pos, rhs, previous (s), s);
        }

        // note: [begin, end)
        void splice (iterator pos, List &rhs, iterator begin, iterator end)
        {
            splice_ (pos, rhs, previous (begin), previous (end));
        }

        void splice_after (iterator pos, List &rhs, iterator prev)
        {
            splice_ (pos, rhs, prev, prev.next());
        }

        // note: (prev, last]
        void splice_after (iterator pos, List &rhs, iterator prev, iterator last)
        {
            splice_ (pos, rhs, prev, last);
        }

        void erase (iterator pos)
        {
            erase_ (previous (pos), pos.next());
        }

        // note: [begin, end)
        void erase (iterator begin, iterator end)
        {
            erase_ (previous (begin), end);
        }

        // note: (prev, last]
        void erase_after (iterator prev, iterator last)
        {
            erase_ (prev, last.next());
        }

        void reverse ()
        {
            sentinel_.next (reverse_ (begin()));
        }

        void merge (List &rhs)
        {
            iterator s1 (sentinel_);
            iterator s2 (rhs.sentinel_);
            iterator e = end();

            for (;;)
            {
                if (s1.next() && s2.next())
                {
                    if (*s2.next() < *s1.next())
                        splice_after (s1, rhs, s2);
                    else
                        ++ s1;
                }
                else
                {
                    if (s1.next() == e)
                        while (s2.next())
                            splice_after (s1, rhs, s2);

                    if (s2.next() == e)
                        break;
                }
            }
        }

        void sort ()
        {
        }

    private:
        // note: insert after
        void insert_ (iterator pos, const T &v)
        {
            if (begin() == end())
                sentinel_.next (new node (v, 0));
            else
                pos.next (new node (v, pos.next()));
        }

        // note: (prev, end)
        void erase_ (iterator prev, iterator end)
        {
            iterator i = prev.next();
            while (i != end)
                delete i++;
        
            prev.next (end);
        }

        // note: (prev, last]
        void splice_ (iterator pos, List &rhs, iterator prev, iterator last)
        {
            iterator succ = pos.next();
            pos.next (prev.next());
            prev.next (last.next());
            last.next (succ);
        }
        
        // note: returns reversed head
        iterator reverse_ (iterator head)
        { 
            iterator i = head;
            iterator curr, res;

            for (;;)
            {
                if (i.next())
                {
                    curr = i++;
                    curr.next (res);
                    res = curr;
                }
                else
                {
                    head = i;
                    head.next (res);
                    break;
                }
            }

            return head;
        }

    private:
        iterator    sentinel_;
};

//=============================================================================
// Main entry point
int
main (int argc, char** argv)
{
    List <int> a, b;

    a.push_front (5);
    a.push_front (4);
    a.push_front (2);
    a.push_front (1);

    List<int>::iterator i = find (a.begin(), a.end(), 2);
    a.insert_after (i, 3);
    a.erase (i);

    b.push_front (0);
    b.insert_after (b.begin(), a.begin(), a.end());
    a.reverse ();
    b.splice_after (b.begin(), a, a.previous (a.begin()), a.begin().next());
    a.swap (b);
    b.pop_front ();

    copy (a.begin(), a.end(), ostream_iterator <int> (cout, " ")); cout << endl;
    copy (b.begin(), b.end(), ostream_iterator <int> (cout, " ")); cout << endl;
    
    return 0;
}
