#ifndef POWER_UP_H
#define POWER_UP_H

#include "Vertex.h"

class PowerUp
{
public:
    enum PowerUpType
    {
        WEAPON,
        ARMOUR, 
        HEALTH
    };

    PowerUp(const char* name, const Vertex& position)
        : mPosition(position), mName(name)
    {
    }

    virtual ~PowerUp()
    {
    }

    PowerUpType GetPowerUpType() const
    {
        return mType;
    }

    const Vertex& GetPosition() const
    {
        return mPosition;
    }

protected:
    Vertex      mPosition;
    PowerUpType mType;
    std::string mName;
};

#endif // POWER_UP_H
