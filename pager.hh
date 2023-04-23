#ifndef PAGER_HH
#define PAGER_HH

#include <fstream>
#include <cstdint>
#include <vector>

#include "pager.hh"

// Extern functions
uint32_t get_rbit(uint32_t idx);
uint32_t get_mbit(uint32_t idx);
void reset_rbit(uint32_t idx);


/* Abstract Pager Class */
class Pager
{
protected:
    uint32_t frames{};
public:
    Pager()=default;
    Pager(uint32_t & f) : frames(f) {}
    virtual uint32_t select_victim_frame() = 0;
    virtual void print_info() const = 0;

    virtual ~Pager()=default;
};

class FifoPager : public Pager
{
protected:
    uint32_t hand{};
public:
    FifoPager(uint32_t & f) : Pager(f) {}
    virtual uint32_t select_victim_frame() override;
    virtual void print_info() const override;
};

class RandomPager : public Pager
{
private:
    uint32_t ofs{};
    std::vector<uint64_t> random_numbers;
public:
    RandomPager(uint32_t & f, std::ifstream & fin);
    virtual uint32_t select_victim_frame() override;
    virtual void print_info() const override
    {}
};

class AgingPager : public Pager
{
public:
    AgingPager(uint32_t & f);
    virtual uint32_t select_victim_frame() override;
    virtual void print_info() const override;
};


class NRUPager : public Pager
{
private:
    bool first{true};
    uint32_t hand{};
    uint32_t pre_inst{};
    /* Before */
    uint32_t pre_hand{};
    uint32_t reset{};
    /* After */
    uint32_t lowest_class{};
    uint32_t victim{};
    uint32_t steps{};
public:
    NRUPager(uint32_t & f) : Pager(f) {}
    virtual uint32_t select_victim_frame() override;
    virtual void print_info() const override;
};

class ClockPager : public FifoPager
{
private:
    uint32_t steps{};
public:
    ClockPager(uint32_t & f) : FifoPager(f) {}
    virtual uint32_t select_victim_frame() override;
    virtual void print_info() const override;
};

class WorkingSetPager : public Pager
{
public:
    virtual uint32_t select_victim_frame() override;
    virtual void print_info() const override;
};

#endif