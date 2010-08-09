
#include <set>
#include <vector>
#include <string>
#include <algorithm>

#include <cmath>
#include <cstdio>

#include "PathNode.h"
#include "PowerUp.h"
#include "Weapon.h"
#include "Health.h"
#include "Armor.h"

static PathNodes sPathNodes;
static PowerUps sPowerUps;

// NOTE: Question 3 -- I feel my creativity is best expressed in 
// terms of my fledgling open source game engine here: 
// == http://github.com/sempuki/scaffold


// ## Question 2
//
// resources used:
// * http://en.wikipedia.org/wiki/Dijkstra%27s_algorithm
// * http://www.cplusplus.com/reference/
//
// algorithm: 
// after much back-forth, I decided to stick with a simple heuristic
// that the nearest powerups were the nearest neighbors of the start node, since
// I presume that level designers will make their graphs spatially coherent
// ie. non-Escher-like. For example, there may be a powerup nearby, but hidden
// behind a wall; in this case the graph should accurately reflect this reality
// by keeping a way-node in between (so as the player can navigate *around* the
// wall). If it's the case that node can be spatially distant, but proximal
// within the graph, then this algorithm can be fooled in some cases.
//
// The pathological case for this heuristic is when all powerups are spread 
// to the edges of the map. This will cause the algorithm to have O(N) nodes
// is its search set. However in this case, I am not sure how we can do 
// much better than searching the entire node space any way without special 
// short-cut lookup information.
//
// space and time complexity in number of Nodes N 
// -- best case: O(1)
// -- worst case: O(N)
//
// implementation notes:
// - too many heap allocations, would need a small object allocator
// - relies on pointer not object identity (ie. doesn't compare by GetName())
//
// Alternative algorithms considered:
// - modified Dijkstra's to find all shortest paths, stopping when powerup is found
// - find all powerups and do two Dijkstra's; start-> <-powerup
// - find all powerups and use a directional heuristic against a vector 
//   pointing towards start, building a path, then choosing the least cost/distance
// - recursive search of the entire space (better memory perf than heap)
// - A* -- not enough time to learn it
//

// records a path through the graph, and it's cost (distance)
struct PathSearchNode 
{
    PathSearchNode *parent;
    PathNode *node;
    float cost;
    bool found;

    PathSearchNode (PathSearchNode *p, PathNode *n, float c)
        : parent (p), node (n), cost (c) {}
};

typedef std::set <PathSearchNode *> PathNodeSearch;
typedef std::set <PathNode *> PathNodeSet;

bool FindPowerUp(PathNodes& path, PowerUp::PowerUpType mType, PathNode *start)
{
    bool found = false;
    float cost;

    PathNodeSet visited;
    PathNodeSearch search, next, alloc;
    search.insert (new PathSearchNode (0, start, 0.0f));

    while (search.size())
    {
        // for each possible path in our current search
        PathNodeSearch::const_iterator si = search.begin();
        PathNodeSearch::const_iterator se = search.end();
        for (PathSearchNode *s; si != se; ++si)
        {
            s = *si;

            // remember which nodes are visited
            visited.insert (s->node);

            // find the required powerup
            PowerUps::const_iterator pi = s->node->GetPowerUps().begin();
            PowerUps::const_iterator pe = s->node->GetPowerUps().end();
            for (PowerUp *p; pi != pe; ++pi)
            {
                p = *pi;

                if (p->GetPowerUpType() == mType) 
                    s->found = found = true;
            }

            // search deeper
            if (!found)
            {
                // for each new branch to add to our next search
                PathNodes::const_iterator ni = s->node->GetLinks().begin();
                PathNodes::const_iterator ne = s->node->GetLinks().end();
                for (PathNode *n; ni != ne; ++ni)
                {
                    n = *ni;

                    if (!visited.count (n))
                    {
                        cost = s->cost + n->Distance (*(s->node));
                        next.insert (new PathSearchNode (s, n, cost));
                    }
                }

            }
        }

        // stop when found
        if (found) break;

        // collect our memory
        alloc.insert (search.begin(), search.end());

        // start next search
        search.clear ();
        search.swap (next);
    }
    
    // path was found
    if (found)
    {
        // find the lowest cost version
        PathSearchNode *result (0);
        PathNodeSearch::const_iterator si = search.begin();
        PathNodeSearch::const_iterator se = search.end();
        for (PathSearchNode *s; si != se; ++si)
        {
            s = *si;

            if (((s->found)) && (!result || (s->cost < result->cost)))
                result = s;
        }

        // walk linked list and push into result
        for (PathSearchNode *s = result; s; s = s->parent)
            path.push_back (s->node);

        // reverse result
        for (int i=0, j=path.size()-1; i < j; ++i, --j)
            std::swap (path[i], path[j]);
    }

    // reclaim memory
    // TODO small object allocator makes this much faster/easier
    PathNodeSearch::const_iterator i = alloc.begin();
    PathNodeSearch::const_iterator e = alloc.end();
    for (; i != e; ++i)
        delete *i;

    return found;
}

