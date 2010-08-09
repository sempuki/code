#ifndef PATH_NODE_H
#define PATH_NODE_H

#include "Vertex.h"

class PathNode;
typedef std::vector<PathNode*> PathNodes;

class PowerUp;
typedef std::vector<PowerUp*> PowerUps;

class PathNode
{
public:
    PathNode(const char* name, const Vertex& position) :
        mPosition(position), mName(name)
    {
    }
    
    ~PathNode()
    {
    }

    void AddLink(PathNode *pathNode)
    {
        mLinks.push_back(pathNode);
    }
    
    void RemoveLink(PathNode *pathNode)
    {
        PathNodes::iterator i = std::find(mLinks.begin(), mLinks.end(), pathNode);
        mLinks.erase(i);
    }

    void AddPowerUp(PowerUp *powerUp)
    {
        mPowerUps.push_back(powerUp);
    }
    
    void RemovePowerUp(PowerUp *powerUp)
    {
        PowerUps::iterator i = std::find(mPowerUps.begin(), mPowerUps.end(), powerUp);
        mPowerUps.erase(i);
    }

    const char* GetName() const
    {
        return mName.c_str();
    }

    const PathNodes& GetLinks() const
    {
        return mLinks;
    }

    const PowerUps& GetPowerUps() const
    {
        return mPowerUps; 
    }

    float Distance (const PathNode &end) const
    {
        float x (end.mPosition.x - mPosition.x);
        float y (end.mPosition.y - mPosition.y);
        float z (end.mPosition.z - mPosition.z);
        return sqrtf (x*x + y*y + z*z);
    }

protected:
    Vertex      mPosition;
    std::string mName;

    PathNodes   mLinks;
    PowerUps    mPowerUps;
};

#endif // PATH_NODE_H
