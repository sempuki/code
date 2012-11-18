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

using namespace std;

namespace graph
{
    typedef int                     Vertex;
    typedef std::vector<Vertex>     VertexList;
    typedef std::vector<VertexList> AdjacencyList;

    typedef VertexList::iterator            VertexIterator;
    typedef VertexList::const_iterator      VertexConstIterator;

    typedef AdjacencyList::iterator         EdgeIterator;
    typedef AdjacencyList::const_iterator   EdgeConstIterator;

    class SparseGraph
    {
        public:
            SparseGraph (size_t nv = 1) : adjacent_ {nv}, nedges_ {0} {}

        public:
            size_t nedges () const { return nedges_; }
            size_t nvertices () const { return adjacent_.size(); }

        public:
            VertexIterator edge_begin (int v) { return adjacent_[v].begin(); }
            VertexConstIterator edge_begin (int v) const { return adjacent_[v].begin(); }

            VertexIterator edge_end (int v) { return adjacent_[v].end(); }
            VertexConstIterator edge_end (int v) const { return adjacent_[v].end(); }

            EdgeIterator vertex_begin () { return adjacent_.begin(); }
            EdgeConstIterator vertex_begin () const { return adjacent_.begin(); }

            EdgeIterator vertex_end () { return adjacent_.end(); }
            EdgeConstIterator vertex_end () const { return adjacent_.end(); }

        public:
            bool has_edge (int v, int w) const 
            { 
                bool result = false;

                if (v < nvertices ())
                {
                    VertexList const &connected {adjacent_[v]};
                    VertexConstIterator itr {std::begin (connected)};
                    VertexConstIterator end {std::end (connected)};

                    result = std::find (itr, end, w) != end;
                }

                return result;
            }

            void join (int v, int w)
            {
                if (std::max (v, w) >= nvertices ())
                {
                    // resize vertices to accommodate all edges
                    adjacent_.resize (std::max (v, w) + 1);
                }

                VertexList &connected = adjacent_[v];
                VertexIterator itr {std::begin (connected)};
                VertexIterator end {std::end (connected)};

                // try to keep vertices in sorted order
                for (; itr != end && *itr < w; ++itr);

                connected.insert (itr, w);
                ++ nedges_;
            }

            void cut (int v, int w)
            {
                if (v < nvertices ())
                {
                    VertexList &connected = adjacent_[v];
                    VertexIterator itr {std::begin (connected)};
                    VertexIterator end {std::end (connected)};

                    if ((itr = std::find (itr, end, w)) != end)
                    {
                        connected.erase (itr);
                        -- nedges_;
                    }
                }
            }

        private:
            AdjacencyList   adjacent_;
            size_t          nedges_;
    };


    // TODO: separate expanding from contracting of the stack based on whether 
    // what you pop has been visited before, to allow processing on first 
    // visiting the parent, or when all children have been visited. This mirrors 
    // what's possible using a recursive solution (processing before or after the 
    // recursive call).

    template <typename Graph, typename Fringe>
    struct Traversal
    {
        Graph const &graph; 
        
        std::vector<int>    order;
        std::vector<Vertex> parent;
        std::vector<bool>   visited;
        std::vector<bool>   processed;

        Traversal (const Graph &g) : 
            graph (g) 
        {
            size_t N = graph.nvertices();

            order.resize (N);
            parent.resize (N);
            visited.resize (N);
            processed.resize (N);

            reset();
        }

        template <typename Visitor, typename Processor>
        void operator() (int start, Visitor visit, Processor process)
        {
            int vertex, time = 0;

            Fringe fringe;
            fringe.push (start);

            while (fringe.size() > 0) 
            {
                vertex = fringe.next();

                if (!visited [vertex])
                {
                    // expand on visit
                    
                    visit (vertex);
                    visited [vertex] = true;
                    order [vertex] = time ++;

                    auto itr = graph.edge_begin (vertex);
                    auto end = graph.edge_end (vertex);

                    for (; itr != end; ++itr)
                    {
                        parent [*itr] = vertex;
                        fringe.push (*itr);
                    }
                }
                else
                {
                    // collapse on process

                    process (vertex);
                    processed [vertex] = true;

                    fringe.pop ();
                }
            }
        }

        void reset ()
        {
            std::fill (std::begin(order), std::end(order),          -1);
            std::fill (std::begin(parent), std::end(parent),        -1);
            std::fill (std::begin(visited), std::end(visited),      false);
            std::fill (std::begin(processed), std::end(processed),  false);
        }
    };

    class stack : public std::stack<Vertex>
    {
        public:
            Vertex &next () { return std::stack<Vertex>::top(); }
            Vertex const &next () const { return std::stack<Vertex>::top(); }
    };

    class queue : public std::queue<Vertex>
    {
        public:
            Vertex &next () { return std::queue<Vertex>::front(); }
            Vertex const &next () const { return std::queue<Vertex>::front(); }
    };
}

//=============================================================================

int main (int argc, char** argv)
{
    graph::SparseGraph graph;
    graph.join (0, 1);
    graph.join (0, 2);
    graph.join (1, 3);
    graph.join (1, 4);
    graph.join (2, 5);
    graph.join (2, 6);

    for (int v = 0; v < graph.nvertices(); ++v)
    {
        cout << "vertex " << v << " is joined with : ";
        auto i = graph.edge_begin (v);
        auto e = graph.edge_end (v);
        for (; i != e; ++i)
            cout << *i << ", ";
        cout << endl;
    }

    graph::Traversal <graph::SparseGraph, graph::stack> traverse (graph);
    traverse (0, [](int v) { cout << "visited: " << v << endl; }, 
                 [](int v) { cout << "processed: " << v << endl; });

    return 0;
}
