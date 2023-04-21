#ifndef PAGER_HH
#define PAGER_HH

#include <cstdint>

#include "pager.hh"


/* Abstract Pager Class */
class Pager
{
public:
    virtual uint32_t select_victim_frame() = 0;
};

class FifoPager : public Pager
{
private:
    uint32_t hand{};
    uint32_t frames{};
public:
    FifoPager(uint32_t & f) : frames(f) {}
    virtual uint32_t select_victim_frame() override;
};

#endif