#include <iostream>
#include <vector>
using namespace std;

/* Constant */
const int MAX_FRAMES = 128;
const int MAX_VPAGES = 64;

/* Class definition */ 
using pte_t = struct 
{
    uint32_t valid : 1;
    uint32_t refer : 1;
    uint32_t modified : 1;
    uint32_t write_prot : 1;
    uint32_t pagedout : 1;
    uint32_t fmapped : 1;
    uint32_t frame_num : 7;
}; // pte type

using frame_t = struct 
{
    int proc_id;
    int vpage;
}; // freme type

class Process
{
public:
    pte_t page_table[MAX_VPAGES];
}; // process type

class Pager
{
public:
    virtual int select_victim_frame() = 0;
};

/* global variables */
int frames;
int tasks;

frame_t frame_table[MAX_FRAMES];
vector<Process> processes;

/* static functions */
static int get_frame();
static bool get_next_line(string &);
static bool get_next_instruction(char & operation, uint32_t & vpage);

Process * current_process;

int main(int argc, char ** args)
{
    // 1. parse arguments

    // 2. read process info

    // 3. read instruction
    char op;
    uint32_t vpage;
    while (get_next_instruction(op, vpage))
    {

    }

    return 0;
}