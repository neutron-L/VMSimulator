#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <deque>
#include <cassert>
#include <unistd.h>

#include "pager.hh"
#include <cstring>
using namespace std;


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
    uint32_t vpage{};
    uint32_t age{};
}; // freme type <proc_id:vpage>

/* static functions */
static void parse_args(int argc, char **argv);
static int get_frame();
static void get_next_line(ifstream &, istringstream &);
static bool get_next_instruction(ifstream &, istringstream &, char &op, uint32_t &);
static void print_page_table();
static void print_frame_table();
static void print_process_statistics();
// static void update_frame_age(uint32_t vpage);

frame_t frame_table[MAX_FRAMES];
deque<uint32_t> free_frame_pool;

/* global variables */
// statistics
unsigned long inst_count, ctx_switches, process_exits;
unsigned long long cost;

class Process
{
    friend bool page_fault_exception_handler(Process &proc, uint32_t vpage);

protected:
    struct VMA
    {
        uint32_t starting_vpage{};
        uint32_t ending_vpage{};
        uint32_t write_prot{};
        uint32_t filemapped{};

        VMA(uint32_t s, uint32_t e, uint32_t w, uint32_t f) : starting_vpage(s),
                                                              ending_vpage(e), write_prot(w), filemapped(f)
        {
        }
    };

    struct Status
    {
        uint64_t unmaps;
        uint64_t maps;
        uint64_t ins;
        uint64_t outs;
        uint64_t fins;
        uint64_t fouts;
        uint64_t zeros;
        uint64_t segv;
        uint64_t segprot;
    };

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

private:
    uint32_t pid;
    pte_t page_table[MAX_VPAGES]{};
    vector<VMA> vmas{};
    Status pstats{};
    string step_msg; // if O_flag is set, output it after each instruction is executed

public:
    Process(uint32_t id) : pid(id) {}

    void do_access(uint32_t vpage, bool is_write)
    {
        /*
        1. The page exists in memory
        2. a page fault exception was raised and successfully handled
        */
        pte_t &pte = page_table[vpage];
        if (pte.present || page_fault_exception_handler(*this, vpage))
        {
            pte.refer = 1;
            if (is_write)
            {
                if (pte.write_prot)
                {
                    ++pstats.segprot;
                    cost += 410;
                    step_msg += " SEGPROT\n";
                }
                else
                    pte.modified = 1;
            }
        }
        // else, go to next instruction
    }

    void add_vma(uint32_t s, uint32_t e, uint32_t w, uint32_t f)
    {
        vmas.emplace_back(VMA(s, e, w, f));
    }

    void print_page_table() const
    {
        for (uint32_t i = 0; i < MAX_VPAGES; ++i)
        {

            auto &page = page_table[i];
            cout << " ";
            if (page.present)
            {
                cout << i << ":";
                cout << (page.refer ? 'R' : '-');
                cout << (page.modified ? 'M' : '-');
                cout << (page.pagedout && !page.fmapped ? 'S' : '-');
            }
            else
                cout << (page.pagedout && !page.fmapped ? '#' : '*');
        }
    }

    void print_statistics()
    {
        printf(" U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n",
               pstats.unmaps, pstats.maps, pstats.ins, pstats.outs,
               pstats.fins, pstats.fouts, pstats.zeros, pstats.segv, pstats.segprot);
    }

    // for pager algorithm
    uint32_t get_rbit(uint32_t vpage) const
    {
        return page_table[vpage].refer;
    }

    uint32_t get_mbit(uint32_t vpage) const
    {
        return page_table[vpage].modified;
    }

    void reset_rbit(uint32_t vpage)
    {
        page_table[vpage].refer = 0;
    }

    uint32_t translate(uint32_t vpage) const
    {
        return page_table[vpage].frame_num;
    }

    void exit_proc()
    {
        step_msg += "EXIT current process " + to_string(pid) + "\n";
        for (uint32_t i = 0; i < MAX_VPAGES; ++i)
        {
            pte_t & pte = page_table[i];
            if (pte.present)
            {
                step_msg += " UNMAP " + to_string(pid) + ":" + to_string(i) + "\n";
                ++pstats.unmaps;
                cost += 410;

                if (pte.modified && pte.fmapped)
                {
                    ++pstats.fouts;
                    cost += 2800;
                    step_msg += " FOUT\n";
                }
                frame_table[pte.frame_num].proc_id = -1; // clear
                free_frame_pool.push_back(pte.frame_num);
            }
        }
        // clear page table
        memset(page_table, 0, sizeof(page_table));
    }

    void print_step_msg()
    {
        cout << step_msg;
    }

    void clear_step_msg()
    {
        step_msg = "";
    }
}; // process type

