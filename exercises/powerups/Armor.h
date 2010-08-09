#ifndef ARMOUR_H
#define ARMOUR_H

#include "PowerUp.h"

class Armor : public PowerUp
{
public:
    Armor(const char* name, const Vertex& position) :
        PowerUp(name, position)
    {
        mType = PowerUp::ARMOUR;
    }

    virtual ~Armor()
    {
    }

    const char* GetClanTag() const
    {
        return mClanTag.c_str();
    }

    void SetClanTag(char* n)
    {
        mClanTag.assign (n);
    }

    float GetProtection ()
    {
        return protection;
    }

protected:
    std::string mClanTag;

    float       protection;
};

#endif // ARMOUR_H

