#include "pager.hh"

#include <string>
using std::ifstream;
using std::string;
using std::vector;

uint32_t FifoPager::select_victim_frame()
{
    auto idx = hand;
    hand = (hand + 1) % frames;
    return idx;
}

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