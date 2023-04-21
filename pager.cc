#include "pager.hh"

uint32_t FifoPager::select_victim_frame()
{
    auto idx = hand;
    hand = (hand + 1) % frames;
    return idx;
}