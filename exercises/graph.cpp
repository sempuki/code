/* tree.cpp --
 *
 *			Ryan McDougall
 */

#include <iostream>
#include <algorithm>
#include <vector>
#include <list>
#include <stack>
#include <queue>

#include <tr1/functional>

using namespace std;
using namespace std::tr1;
using namespace std::tr1::placeholders;

// note: without loss of generality, 
// all vertices are integer named
// and edges are pairs of vertices

// Adjacency Matrix-based implementation

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

        EdgeIterator edge_begin (int v) const { return EdgeIterator (adj_[v], 0); }
        EdgeIterator edge_end (int v) const { return EdgeIterator (adj_[v], adj_[v].size()); }

        void add () 
        { 
            resize_ (vertices() + 1);
        }

        void remove (int v)
        {
            for (int i = 0; i < vertices(); ++i)
                cut (i, v);
        }

        bool joined (int v, int w) const 
        { 
            return (adj_[v][w] || adj_[w][v]);
        }

        void join (int v, int w)
        {
            if (!joined (v, w))
            {
                ++ edges_;
                adj_[v][w] = 
                    adj_[w][v] = true;
            }
        }

        void cut (int v, int w)
        {
            if (joined (v, w))
            {
                -- edges_;
                adj_[v][w] = 
                    adj_[w][v] = false;
            }
        }

    private:
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

// Adjacency List-based implementation
class SparseGraph
{
    public:
        typedef vector <int> AdjacencyVector;
        typedef vector <AdjacencyVector> AdjacencyList;

        typedef AdjacencyVector::const_iterator EdgeIterator;

        SparseGraph (int n) :
            edges_ (0)
        {
            resize_ (n);
        }

        size_t vertices () const { return adj_.size(); }
        size_t edges () const { return edges_; }

        EdgeIterator edge_begin (int v) const { return adj_[v].begin(); }
        EdgeIterator edge_end (int v) const { return adj_[v].end(); }

        void add () 
        { 
            resize_ (vertices() + 1);
        }

        void remove (int v)
        {
            for (int i = 0; i < vertices(); ++i)
                cut (i, v);
        }

        bool joined (int v, int w) const 
        { 
            return (find_ (adj_[v], w) || find_ (adj_[w], v));
        }

        void join (int v, int w)
        {
            if (!joined (v, w))
            {
                ++ edges_;
                insert_ (adj_[v], w);
                insert_ (adj_[w], v);
            }
        }

        void cut (int v, int w)
        {
            if (joined (v, w))
            {
                -- edges_;
                erase_ (adj_[v], w);
                erase_ (adj_[w], v);
            }
        }

    private:
        bool find_ (const AdjacencyVector &v, int w) const
        {
            return (find (v.begin(), v.end(), w) != v.end());
        }

        void insert_ (AdjacencyVector &v, int w)
        {
            v.push_back (w);
        }

        void erase_ (AdjacencyVector &v, int w)
        {
            v.erase (find (v.begin(), v.end(), w));
        }

        void resize_ (int n)
        {
            adj_.resize (n);
        }

    private:
        AdjacencyList   adj_;
        size_t          edges_;
};

// note: O(V^2) since each vertex is visited, with a search of all
// adjacent vertices via the EdgeIterator::operator++
template <typename Graph>
struct TraverseDFS
{
    const Graph &graph; 
    TraverseDFS (const Graph &g) : graph (g) {}

    void operator() (function <void(int)> fn, int start)
    {
        vector <bool> visited (graph.vertices());
        stack <int> next;

        visited [start] = true;
        next.push (start);

        for (int v; next.size ();)
        {
            v = next.top ();
            next.pop ();

            fn (v); // apply operator

            typename Graph::EdgeIterator i = graph.edge_begin (v);
            typename Graph::EdgeIterator e = graph.edge_end (v);
            for (; i != e; ++i)
            {
                if (!visited [*i]) 
                {
                    visited [*i] = true;
                    next.push (*i);
                }
            }
        }
    }
};

void print (int v) { cout << "found vertex: " << v << endl; }


//=============================================================================
// Main entry point
int
main (int argc, char** argv)
{
    DenseGraph graph (7);
    graph.join (0, 1);
    graph.join (0, 2);
    graph.join (1, 3);
    graph.join (1, 4);
    graph.join (2, 5);
    graph.join (2, 6);

    for (int v = 0; v < graph.vertices(); ++v)
    {
        cout << "vertex " << v << " is joined with : ";
        DenseGraph::EdgeIterator i = graph.edge_begin (v);
        DenseGraph::EdgeIterator e = graph.edge_end (v);
        for (; i != e; ++i)
            cout << *i << ", ";
        cout << endl;
    }

    TraverseDFS <DenseGraph> traverse (graph);
    traverse (print, 0);

    return 0;
}
