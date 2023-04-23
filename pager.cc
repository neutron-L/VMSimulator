#include "pager.hh"

#include <string>
using std::ifstream;
using std::string;
using std::vector;

/* FIFO */ 
uint32_t FifoPager::select_victim_frame()
{
    auto idx = hand;
    hand = (hand + 1) % frames;
    return idx;
}

void FifoPager::print_info() const
{
    printf(" ASELECT: %lu\n", hand);
}

/* Random */ 
RandomPager::RandomPager(uint32_t & f, ifstream & fin) : frames(f)
{
    uint32_t n;
    fin >> n;
    random_numbers.resize(n);
    for (auto & num : random_numbers)
        fin >> num;
}

uint32_t RandomPager::select_victim_frame()
{
    uint32_t idx = ofs;
    ofs = (ofs + 1) % random_numbers.size();
    return random_numbers[idx] % frames;
}



/* Aging */
uint32_t AgingPager::select_victim_frame()
{
    return 0;
}


/* ESCPager */
uint32_t NRUPager::select_victim_frame()
{
    return 0;
}


/* ClockPager */
uint32_t ClockPager::select_victim_frame()
{
    steps = 0;
    return 0;
}


void ClockPager::print_info() const
{
    printf(" ASELECT: %lu %lu\n", hand, steps);
}


/* WorkingSetPager */
uint32_t WorkingSetPager::select_victim_frame()
{
    return 0;
}