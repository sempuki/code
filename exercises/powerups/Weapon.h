#ifndef WEAPON_H
#define WEAPON_H

#include "PowerUp.h"

class Weapon : public PowerUp
{
public:
    Weapon(const char* name, const Vertex& position) :
        PowerUp(name, position)
    {
        mType = PowerUp::WEAPON;
    }

    virtual ~Weapon()
    {
    }

    float GetDamage ()
    {
        return damage;
    }

protected:
    float damage;
};

#endif // WEAPON_H

