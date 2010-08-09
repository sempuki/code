#ifndef HEALTH_H
#define HEALTH_H

#include "PowerUp.h"

class Health : public PowerUp
{
public:
    Health(const char* name, const Vertex& position) :
        PowerUp(name, position)
    {
        mType = PowerUp::HEALTH;
    }

    virtual ~Health()
    {
    }
    
protected:
    float healing;
};

#endif // HEALTH_H

