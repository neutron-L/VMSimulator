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
    printf(" ASELECT: %u\n", hand);
}

/* Random */ 
RandomPager::RandomPager(uint32_t & f, ifstream & fin) : Pager(f)
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

void AgingPager::print_info() const
{
}

/* ESCPager */
uint32_t NRUPager::select_victim_frame()
{
    return 0;
}


void NRUPager::print_info() const
{
}


/* ClockPager */
uint32_t ClockPager::select_victim_frame()
{
    steps = 0;
    while (get_rbit(hand))
    {
        reset_rbit(hand);
        hand = (hand + 1) % frames;
        ++steps;
    }
    uint32_t idx = hand;
    hand = (hand + 1) % frames;

    return idx;
}


void ClockPager::print_info() const
{
    printf(" ASELECT: %u %u\n", hand, steps);
}


/* WorkingSetPager */
uint32_t WorkingSetPager::select_victim_frame()
{
    return 0;
}

void WorkingSetPager::print_info() const
{
}