/* tree.cpp --
 *
 *			Ryan McDougall
 */

#include <iostream>
#include <algorithm>
#include <vector>
#include <stack>
#include <queue>

using namespace std;

namespace graph
{
    typedef int Vertex;
    typedef int Weight;

    struct Edge
    {
        Vertex vertex;
        Weight weight;

        Edge () : vertex {-1}, weight {-1} {}
        Edge (Vertex v, Weight w) : vertex {v}, weight {w} {}
    };

    typedef std::vector<Edge>               EdgeList;
    typedef std::vector<EdgeList>           AdjacencyList;

    typedef EdgeList::iterator              EdgeIterator;
    typedef EdgeList::const_iterator        EdgeConstIterator;

    class SparseGraph
    {
        public:
            SparseGraph (size_t nv = 1) : adjacent_ {nv}, nedges_ {0} {}

        public:
            size_t nedges () const { return nedges_; }
            size_t nvertices () const { return adjacent_.size(); }

        public:
            EdgeIterator edge_begin (int v) { return std::begin (adjacent_[v]); }
            EdgeConstIterator edge_begin (int v) const { return std::begin (adjacent_[v]); }

            EdgeIterator edge_end (int v) { return std::end (adjacent_[v]); }
            EdgeConstIterator edge_end (int v) const { return std::end (adjacent_[v]); }

        public:
            bool has_edge (Vertex v, Vertex w) const 
            { 
                bool result = false;

                if (v < adjacent_.size())
                {
                    EdgeList const &connected = adjacent_[v];
                    EdgeConstIterator begin = std::begin (connected);
                    EdgeConstIterator end = std::end (connected);

                    auto has_vertex = [w] (Edge const &e) { return e.vertex == w; };
                    auto itr = std::find_if (begin, end, has_vertex);

                    result = itr != end;
                }

                return result;
            }

            Weight edge_weight (Vertex v, Vertex w) const
            {
                Weight result = -1;

                if (v < adjacent_.size())
                {
                    EdgeList const &connected = adjacent_[v];
                    EdgeConstIterator begin = std::begin (connected);
                    EdgeConstIterator end = std::end (connected);

                    auto has_vertex = [w] (Edge const &e) { return e.vertex == w; };
                    auto itr = std::find_if (begin, end, has_vertex);

                    if (itr != end)
                        result = itr->weight;
                }

                return result;
            }

            void join (Vertex v, Vertex w, Weight weight = -1)
            {
                if (std::max (v, w) >= adjacent_.size())
                {
                    // resize connected to accommodate all edges
                    adjacent_.resize (std::max (v, w) + 1);
                }

                Edge edge {w, weight};

                EdgeList &connected = adjacent_[v];
                EdgeIterator itr = std::begin (connected);
                EdgeIterator end = std::end (connected);

                // try to keep edges in sorted vertex order
                for (; itr != end && itr->vertex < edge.vertex; ++itr);

                connected.insert (itr, edge);

                ++ nedges_;
            }

            void cut (Vertex v, Vertex w)
            {
                if (v < adjacent_.size())
                {
                    EdgeList &connected = adjacent_[v];
                    EdgeIterator begin = std::begin (connected);
                    EdgeIterator end = std::end (connected);

                    auto has_vertex = [w] (Edge const &e) { return e.vertex == w; };
                    auto itr = std::find_if (begin, end, has_vertex);

                    if (itr != end)
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
                        parent [itr->vertex] = vertex;
                        fringe.push (itr->vertex);
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

    template <typename Graph>
    struct MinSpanningTree
    {
        Graph const &graph; 
        
        std::vector<Vertex> parent;
        std::vector<bool>   visited;

        MinSpanningTree (const Graph &g) : 
            graph (g) {}

        void operator() (int start)
        {
        }
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
            cout << i->vertex << ", ";
        cout << endl;
    }

    graph::Traversal <graph::SparseGraph, graph::stack> traverse (graph);
    traverse (0, [](int v) { cout << "visited: " << v << endl; }, 
                 [](int v) { cout << "processed: " << v << endl; });

    return 0;
}
