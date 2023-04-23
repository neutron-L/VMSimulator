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
void update_frame_age();
uint32_t get_frame_age(uint32_t);

uint32_t get_frame_timelastuse(uint32_t);
void set_frame_timelastuse(uint32_t, uint32_t);

/* Abstract Pager Class */
class Pager
{
protected:
    uint32_t frames{};
public:
    Pager()=default;
    Pager(uint32_t & f) : frames(f) {}
    virtual uint32_t select_victim_frame() = 0;
    virtual void print_info() = 0;

    virtual ~Pager()=default;
};

class FifoPager : public Pager
{
protected:
    uint32_t hand{};
public:
    FifoPager(uint32_t & f) : Pager(f) {}
    virtual uint32_t select_victim_frame() override;
    virtual void print_info() override;
};

class RandomPager : public Pager
{
private:
    uint32_t ofs{};
    std::vector<uint64_t> random_numbers;
public:
    RandomPager(uint32_t & f, std::ifstream & fin);
    virtual uint32_t select_victim_frame() override;
    virtual void print_info() override
    {}
};

class AgingPager : public Pager
{
private:
    uint32_t hand{};
    uint32_t victim{};
    std::string msg{};
public:
    AgingPager(uint32_t & f) : Pager(f) {}
    virtual uint32_t select_victim_frame() override;
    virtual void print_info() override;
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
    virtual void print_info() override;
};

class ClockPager : public FifoPager
{
private:
    uint32_t steps{};
public:
    ClockPager(uint32_t & f) : FifoPager(f) {}
    virtual uint32_t select_victim_frame() override;
    virtual void print_info() override;
};

class WorkingSetPager : public Pager
{
private:
    uint32_t hand{};
    uint32_t victim{};
public:
    WorkingSetPager(uint32_t & f) : Pager(f) {}
    virtual uint32_t select_victim_frame() override;
    virtual void print_info() override;
};

#endif