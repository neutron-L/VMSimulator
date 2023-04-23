#include "pager.hh"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
using std::cout;
using std::endl;
using std::ifstream;
using std::string;
using std::vector;
using std::to_string;

extern uint32_t inst_count;

static string to_hex(uint32_t num)
{
    std::ostringstream ss;
    ss << "0x" << std::setfill('0') << std::setw(8) << std::hex << num;
    return ss.str();
}

/* FIFO */ 
uint32_t FifoPager::select_victim_frame()
{
    auto idx = hand;
    hand = (hand + 1) % frames;
    return idx;
}

void FifoPager::print_info()
{
    printf(" ASELECT: %u", hand);
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
    update_frame_age();


    uint32_t cnt = frames - 1;

    msg = " ASELECT " + to_string(hand) + "-" + to_string((hand - 1 + frames) % frames) + " | ";
    msg += to_string(hand) + ":" + to_hex(get_frame_age(hand)) + " ";
    victim = hand;
    while (cnt--)
    {
        hand = (hand + 1) % frames;
        msg += to_string(hand) + ":" + to_hex(get_frame_age(hand)) + " ";
        if (get_frame_age(hand) < get_frame_age(victim))
            victim = hand;
    }
    hand = (victim + 1) % frames;
    msg += "| " + to_string(victim);
    
    return victim;
}

void AgingPager::print_info()
{
    cout << msg;
    msg = "";
}

/* ESCPager */
uint32_t NRUPager::select_victim_frame()
{
    steps = 0;
    pre_hand = hand;
    lowest_class = 4;

    if (inst_count == 49 || inst_count - pre_inst >= 50)
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


void NRUPager::print_info()
{
    printf(" ASELECT: %u %u | %u %u %u", pre_hand, reset, lowest_class, victim, steps);
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


void ClockPager::print_info()
{
    printf(" ASELECT: %u %u", hand, steps);
}


/* WorkingSetPager */
uint32_t WorkingSetPager::select_victim_frame()
{
    uint32_t start = hand;
    victim = hand;

    while (true)
    {
        // check refer bit
        if (get_rbit(hand))
        {
            set_frame_timelastuse(hand, inst_count);
            reset_rbit(hand);
        }
        else
        {
            if (inst_count - get_frame_timelastuse(hand) >= 50)
            {
                victim = hand;
                break;
            }
            else if (get_frame_timelastuse(hand) < get_frame_timelastuse(victim))
                victim = hand;
        }
        hand = (hand + 1) % frames;
        // break
        if (hand == start)
            break;
    }
    hand = (victim + 1) % frames;

    return victim;
}

void WorkingSetPager::print_info()
{
}