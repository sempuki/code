#include <array>
#include <iostream>
#include <limits>
#include <queue>
#include <vector>
using namespace std;

template <typename Weight, size_t Dimension>
class DenseGraph {
   public:
    DenseGraph(const Weight& initial) {
        for (auto&& row : graph_) {
            row.fill(initial);
        }
    }

    void join(int v, int w, Weight weight) {
        graph_[v][w] = graph_[w][v] = weight;
    }

    const auto& edges(int v) const { return graph_[v]; }

   private:
    std::array<std::array<Weight, Dimension>, Dimension> graph_;
};

template <typename Weight>
class SparseGraph {
   public:
    struct Node {
        Weight weight = {};
        int adjacent = -1;
    };

    SparseGraph(size_t nvertices) { graph_.resize(nvertices); }

    void join(int v, int w, Weight weight) {
        graph_[v].emplace_back(weight, w);
        graph_[w].emplace_back(weight, v);
    }

    const auto& edges(int v) const { return graph_[v]; }

   private:
    std::vector<std::vector<Node>> graph_;
};

int main() {
    size_t nqueries;
    cin >> nqueries;

    while (nqueries--) {
        size_t nvertices, nedges;
        cin >> nvertices >> nedges;

        DenseGraph<int, 1000> graph{-1};
        while (nedges--) {
            size_t v, w;
            cin >> v >> w;
            graph.join(v - 1, w - 1, 6);  // from zero
        }

        size_t start;
        cin >> start;

        std::vector<bool> visited(nvertices);
        std::vector<int> cost(nvertices);

        std::fill(begin(visited), end(visited), false);
        std::fill(begin(cost), end(cost), numeric_limits<int>::max());

        std::queue<int> search;
        search.push(start - 1);  // from zero
        cost[start - 1] = 0;

        while (!search.empty()) {
            auto current = search.front();
            visited[current] = true;

            auto&& weight = graph.edges(current);
            for (size_t v = 0; v < nvertices; ++v) {
                if (!visited[v] && weight[v] > 0) {
                    cost[v] = std::min(cost[current] + weight[v], cost[v]);
                    search.push(v);  // visit next
                }
            }
            search.pop();
        }

        for (int v = 0; v < nvertices; ++v) {
            if (v != start - 1) {
                cout << (cost[v] < numeric_limits<int>::max() ? cost[v] : -1)
                     << ' ';
            }
        }
        cout << endl;
    }
}
