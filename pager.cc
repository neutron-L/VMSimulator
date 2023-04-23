#include "pager.hh"
#include <iostream>
#include <vector>
#include <string>
using std::cout;
using std::endl;
using std::ifstream;
using std::string;
using std::vector;

extern uint32_t inst_count;

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
    // cout << random_numbers[idx] % frames << endl; 
    return random_numbers[idx] % frames;
}

/* Aging */
uint32_t AgingPager::select_victim_frame()
{
    uint32_t victim;
    for (uint32_t i = 0; i < frames; ++i)

    return 0;
}

void AgingPager::print_info() const
{
}

/* ESCPager */
uint32_t NRUPager::select_victim_frame()
{
    steps = 0;
    pre_hand = hand;
    lowest_class = 4;

    if (inst_count - pre_inst >= 50)
    {
        reset = 1;
        pre_inst = inst_count;
    }
    else
        reset = 0;

    while (true)
    {
         // get class of the frame
        uint32_t c = (get_rbit(hand) << 1) | (get_mbit(hand));

        // update victim
        if (lowest_class > c)
        {
            lowest_class = c;
            victim = hand;
        }
        if (reset)
            reset_rbit(hand);
        hand = (hand + 1) % frames;
        ++steps;
        // break
        if (((lowest_class == 0 || hand == pre_hand) && !reset) ||  (hand == pre_hand && reset))
            break;
    }
    hand = (victim + 1) % frames;

    return victim;
}


void NRUPager::print_info() const
{
    printf(" ASELECT: %u %u | %u %u %u\n", pre_hand, reset, lowest_class, victim, steps);
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