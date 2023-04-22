#ifndef PAGER_HH
#define PAGER_HH

#include <fstream>
#include <cstdint>
#include <vector>

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

class RandomPager : public Pager
{
private:
    uint32_t ofs{};
    uint32_t frames{};
    std::vector<uint32_t> random_numbers;
public:
    RandomPager(uint32_t & f, std::ifstream & fin);
    virtual uint32_t select_victim_frame() override;
};

class AgingPager : public Pager
{
public:
    virtual uint32_t select_victim_frame() override;
};


class ESCPager : public Pager
{
    public:
    virtual uint32_t select_victim_frame() override;
};

class ClockPager : public Pager
{
    public:
    virtual uint32_t select_victim_frame() override;
};

class WorkingSetPager : public Pager
{
    public:
    virtual uint32_t select_victim_frame() override;
};

#endif