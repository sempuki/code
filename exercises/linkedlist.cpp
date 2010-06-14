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
        ~List () { erase_ (begin(), end()); }

        iterator begin () { return sentinel_.next(); }
        iterator end () { return iterator(); }

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

        T &front () { return *(sentinel_.next()); }

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
            insert_before_ (begin(), v);
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

        void splice_after (iterator pos, List &rhs, iterator prev)
        {
            splice_after_ (pos, rhs, prev, prev.next());
        }

        void splice_after (iterator pos, List &rhs, iterator prev, iterator end)
        {
            splice_after_ (pos, rhs, prev, end);
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
            sentinel_.next (reverse_ (begin()));
        }

        void merge (List &rhs)
        {
            //iterator s1 (sentinel_), s2 (rhs.sentinel_);

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
            //}
        }

    private:
        void insert_before_ (iterator pos, const T &v) 
        { 
            if (begin() == end())
                sentinel_.next (new node (v, 0));
            else
            {
                iterator prev (previous (pos));
                prev.next (new node (v, prev.next()));
            }
        }

        void insert_after_ (iterator pos, const T &v)
        {
            if (begin() == end())
                sentinel_.next (new node (v, 0));
            else
                pos.next (new node (v, pos.next()));
        }

        void splice_after_ (iterator pos, List &rhs, iterator prev, iterator end)
        {
            iterator succ = pos.next();

            pos.next (prev.next());
            prev.next (end.next());
            end.next (succ);
        }
        
        void erase_ (iterator b, iterator e)
        {
            iterator prev (previous (b));
            prev.next (e);
            
            for (; b != e; ++b)
                delete (node *)b;
        }

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

    copy (a.begin(), a.end(), ostream_iterator <int> (cout, " ")); cout << endl;
    copy (b.begin(), b.end(), ostream_iterator <int> (cout, " ")); cout << endl;
    
    return 0;
}
