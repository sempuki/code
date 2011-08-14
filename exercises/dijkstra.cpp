#include <iostream>
#include <algorithm>
#include <iterator>
#include <vector>
#include <cstring>
#include <climits>

int const NVERTICES = 10;
int graph[NVERTICES][NVERTICES]; 
// graph[a][b] = n; => edge (a, b) has weight n

template<int N>
std::vector<int> find_shortest_path(int const graph[N][N], int a, int b)
{
    using std::vector;
    using std::fill_n;

    vector<int> path;
    bool visited[N];
    int distance[N];
    int parent[N];
    int weight;
    
    fill_n(visited, N, false);
    fill_n(distance, N, INT_MAX);
    
    int current = a;
    distance[current] = 0;

    while (!visited[current])
    {
        visited[current] = true;

        // find shortest path to i through current
        for (int i=0; i < N; ++i)
        {
            weight = graph[current][i];

            if (!visited[i] && weight >= 0 && distance[current] + weight < distance[i])
            {
                distance[i] = distance[current] + weight;
                parent[i] = current;
            }
        }

        // find next (min distance) vertex to visit
        for (int i=0, D=INT_MAX; i < N; ++i)
            if (!visited[i] && distance[i] < D)
                D = distance[i], current = i;
    }

    // record shortest path from parent list
    for (current = b; current != a; current = parent[current])
        path.push_back(current);
    path.push_back(a);

    reverse(path.begin(), path.end());

    return path;
}

int main()
{
    using std::cout;
    using std::vector;
    using std::copy;
    using std::fill_n;
    using std::ostream_iterator;

    // null edges have negative weight
    for (int i=0; i < NVERTICES; ++i)
        fill_n(graph[i], NVERTICES, -1);

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

    vector<int> path = find_shortest_path(graph, 1, 5);
    copy(path.begin(), path.end(), ostream_iterator<int> (cout, ", "));
    cout << std::endl;

    return 0;
}
