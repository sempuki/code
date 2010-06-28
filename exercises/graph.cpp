/* tree.cpp --
 *
 *			Ryan McDougall
 */

#include <iostream>
#include <algorithm>
#include <stack>
#include <queue>

#include <tr1/functional>

using namespace std;
using namespace std::tr1;
using namespace std::tr1::placeholders;

template <typename T>
void erase_nth (T &v, int n)
{
    v.erase (v.begin() + n);
}

// note: without loss of generality, 
// all vertices are integer named
// and edges are integer pairs

class DenseGraph
{
    public:
        typedef vector <bool> AdjacencyVector;
        typedef vector <AdjacencyVector> AdjacencyMatrix;

        class EdgeIterator : public std::iterator <std::forward_iterator_tag, int>
        {
            public:
                EdgeIterator (const AdjacencyVector &v, int p = 0) 
                    : v_ (v), p_ (p) 
                {
                    // find first adjacent vertex
                    for (; (p_ < v_.size()) && !v_[p_]; ++p_); 
                }

                int &operator* ()
                { 
                    return p_; 
                }

                EdgeIterator &operator++ () 
                { 
                    for (++p_; (p_ < v_.size()) && !v_[p_]; ++p_); 
                    return *this; 
                }

                EdgeIterator operator++ (int) 
                { 
                    EdgeIterator t (*this);  
                    ++ (*this);
                    return t; 
                }

                bool operator== (const EdgeIterator &rhs) const 
                { 
                    return (v_ == rhs.v_) && (p_ == rhs.p_); 
                }
                
                bool operator!= (const EdgeIterator &rhs) const 
                { 
                    return !(*this == rhs); 
                }

            private:
                const AdjacencyVector &v_;
                int p_;
        };

        DenseGraph (int n) :
            edges_ (0)
        {
            resize_ (n);
        }

        size_t vertices () const { return adj_.size(); }
        size_t edges () const { return edges_; }
        bool edge (int v, int w) { return joined_ (v, w); }

        EdgeIterator edge_begin (int v) { return EdgeIterator (adj_[v], 0); }
        EdgeIterator edge_end (int v) { return EdgeIterator (adj_[v], adj_[v].size()); }

        void add () 
        { 
            resize_ (vertices() + 1);
        }

        void remove (int v)
        {
            erase_ (v);
        }

        void insert (int v, int w)
        {
            if (!joined_ (v, w))
            {
                join_ (v, w);
                ++ edges_;
            }
        }

        void remove (int v, int w)
        {
            if (joined_ (v, w))
            {
                cut_ (v, w);
                -- edges_;
            }
        }

    private:
        bool joined_ (int v, int w)
        {
            return (adj_[v][w] || adj_[w][v]);
        }

        void join_ (int v, int w)
        {
            adj_[v][w] = adj_[w][v] = true;
        }

        void cut_ (int v, int w)
        {
            adj_[v][w] = adj_[w][v] = false;
        }

        void erase_ (int v)
        {
            for (int i = 0; i < vertices(); ++i)
                cut_ (i, v);
        }

        void resize_ (int n)
        {
            adj_.resize (n);
            for_each (adj_.begin(), adj_.end(), 
                    bind (&AdjacencyVector::resize, _1, n, false));
        }

    private:
        AdjacencyMatrix adj_;
        size_t          edges_;
};

//=============================================================================
// Main entry point
int
main (int argc, char** argv)
{
    DenseGraph graph (5);
    graph.insert (1, 0);
    graph.insert (2, 1);
    graph.insert (3, 2);
    graph.insert (4, 3);

    for (int v = 0; v < graph.vertices(); ++v)
    {
        cout << "vertex " << v << " is joined with : ";
        DenseGraph::EdgeIterator i = graph.edge_begin (v);
        DenseGraph::EdgeIterator e = graph.edge_end (v);
        for (; i != e; ++i)
            cout << *i << ", ";
        cout << endl;
    }

    return 0;
}
