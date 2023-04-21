#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <cassert>

#include "pager.hh"
using namespace std;

/* Constant */
extern const int MAX_FRAMES = 128;
extern const int MAX_VPAGES = 64;

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

class Process
{
protected:
    struct VMA
    {
        uint32_t starting_vpage;
        uint32_t ending_vpage;
        uint32_t write_prot;
        uint32_t filemapped;
    };

public:
    pte_t page_table[MAX_VPAGES];
    vector<VMA> vmas;

    // Check if the virtual page is located in a valid VMA,
    // if it is in a valid vma, return the vma`s index, else return -1
    int vpage_vma(const uint32_t &vpage) const
    {
        auto num_of_vmas = vmas.size();
        for (uint32_t i = 0; i < num_of_vmas; ++i)
        {
            const auto &vma = vmas[i];
            if (vpage >= vma.starting_vpage && vpage <= vma.ending_vpage)
                return i;
        }

        return -1;
    }

    void print_page_table() const
    {
        for (uint32_t i = 0; i < MAX_VPAGES; ++i)
        {
            cout << ' ' << i << ":";
            auto & page = page_table[i];
            if (page.present)
            {
                cout << (page.refer ? 'R' : '-');
                cout << (page.modified ? 'M' : '-');
                cout << (page.pagedout ? 'S' : '-');
            }
            else
                cout << (page.pagedout ? '#' : '*');
        }
    }
}; // process type




/* static functions */
static int get_frame();
static void get_next_line(ifstream &, istringstream &);
static bool get_next_instruction(ifstream &, istringstream &, char &op, uint32_t &);
static void print_page_table();
static void print_frame_table();

/* Function */
bool page_fault_exception_handler(uint32_t vpage);
void context_switch(uint32_t procid);
void do_access(uint32_t vpage, bool is_write);
void exit_proc(uint32_t procid);

/* global variables */
uint32_t frames = 16;
uint32_t num_of_tasks;

frame_t frame_table[MAX_FRAMES];
vector<Process> proc_vec;
Process *cur_proc;
uint32_t cur_procid;
Pager *pager;
set<uint32_t> free_frame_pool;

int main(int argc, char **args)
{
    // 0. initialize the free frame pool, frame table and pager
    for (uint32_t i = 0; i < frames; ++i)
        free_frame_pool.insert(i);
    for (auto & frame : frame_table)
        frame.proc_id = -1;
    pager = new FifoPager(frames);
    // 1. parse arguments

    // 2. read process info
    ifstream fs(args[1]);
    if (!fs.is_open())
    {
        cerr << "Fail to open file " << args[1] << endl;
        exit(1);
    }
    istringstream sin;
    string line;
    int num_of_vmas;

    get_next_line(fs, sin);
    sin >> num_of_tasks; // read number of processes
    proc_vec.resize(num_of_tasks);

    // read process
    for (auto &proc : proc_vec)
    {
        get_next_line(fs, sin);
        sin >> num_of_vmas;
        proc.vmas.resize(num_of_vmas);
        // read process vmas
        for (auto &vma : proc.vmas)
        {
            get_next_line(fs, sin);
            sin >> vma.starting_vpage;
            sin >> vma.ending_vpage;
            sin >> vma.write_prot;
            sin >> vma.filemapped;
        }
    }

    // 3. read instruction
    char op;
    uint32_t num;
    uint32_t idx = 0;
    while (get_next_instruction(fs, sin, op, num))
    {
        printf("%d: ==> %c %d\n", idx, op, num);
        ++idx;
        switch (op)
        {
        case 'c':
            context_switch(num);
            break;
        case 'r':
            do_access(num, false);
            break;
        case 'w':
            do_access(num, true);
            break;
        case 'e':
            exit_proc(num);
            break;
        default:
        {
            cerr << "Invalid operation " << op << endl;
            break;
        }
        }
    }

    fs.close();

    return 0;
}

bool page_fault_exception_handler(uint32_t vpage)
{
    int vma_idx = cur_proc->vpage_vma(vpage);
    if (vma_idx == -1)
    {
        cout << " SEGV\n";
        return false;
    }

    // get a new frame
    uint32_t idx = get_frame();

    // unmap frame
    auto pid = frame_table[idx].proc_id;
    if (pid != -1)
    {
        printf(" UNMAP %d:%d\n", pid, frame_table[idx].vpage);
        pte_t &unmap_pte = proc_vec[pid].page_table[frame_table[idx].vpage];
        if (unmap_pte.modified) // whether need to swap out
        {
            if (unmap_pte.fmapped)
                cout << " FOUT" << endl;
            else
                cout << " OUT" << endl;
            unmap_pte.pagedout = 1; // Flag that the page has been swapped out
            unmap_pte.modified = 0;
        }
        unmap_pte.present = 0; // reset present
    }

    pte_t &pte = cur_proc->page_table[vpage];

    // set present
    pte.present = 1; 
    // set the file mapped and write protection bit
    pte.fmapped = cur_proc->vmas[vma_idx].filemapped;
    pte.write_prot = cur_proc->vmas[vma_idx].write_prot;
    

    // reverse map
    frame_table[idx].proc_id = cur_procid;
    frame_table[idx].vpage = vpage;

    if (pte.fmapped)
        cout << " FIN" << endl;
    else if (pte.pagedout)
        cout << " IN" << endl;
    else
        cout << " ZERO" << endl;
    cout << " MAP " << idx << endl;

    return true;
}

void context_switch(uint32_t procid)
{
    assert(procid < num_of_tasks);
    cur_proc = &proc_vec[procid];
    cur_procid = procid;
}

void do_access(uint32_t vpage, bool is_write)
{
    /*
    1. The page exists in memory
    2. a page fault exception was raised and successfully handled
    */
    pte_t &pte = cur_proc->page_table[vpage];
    if (pte.present || page_fault_exception_handler(vpage))
    {
        pte.refer = 1;
        if (is_write)
        {
            if (pte.write_prot)
                cout << " SEGPROT" << endl;
            else
                pte.modified = 1;
        }
    }
    // else, go to next instruction
}

void exit_proc(uint32_t procid)
{
}

static int get_frame()
{
    int fidx = -1;

    if (!free_frame_pool.empty())
    {
        fidx = *free_frame_pool.begin();
        free_frame_pool.erase(fidx);
    }
    else
        fidx = pager->select_victim_frame();
    return fidx;
}

static void get_next_line(ifstream &fs, istringstream &sin)
{
    static string line;
    while (getline(fs, line))
    {
        if (line[0] == '#')
            continue;
        sin.clear();
        sin.str(line);
        assert(sin.str() == line);

        return;
    }
    cerr << "Fail to read line\n";
    exit(1);
}

static bool get_next_instruction(ifstream &fs, istringstream &sin, char &operation, uint32_t &num)
{
    static string line;
    while (getline(fs, line))
    {
        if (line[0] == '#')
            continue;
        sin.clear();
        sin.str(line);
        assert(sin.str() == line);
        sin >> operation;
        sin >> num;

        return true;
    }
    return false;
}

static void print_page_table()
{
}

static void print_frame_table()
{
    cout << "FT:";
    for (auto &frame : frame_table)
    {
        if (frame.proc_id == -1)
            cout << " *";
        else
            cout << " " << frame.proc_id << ":" << frame.vpage;
    }
    cout << endl;
}