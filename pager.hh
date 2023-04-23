#ifndef PAGER_HH
#define PAGER_HH

#include <fstream>
#include <cstdint>
#include <vector>

#include "pager.hh"


/* Constant */
const int MAX_FRAMES = 128;
const int MAX_VPAGES = 64;

/* Class definition */
using pte_t = struct
{
    uint32_t present : 1;
    uint32_t refer : 1;
    uint32_t modified : 1;
    uint32_t write_prot : 1;
    uint32_t pagedout : 1;
    uint32_t fmapped : 1;
    uint32_t frame_num : 7;
}; // pte type

using frame_t = struct
{
    int proc_id; // if proc_id is -1, means it has not been mapped to a vpage
    uint32_t vpage;
}; // freme type <proc_id:vpage>


/* Abstract Pager Class */
class Pager
{
public:
    virtual uint32_t select_victim_frame() = 0;
    virtual void print_info() const = 0;
};

class FifoPager : public Pager
{
protected:
    uint32_t hand{};
    uint32_t frames{};
public:
    FifoPager(uint32_t & f) : frames(f) {}
    virtual uint32_t select_victim_frame() override;
    virtual void print_info() const override;
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
    virtual void print_info() const override;
};

class AgingPager : public Pager
{
private:
    frame_t * frame_table;
public:
    virtual uint32_t select_victim_frame() override;
    virtual void print_info() const override;
};


class NRUPager : public Pager
{
public:
    virtual uint32_t select_victim_frame() override;
    virtual void print_info() const override;
};

class ClockPager : public FifoPager
{
private:
    uint32_t steps{};
public:
    ClockPager()=default;
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