/* Function */
bool page_fault_exception_handler(Process &proc, uint32_t vpage);
void context_switch(uint32_t procid);

/* For Pager use */
uint32_t get_rbit(uint32_t idx);
uint32_t get_mbit(uint32_t idx);
void reset_rbit(uint32_t idx);

/* For pager to access the reference of */
// input variable: number of frames and processes
uint32_t frames;
uint32_t num_of_tasks;

vector<Process> proc_vec;
Process *cur_proc;
uint32_t cur_procid;
Pager *pager;

// flag
bool O_flag, P_flag, F_flag, S_flag, x_flag, y_flag, f_flag, a_flag;
string infile, rfile;

int main(int argc, char **argv)
{
    // 1. parse arguments
    parse_args(argc, argv);

    // 2. initialize the free frame pool, frame table and pager
    for (uint32_t i = 0; i < frames; ++i)
        free_frame_pool.push_back(i);
    for (auto &frame : frame_table)
        frame.proc_id = -1;

    // 3. read process info
    ifstream fs(infile);
    if (!fs.is_open())
    {
        cerr << "Fail to open file " << infile << endl;
        exit(1);
    }
    istringstream sin;
    string line;
    int num_of_vmas;

    get_next_line(fs, sin);
    sin >> num_of_tasks; // read number of processes
    proc_vec.reserve(num_of_tasks);

    // read process
    uint32_t s, e, w, f;
    for (uint32_t i = 0; i < num_of_tasks; ++i)
    {
        Process proc(i);
        get_next_line(fs, sin);
        sin >> num_of_vmas;
        // read process vmas
        while (num_of_vmas--)
        {
            get_next_line(fs, sin);
            sin >> s;
            sin >> e;
            sin >> w;
            sin >> f;
            proc.add_vma(s, e, w, f);
        }
        proc_vec.push_back(proc);
    }

    // 4. read instruction
    char op;
    uint32_t num;
    while (get_next_instruction(fs, sin, op, num))
    {
        switch (op)
        {
        case 'c':
            context_switch(num);
            cost += 130;
            ++ctx_switches;
            break;
        case 'r':
            cur_proc->do_access(num, false);
            ++cost;
            break;
        case 'w':
            cur_proc->do_access(num, true);
            ++cost;
            break;
        case 'e':
            proc_vec[num].exit_proc();
            cost += 1230;
            ++process_exits;
            break;
        default:
        {
            cerr << "Invalid operation " << op << endl;
            break;
        }
        }
        if (O_flag)
        {
            printf("%ld: ==> %c %d\n", inst_count, op, num);
            cur_proc->print_step_msg();
        }
        if (x_flag)
            cur_proc->print_page_table();
        if (y_flag)
            print_page_table();
        if (f_flag)
            print_frame_table();
        if (a_flag)
            pager->print_info();
        ++inst_count;
        cur_proc->clear_step_msg();
    }
    if (P_flag)
        print_page_table();
    if (F_flag)
        print_frame_table();
    if (S_flag)
        print_process_statistics();
    fs.close();
    delete pager;

    return 0;
}

bool page_fault_exception_handler(Process &proc, uint32_t vpage)
{
    int vma_idx = proc.vpage_vma(vpage);
    if (vma_idx == -1)
    {
        ++proc.pstats.segv;
        cost += 440;
        proc.step_msg += " SEGV\n";
        return false;
    }

    // get a new frame
    uint32_t idx = get_frame();
    // unmap frame
    auto pid = frame_table[idx].proc_id;
    if (pid != -1)
    {
        auto &victim_proc = proc_vec[pid];
        proc.step_msg += " UNMAP " + to_string(pid) + ":" + to_string(frame_table[idx].vpage) + "\n";
        ++victim_proc.pstats.unmaps;
        cost += 410;
        pte_t &unmap_pte = victim_proc.page_table[frame_table[idx].vpage];
        if (unmap_pte.modified) // whether need to swap out
        {
            if (unmap_pte.fmapped)
            {
                ++victim_proc.pstats.fouts;
                cost += 2800;
                proc.step_msg += " FOUT\n";
            }
            else
            {
                ++victim_proc.pstats.outs;
                cost += 2750;
                proc.step_msg += " OUT\n";
            }
            unmap_pte.pagedout = 1; // Flag that the page has been swapped out
            unmap_pte.modified = 0;
        }
        unmap_pte.present = 0; // reset present
    }

    pte_t &pte = proc.page_table[vpage];

    // set present
    pte.present = 1;
    // set the file mapped and write protection bit
    pte.fmapped = proc.vmas[vma_idx].filemapped;
    pte.write_prot = proc.vmas[vma_idx].write_prot;

    // map
    pte.frame_num = idx;

    // reverse map
    frame_table[idx].proc_id = cur_procid;
    frame_table[idx].vpage = vpage;

    if (pte.fmapped)
    {
        ++proc.pstats.fins;
        cost += 2350;
        proc.step_msg += " FIN\n";
    }
    else if (pte.pagedout)
    {
        ++proc.pstats.ins;
        cost += 3200;
        proc.step_msg += " IN\n";
    }
    else
    {
        ++proc.pstats.zeros;
        cost += 150;
        proc.step_msg += " ZERO\n";
    }
    proc.step_msg += " MAP " + to_string(idx) + "\n";
    cost += 350;
    ++proc.pstats.maps;

    return true;
}

