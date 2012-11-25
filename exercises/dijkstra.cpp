#include <iostream>
#include <algorithm>
#include <iterator>
#include <vector>
#include <cstring>
#include <climits>

using std::cout;
using std::endl;

int const NVERTICES = 10;
int graph[NVERTICES][NVERTICES]; 
// graph[a][b] = n; => edge (a, b) has weight n

template<int N>
std::vector<int> find_shortest_path(int const graph[N][N], int a, int b)
{
    std::vector<int> shortest_path;

    bool visited[N];    // which vertices have been visited
    int  distance[N];   // min distance (in weight) from from vertex a to i
    int  parent[N];     // shortest path from a to all i (in reverse)
    
    std::fill_n (visited, N, false);
    std::fill_n (distance, N, INT_MAX);
    
    int weight, current = a;
    distance[current] = 0;

    while (!visited[current])
    {
        visited[current] = true;

        // find shortest path from a to i through current
        for (int i=0; i < N; ++i)
        {
            weight = graph[current][i];
            
            if (!visited[i] && weight >= 0) // not visited and has edge
            {
                if (distance[current] + weight < distance[i]) // has shortcut via current
                {
                    distance[i] = distance[current] + weight;
                    parent[i] = current;
                }
            }
        }

        // extend path through minimum weighted edge
        for (int i=0, D=INT_MAX; i < N; ++i)
            if (!visited[i] && distance[i] < D)
                D = distance[i], current = i;
    }

    // walk parent list in reverse from b to a to find path
    for (current = b; current != a; current = parent[current])
        shortest_path.push_back (current);
    shortest_path.push_back (a);

    std::reverse (std::begin (shortest_path), std::end (shortest_path));

    return shortest_path;
}

int main()
{
    // null edges have negative weight
    for (int i=0; i < NVERTICES; ++i)
        std::fill_n (graph[i], NVERTICES, -1);

    // test map
    graph[1][2] = graph[2][1] = 7; 
    graph[1][3] = graph[3][1] = 9; 
    graph[1][6] = graph[6][1] = 14; 
    graph[2][3] = graph[3][2] = 10; 
    graph[2][4] = graph[4][2] = 15; 
    graph[3][4] = graph[4][3] = 11; 
    graph[3][6] = graph[6][3] = 2; 
    graph[4][5] = graph[5][4] = 6; 
    graph[5][6] = graph[6][5] = 9; 
    graph[1][1] = graph[2][2] = 
    graph[3][3] = graph[4][4] = 
    graph[5][5] = 0; 

    std::vector<int> path = find_shortest_path (graph, 1, 5);

    std::copy (std::begin (path), std::end (path), 
            std::ostream_iterator<int> (cout, ", "));
    cout << endl;

    return 0;
}
