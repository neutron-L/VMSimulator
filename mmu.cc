#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cassert>
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
    uint32_t vpage;
}; // freme type

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

    void print_page_table() const
    {
        for (uint32_t i = 0; i < MAX_VPAGES; ++i)
        {
            cout << ' ' << i << ":";
            auto & page = page_table[i];
            if (page.valid)
            {
                cout << (page.refer ? 'R' : '-');
                cout << (page.modified ? 'R' : '-');
                cout << (page.pagedout ? 'R' : '-');
            }
            else
                cout << (page.pagedout ? '#' : '*');
        }
    }
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
vector<Process> proc_vec;

/* static functions */
static int get_frame();
static void get_next_line(ifstream &, istringstream &);
static bool get_next_instruction(ifstream &, istringstream &, char & operation, uint32_t & vpage);
static void print_page_table();
static void print_frame_table();

Process * current_process;

int main(int argc, char ** args)
{
    // 1. parse arguments

    // 2. read process info
    ifstream fs(args[1]);
    if (!fs.is_open())
    {
        cerr << "Fail to open file " << args[1] << endl;
        return 1;
    }
    istringstream sin;
    string line;
    int num_of_tasks, num_of_vmas;

    get_next_line(fs, sin);
    sin >> num_of_tasks;               // read number of processes
    proc_vec.resize(num_of_tasks);

    // read process 
    for (auto & proc : proc_vec)
    {
        get_next_line(fs, sin);
        sin >> num_of_vmas;
        proc.vmas.resize(num_of_vmas);
        // read process vmas
        for (auto & vma : proc.vmas)
        {
            get_next_line(fs, sin);
            sin >> vma.starting_vpage;
            sin >> vma.ending_vpage;
            sin >> vma.write_prot;
            sin >> vma.filemapped;
        }
    }

    // Check
    uint i = 0;
    cout << proc_vec.size() << endl;
    for (auto & proc : proc_vec)
    {
        cout << "#### process " << i++ << endl;
        cout << "#\n";
        cout << proc.vmas.size() << endl;
        // read process vmas
        for (auto & vma : proc.vmas)
            cout << vma.starting_vpage << ' ' << vma.ending_vpage << ' ' << vma.write_prot << ' ' << vma.filemapped << endl;
    }

    // 3. read instruction
    char op;
    uint32_t vpage;
    cout << "#### instruction simulation ######\n";
    while (get_next_instruction(fs, sin, op, vpage))
    {
        cout << op << ' ' << vpage << endl;
    }

    fs.close();

    return 0;
}

static int get_frame()
{
    return 0;
}

static void get_next_line(ifstream & fs, istringstream & sin)
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

static bool get_next_instruction(ifstream & fs, istringstream & sin, char & operation, uint32_t & vpage)
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
        sin >> vpage;

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
    for (auto & frame : frame_table)
    {
        if (frame.proc_id == -1)
            cout << " *";
        else
            cout << " " << frame.proc_id << ":" << frame.vpage;
    }
    cout << endl;
}