// For this example, all links are symmetric.
inline void LinkNodes(PathNode *n1, PathNode *n2)
{
    n1->AddLink(n2);
    n2->AddLink(n1);
}

int main(int, char*[])
{
    sPathNodes.push_back(new PathNode("Node0", Vertex(300, 60, 0)));
    sPathNodes.push_back(new PathNode("Node1", Vertex(100, 60, 0)));
    sPathNodes.push_back(new PathNode("Node2", Vertex(80, 560, 0)));
    sPathNodes.push_back(new PathNode("Node3", Vertex(280, 650, 0)));
    sPathNodes.push_back(new PathNode("Node4", Vertex(300, 250, 0)));
    sPathNodes.push_back(new PathNode("Node5", Vertex(450, 400, 0)));
    sPathNodes.push_back(new PathNode("Node6", Vertex(450, 60, 0)));
    sPathNodes.push_back(new PathNode("Node7", Vertex(450, 400, 0)));

    LinkNodes(sPathNodes[1], sPathNodes[4]);
    LinkNodes(sPathNodes[0], sPathNodes[1]);
    LinkNodes(sPathNodes[0], sPathNodes[6]);
    LinkNodes(sPathNodes[0], sPathNodes[4]);
    LinkNodes(sPathNodes[7], sPathNodes[4]);
    LinkNodes(sPathNodes[7], sPathNodes[5]);
    LinkNodes(sPathNodes[2], sPathNodes[4]);
    LinkNodes(sPathNodes[2], sPathNodes[3]);
    LinkNodes(sPathNodes[3], sPathNodes[5]);

    sPowerUps.push_back(new Weapon("Weapon0", Vertex(340, 670, 0)));
    sPathNodes[3]->AddPowerUp(sPowerUps[0]);    
    sPowerUps.push_back(new Weapon("Weapon1", Vertex(500, 220, 0)));
    sPathNodes[7]->AddPowerUp(sPowerUps[1]);    

    sPowerUps.push_back(new Health("Health0", Vertex(490, 10, 0)));
    sPathNodes[6]->AddPowerUp(sPowerUps[2]);    
    sPowerUps.push_back(new Health("Health1", Vertex(120, 20, 0)));
    sPathNodes[1]->AddPowerUp(sPowerUps[3]);    

    sPowerUps.push_back(new Armor("Armour0", Vertex(500, 360, 0)));
    sPathNodes[5]->AddPowerUp(sPowerUps[4]);    
    sPowerUps.push_back(new Armor("Armour1", Vertex(180, 525, 0)));
    sPathNodes[2]->AddPowerUp(sPowerUps[5]);    

    PathNodes path;

    if(!FindPowerUp(path, PowerUp::WEAPON, sPathNodes[4]))
    {
        printf("No path found: IMPOSSIBLE!\n");
    }
    else
    {
        printf("Path found: ");

        for(PathNodes::iterator i = path.begin(); i != path.end(); ++i)
        {
            PathNode *n = *i;
            printf("%s ", n->GetName());
        }

        printf("\n");
    }
       
    return(0);
}