void context_switch(uint32_t procid)
{
    assert(procid < num_of_tasks);
    cur_proc = &proc_vec[procid];
    cur_procid = procid;
}

static void parse_args(int argc, char **argv)
{
    int o;
    const char *optstring = "f:a:o:";
    char algo = ' ';
    while ((o = getopt(argc, argv, optstring)) != -1)
    {
        switch (o)
        {
        case 'f':
            frames = stoi(optarg);
            // cout << "frames: " << frames << endl;
            break;
        case 'a':
            algo = optarg[0];
            // cout << "algo: " << algo << endl;
            break;
        case 'o':
        {
            // cout << "O = " << optarg << endl;
            O_flag = strchr(optarg, 'O') != NULL;
            P_flag = strchr(optarg, 'P') != NULL;
            F_flag = strchr(optarg, 'F') != NULL;
            S_flag = strchr(optarg, 'S') != NULL;
            x_flag = strchr(optarg, 'x') != NULL;
            y_flag = strchr(optarg, 'y') != NULL;
            f_flag = strchr(optarg, 'f') != NULL;
            a_flag = strchr(optarg, 'a') != NULL;
        }
        break;
        default:
        {
            cerr << "invalid option " << o << endl;
            exit(1);
        }
        }
    }

    infile = argv[argc - 2];
    rfile = argv[argc - 1];
    // cout << infile << ' ' << rfile << endl;

    switch (algo)
    {
    case 'a':
        // pager = new AgingPager();
        break;
    case 'c':
        pager = new ClockPager(frames);
        break;
    case 'e':
        pager = new NRUPager(frames);
        break;
    case 'f':
        pager = new FifoPager(frames);
        break;
    case 'r':
    {
        ifstream fin(rfile);
        if (!fin.is_open())
        {
            cerr << "Fail to open " << rfile << endl;
            exit(1);
        }
        pager = new RandomPager(frames, fin);
        fin.close();
    }
    break;
    case 'w':
        // pager = new WorkingSetPager();
        break;

    default:
        break;
    }
}

/* Clock Pager */
uint32_t get_rbit(uint32_t idx)
{
    int id = frame_table[idx].proc_id;
    assert(id != -1);
    return proc_vec[id].get_rbit(frame_table[idx].vpage);
}

void reset_rbit(uint32_t idx)
{
    proc_vec[frame_table[idx].proc_id].reset_rbit(frame_table[idx].vpage);
}

/* NRU Pager */
uint32_t get_mbit(uint32_t idx)
{
    int id = frame_table[idx].proc_id;
    assert(id != -1);
    return proc_vec[id].get_mbit(frame_table[idx].vpage);
}

static int get_frame()
{
    int fidx = -1;

    if (!free_frame_pool.empty())
    {
        fidx = free_frame_pool.front();
        free_frame_pool.pop_front();
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
    for (uint32_t i = 0; i < num_of_tasks; ++i)
    {
        printf("PT[%d]:", i);
        proc_vec[i].print_page_table();
        cout << endl;
    }
}

static void print_frame_table()
{
    cout << "FT:";
    for (uint32_t i = 0; i < frames; ++i)
    {
        auto &frame = frame_table[i];
        if (frame.proc_id == -1)
            cout << " *";
        else
            cout << " " << frame.proc_id << ":" << frame.vpage;
    }
    cout << endl;
}

static void print_process_statistics()
{
    for (uint32_t i = 0; i < num_of_tasks; ++i)
    {
        printf("PROC[%d]:", i);
        proc_vec[i].print_statistics();
    }
    printf("TOTALCOST %lu %lu %lu %llu %lu\n",
           inst_count, ctx_switches, process_exits, cost, sizeof(pte_t));
}


// static void update_frame_age(uint32_t vpage)
// {
//     for (uint32_t i = 0; i < frames; ++i)
//         frame_table[i].age >>= 1;
//     frame_table[cur_proc->translate(vpage)].age |= 0x80000000;
// }