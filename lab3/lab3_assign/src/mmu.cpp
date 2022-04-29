#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <deque>
#include <string>
#include <string.h>
#include <fstream>
#include <algorithm>
#include <queue>
#include <stack>
#include <sstream>
#include <tuple>
#include <map>
#include <stdarg.h>
#include <climits>


using namespace std;

class Process;
class Pager;
int GetRandom(int burst);



enum ALGO {FIFO=0, RANDOM, CLOCK, ESCNRU, AGING, WORKINGSET};
enum INSTRUCTION {CONTEXT, READ, WRITE, EXIT};
enum COST_MAP_KEY {KMAP, KUNMAP, KIN, KOUT, KFIN, KFOUT, KZERO, KSEGV, KSEGPROT, KREAD_WRITE, KCONTEXT_SWITCH, KEXIT};

string INPUT_FILE = "";
string RFILE = "";
vector<int> randomValues;
vector<int>::iterator randomValuesIter;
bool oohNohOption = false;
bool pageTableOption = false;
bool frameTableOption = false;
bool statsNSummaryOption = false;
int totalFrames = 0;
ALGO algo = FIFO;
int numberOfProcess = 0;
int inst_count = 0, ctx_switches = 0, process_exits = 0, readWriteInstructions = 0;
map<int,int> costMap = 
{
    {KMAP, 300},
    {KUNMAP, 400},
    {KIN, 3100},
    {KOUT, 2700},
    {KFIN, 2800},
    {KFOUT, 2400},
    {KZERO, 140},
    {KSEGV, 340},
    {KSEGPROT, 420},
    {KREAD_WRITE, 1},
    {KCONTEXT_SWITCH, 130},
    {KEXIT, 1250},
};
const int MAX_FRAMES = 128;
const int MAX_VPAGES = 64;
const int RESET_INSTRUCTION_COUNT = 50;

typedef struct VMA
{

    public:
        int startingVirtualPage;
        int endingVirtualPage;
        bool writeProtected;
        bool fileMapped;

    VMA(int start, int end, bool wp, bool fm)
    {
        startingVirtualPage = start;
        endingVirtualPage = end;
        writeProtected = wp;
        fileMapped = fm;
    }

    public:
        void Print()
        {
            printf("start %d end %d write protected %s file mapped %s\n",
            startingVirtualPage, endingVirtualPage, writeProtected ? "true" : "false", fileMapped ? "true" : "false");
        }
} VMA;

// PTE is comprised of the PRESENT/VALID, REFERENCED,
// MODIFIED, WRITE_PROTECT, and PAGEDOUT bits 
typedef struct pte_t
{
    unsigned int valid:1;
    unsigned int referenced:1;
    unsigned int modified:1;
    unsigned int writeProtected:1;
    unsigned int pagedOut:1;
    unsigned int frameAddress:7;
    unsigned int fileMapped:1;
    unsigned int accessValid:1;
    unsigned int accessValidated:1;

    pte_t()
    {
        valid = 0;
        referenced = 0;
        modified = 0;
        writeProtected = 0;
        pagedOut = 0;
        frameAddress = 0;
        fileMapped = 0;
        accessValid = 0;
        accessValidated = 0;
    }

    public:
        void Clear()
        {
            valid = 0;
            referenced = 0;
            modified = 0;
            // writeProtected = 0;
            // pagedOut = 0;
            frameAddress = 0;
            // fileMapped = 0;
            // accessValid = 0;
            // accessValidated = 0;
        }

} pte_t; // can only be total of 32-bit size and will check on this 

typedef struct frame_t
{
    int pid;
    int vPageNumber;
    int frameAddress;
    unsigned int age = 0;
    unsigned long long int instructionLastAccesed = 0;

    frame_t ()
    {
        pid = -1;
        vPageNumber = -1;
        frameAddress = -1;
    }
} frame_t;

typedef struct Statistics {
    unsigned long long unmaps;
    unsigned long long maps;
    unsigned long long ins;
    unsigned long long outs;
    unsigned long long fins;
    unsigned long long fouts;
    unsigned long long zeros;
    unsigned long long segv;
    unsigned long long segprot;

    Statistics()
    {
        unmaps = 0;
        maps = 0;
        ins = 0;
        outs = 0;
        fins = 0;
        fouts = 0;
        zeros = 0;
        segv = 0;
        segprot = 0;
    }

} Statistics;


frame_t frameTable[MAX_FRAMES];
deque <frame_t *> freeMemoryPool;
Pager* pager;
vector<Process> allProcesses;
Process *currentProcess = NULL;
Process *processToQuit = NULL;
deque< pair<INSTRUCTION, int > > instructions;

class Process
{
    public:
        int pid;
        pte_t pageTable[MAX_VPAGES]; // a per process array of fixed size=64 of pte_t not pte_t pointers
        vector<VMA> vMAs;
        Statistics pstats;

    public:
        bool CheckVPageAccess(int vPageNumber)
        {
            if (pageTable[vPageNumber].accessValidated == 1)
            {
                return pageTable[vPageNumber].accessValid == 1;
            }

            pageTable[vPageNumber].accessValidated = 1;

            for (auto iter = vMAs.begin(); iter != vMAs.end(); iter++)
            {
                if (vPageNumber >= iter->startingVirtualPage && vPageNumber <= iter->endingVirtualPage)
                {
                    pageTable[vPageNumber].fileMapped = iter->fileMapped;
                    pageTable[vPageNumber].writeProtected = iter->writeProtected;
                    pageTable[vPageNumber].valid = 1;
                    pageTable[vPageNumber].accessValid = 1;
                    return true;
                }
            }

            pageTable[vPageNumber].valid = 0;
            // pageTable[vPageNumber].accessValid = 0;
            return false;
        }

        void PrintStats()
        {
            printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n",
                    pid,
                    pstats.unmaps, pstats.maps, pstats.ins, pstats.outs,
                    pstats.fins, pstats.fouts, pstats.zeros,
                    pstats.segv, pstats.segprot);
        }

        // void PrintOhNo(const char* printString, ...)
        // {
        //     if (oohNohOption)
        //     {
        //         va_list args;
        //         va_start(args, printString);
        //         printf(printString, args);
        //         va_end (args);
        //     }
        // }
        void PrintSEGV()
        {
            if (oohNohOption) { printf(" SEGV\n");}
            pstats.segv++;
        }

        void PrintFOUT()
        {
            if (oohNohOption) { printf(" FOUT\n");}
            pstats.fouts++;
        }

        void PrintOUT()
        {
            if (oohNohOption) { printf(" OUT\n");}
            pstats.outs++;
        }

        void PrintMAP(int value)
        {
            if (oohNohOption) { printf(" MAP %d\n", value);}
            pstats.maps++;
        }

        void PrintUNMAP(int pageNumber)
        {
            if (oohNohOption) { printf(" UNMAP %d:%d\n", pid, pageNumber);}
            pstats.unmaps++;
        }

        void PrintSEGPROT()
        {
            if (oohNohOption) { printf(" SEGPROT\n");}
            pstats.segprot++;
        }

        void PrintFIN()
        {
            if (oohNohOption) { printf(" FIN\n");}
            pstats.fins++;
        }

        void PrintIN()
        {
            if (oohNohOption) { printf(" IN\n");}
            pstats.ins++;
        }

        void PrintZERO()
        {
            if (oohNohOption) { printf(" ZERO\n");}
            pstats.zeros++;
        }

};

class Pager
{
    public:
        virtual frame_t* select_victim_frame() = 0; // virtual base class
        virtual void ClearFrameAge(frame_t* frame) {};
        virtual void ClearFrameLastAcessed(frame_t* frame) {};
        virtual int IncrementHand(int hand)
        {
            hand++;
            if (hand == totalFrames)
            {
                hand = 0;
            }
            return hand;
        }
};

class FIFOPager : public Pager
{
    private:
        const ALGO type = FIFO;
        int index = 0;
    
    public:
        frame_t* select_victim_frame()
        {
            frame_t* frame = &frameTable[index];
            index++;
            if(index == totalFrames)
            {
                index = 0;
            }
            return frame;
        }

};

class RandomPager : public Pager
{
    private:
        const ALGO type = RANDOM;
    
    public:
        frame_t* select_victim_frame()
        {
            return &frameTable[GetRandom(totalFrames)];
        }
};

class ClockPager : public Pager
{
    private:
        const ALGO type = CLOCK;
        int index = 0;
    
    public:
        frame_t* select_victim_frame()
        {
            frame_t* frame = NULL;
            bool victimFound = false;
            while(!victimFound)
            {
                frame = &frameTable[index];
                Process *p;

                for (auto iter = allProcesses.begin(); iter != allProcesses.end(); iter++)
                {
                    if (iter->pid == frame->pid)
                    {
                        if (iter->pageTable[frame->vPageNumber].referenced != 1)
                        {
                            victimFound = true;
                        }
                        iter->pageTable[frame->vPageNumber].referenced = 0;
                        break;
                    }
                }

                index++;
                if(index == totalFrames)
                {
                    index = 0;
                }
            }
            return frame; 
        }
};

class ESCNRUPager : public Pager
{
    private:
        const ALGO type = ESCNRU;
        unsigned long long lastReset = 0;
        frame_t *victimFrameClass0, *victimFrameClass1, *victimFrameClass2, *victimFrameClass3;
        int hand = 0, handClass0 = 0, handClass1 = 0, handClass2 = 0, handClass3 = 0;
    
    public:
        
        bool RetrunClass0AfterScanAndResetFrames(bool shouldReset)
        {
            int nofScannedFrames = 0;
            bool beginningOfScan = true;
            int currentHand = -1;
            int endHand = hand;
            bool isReferenced = false, isModified = false;

            victimFrameClass0 = victimFrameClass1 = victimFrameClass2 = victimFrameClass3 = NULL;
            handClass0 = handClass1 = handClass2 = handClass3 = 0;

            while (currentHand != endHand)
            {
                if (currentHand == -1) { currentHand = hand;}
                frame_t* frame = &frameTable[currentHand];
                nofScannedFrames++;

                pte_t* page = NULL;
                for (auto iter = allProcesses.begin(); iter != allProcesses.end(); iter++)
                {
                    if (iter->pid == frame->pid)
                    {
                        page = &(iter->pageTable[frame->vPageNumber]);
                        break;
                    }
                }
                // Check for NULL PageTableEntry???
                isReferenced = page->referenced == 1; 
                isModified = page->modified == 1;
                if(page->valid == 1 && shouldReset )
                {
                    page->referenced = 0;
                }
            
                // Class 0
                if(!isReferenced && !isModified && victimFrameClass0 == NULL)
                {
                    victimFrameClass0 = frame;
                    handClass0 = currentHand;
                    if(!shouldReset)
                    {
                        hand = currentHand;
                        hand = IncrementHand(hand);
                        return true;
                    }
                }

                // Class 1
                if(!isReferenced && isModified && victimFrameClass1 == NULL)
                {
                    victimFrameClass1 = frame;
                    handClass1 = currentHand;
                }

                // Class 2
                if(isReferenced && !isModified && victimFrameClass2 == NULL)
                {
                    victimFrameClass2 = frame;
                    handClass2 = currentHand;
                }

                // Class 3
                if(isReferenced && isModified && victimFrameClass3 == NULL)
                {
                    victimFrameClass3 = frame;
                    handClass3 = currentHand;
                }

                currentHand = IncrementHand(currentHand);
            }
            return false;
        }

        frame_t* select_victim_frame()
        {
            bool shouldReset = false;
            unsigned long long instructionsAfterLastReset = inst_count - lastReset + 1;
            if (instructionsAfterLastReset >= RESET_INSTRUCTION_COUNT)
            {
                shouldReset = true;
                lastReset = inst_count + 1;
            }

            if (RetrunClass0AfterScanAndResetFrames(shouldReset))
            {
                return victimFrameClass0;
            }
            
            if(victimFrameClass0 != NULL)
            {
                hand = handClass0;
                hand = IncrementHand(hand);
                return victimFrameClass0;
            }
            if(victimFrameClass1 != NULL)
            {
                hand = handClass1;
                hand = IncrementHand(hand);
                return victimFrameClass1;
            }
            if(victimFrameClass2 != NULL)
            {
                hand = handClass2;
                hand = IncrementHand(hand);
                return victimFrameClass2;
            }
            if(victimFrameClass3 != NULL)
            {
                hand = handClass3;
                hand = IncrementHand(hand);
                return victimFrameClass3;
            }
            hand = IncrementHand(hand);
            return NULL;
        }
};

class AgingPager : public Pager
{
    private:
        const ALGO type = AGING;
        int hand = 0;
    
    public:
        frame_t* select_victim_frame()
        {
            frame_t* frame = &frameTable[hand];
            int currentHand = -1;
            int minIndex = hand;
            frame_t* minFrame = &frameTable[hand];

            while (currentHand != hand)
            {
                if (currentHand == -1) { currentHand = hand;}
                frame_t* frame = &frameTable[currentHand];
                frame->age = frame->age >> 1;

                for (auto iter = allProcesses.begin(); iter != allProcesses.end(); iter++)
                {
                    if (frame->pid == iter->pid)
                    {
                        if (iter->pageTable[frame->vPageNumber].referenced == 1)
                        {
                            frame->age = frame->age | 0x80000000;
                            iter->pageTable[frame->vPageNumber].referenced = 0;
                        }
                        break;
                    }
                }

                if (frame->age < frameTable[minIndex].age)
                {
                    minIndex = currentHand;
                }

                currentHand = IncrementHand(currentHand);
            }

            hand = minIndex;
            hand = IncrementHand(hand);
            return &frameTable[minIndex];
        }
};

class WorkingSetPager : public Pager
{
    private:
        const ALGO type = WORKINGSET;
        int hand = 0;
    
    public:
        frame_t* select_victim_frame()
        {
            int currentHand = -1;
            frame_t* minFrame = &frameTable[hand];            
            
            int lastFrameIndex = -1;
            int lastFrameIndexRep = -1;
            unsigned long long int oldestFrameValue = INT_MAX;
            unsigned long long int oldestFrameValueR = INT_MAX;

            while (currentHand != hand)
            {
                if (currentHand == -1) { currentHand = hand;}
                frame_t* frame = &frameTable[currentHand];
                
                unsigned long long instructionPassed = inst_count - frame->instructionLastAccesed + 1;
                bool moreThanTau = false;
                if(instructionPassed >= RESET_INSTRUCTION_COUNT)
                {
                    moreThanTau = true;
                }

                pte_t* page = NULL;
                for( auto iter = allProcesses.begin(); iter != allProcesses.end(); iter++)
                {
                    if (iter->pid == frame->pid)
                    {
                        page = &(iter->pageTable[frame->vPageNumber]);
                    }
                }
                
                if(moreThanTau && page->referenced == 0){
                    hand = currentHand;
                    hand = IncrementHand(currentHand);
                    return frame;
                }

                if(page->referenced == 1)
                {
                    frame->instructionLastAccesed = inst_count + 1;
                }

                if(frame->instructionLastAccesed < oldestFrameValue && page->referenced == 0)
                {
                    lastFrameIndex = currentHand;
                    oldestFrameValue = frame->instructionLastAccesed;
                }
                if(frame->instructionLastAccesed < oldestFrameValueR && page->referenced == 1)
                {
                    lastFrameIndexRep = currentHand;
                    oldestFrameValueR = frame->instructionLastAccesed;
                }
                
                page->referenced = 0;
                                
                currentHand = IncrementHand(currentHand);
            }

            if(lastFrameIndex == -1)
            {
                hand = lastFrameIndexRep;
                hand = IncrementHand(hand);
                return &frameTable[lastFrameIndexRep];
            }
            else
            {
                hand = lastFrameIndex;
                hand = IncrementHand(hand);
                return &frameTable[lastFrameIndex];
            }

            return NULL;
        }

        void resetLastAccessedForFrame(frame_t* frame)
        {
            frame->instructionLastAccesed = inst_count + 1;
        }
};

inline std::string trim(const std::string& line)
{
    std::size_t start = line.find_first_not_of(" \t");
    std::size_t end = line.find_last_not_of(" \t");
    return line.substr(start, end - start + 1);
}



void ReadRandomNumbers()
{
    ifstream fin;
    string line;
    int lineNumber = 0;
    int totalLines = 0;
    try
    {
        fin.open(RFILE);
    }
    catch(std::exception const& e)
    {
        cout << "There was an error in opening random number file " << RFILE << ": " << e.what() << endl;
        exit(1);
    }

    while (fin && lineNumber <= totalLines) {
        // Read a line from input file
        getline(fin, line);
        lineNumber++;
        int r;
        try
        {
            r = stoi(line);
        }
        catch(std::exception const& e)
        {
            cout << "There was an error reading number at line number " << lineNumber 
            << " in random number file " << RFILE << ": " << e.what() << endl;
            exit(1);
        }
        if (lineNumber == 1)
        {
            totalLines = r;
        }
        else
        {
            randomValues.push_back(r);
        }
    }

    fin.close();

    if (!randomValues.empty())
    {
        randomValuesIter = randomValues.begin();
    }

}

int GetRandom(int burst)
{
    int nextRandom;
    if (randomValues.empty())
    {
        printf("No random values read from file %s", RFILE.c_str());
        return 0;
    }
    if (randomValuesIter == randomValues.end())
    {
        randomValuesIter = randomValues.begin();
    }
    nextRandom = *randomValuesIter;
    randomValuesIter++;

    return (nextRandom % burst);
}

void ParseCommandLineOptions (int argc, char** argv)
{
    opterr = 0;
    int cmdopt;

    // printf("Number of arguments = %d\n", argc);
    // printf("optind: %d : %s\n", optind, argv[optind-1]);

    // while ((cmdopt = getopt(argc, argv, "vteps:")) != -1)
    while ((cmdopt = getopt(argc, argv, "f:a:oOPFS")) != -1)
    {
        // printf("optind: %d : ", optind);
        switch (cmdopt)
        {
            case 'f':
                {
                    totalFrames = stoi(optarg);
                    // printf("printVerbose = %s\n", printVerbose ? "true" : "false");
                    for (int i = 0; i < totalFrames; i++)
                    {
                        frameTable[i].frameAddress = i;
                        freeMemoryPool.push_back(&frameTable[i]);
                    }
                    break;
                }
            case 'a':
                {
                    char _algo;
                    sscanf(optarg, "%c", &_algo);
                    switch (_algo)
                    {
                        case 'f':
                            algo = FIFO;
                            pager = new FIFOPager();
                            break;
                        case 'r':
                            algo = RANDOM;
                            pager = new RandomPager();
                            break;
                        case 'c':
                            algo = CLOCK;
                            pager = new ClockPager();
                            break;
                        case 'e':
                            algo = ESCNRU;
                            pager = new ESCNRUPager();
                            break;
                        case 'a':
                            algo = AGING;
                            pager = new AgingPager();
                            break;
                        case 'w':
                            algo = WORKINGSET;
                            pager = new WorkingSetPager();
                            break;
                    }
                    break;
                }
            case 'o':
                break;
            case 'O':
                oohNohOption = true;
                break;
            case 'P':
                pageTableOption = true;
                break;
            case 'F':
                frameTableOption = true;
                break;
            case 'S':
                statsNSummaryOption = true;
                break;
            case '?':
                // printf("?: optopt = -%c\n", optopt);
                if (optopt == 'f' || optopt == 'a') { printf("Option -%c requires an argument\n", optopt); }
                break;
            default:
                printf("default: %s\n", optarg);
        }
    }
    if (argc - optind < 0)
    {
        printf("Error: option arguments > total number of arguments (optind > argc)\n");
        exit(1);
    }
    else if (argc - optind < 1)
    {
        printf("Error: The command is missing input file and random file argument\n");
    }
    else if (argc - optind < 2)
    {
        printf("Error: The command is missing random numbers file argument\n");
    }
    else if (argc - optind > 2)
    {
        printf("Error: more than 2 non-option arguments\n");
    }
    else if (argc - optind == 2)
    {
        INPUT_FILE = argv[optind];
        RFILE = argv[optind+1];
    }

    // printf("Input file: %s\n", INPUT_FILE.c_str());
    // printf("Random number file: %s\n", RFILE.c_str());
}

void ReadProcessInput()
{
    ifstream fin;
    string line;
    int lineNumber = 0;
    vector<Process>::iterator proc = allProcesses.begin();
    // printf("Reading process details from %s\n", INPUT_FILE.c_str());

    try
    {
        fin.open(INPUT_FILE);
    }
    catch(std::exception const& e)
    {
        printf("There was an error in opening input file %s: %s\n", INPUT_FILE.c_str(), e.what());
        exit(1);
    }

    while (fin) {
        while(getline(fin, line))
        {
            line = trim(line);
            if(line[0] != '#')
            {
                sscanf(line.c_str(), "%d", &numberOfProcess);
                break;
            }
        }
        // Read process VMA details
        int procId = 0;
        while (procId < numberOfProcess && getline(fin, line))
        {
            line = trim(line);
            if(line[0] == '#') { continue;}
            Process p = Process();
            p.pid = procId;
            int vMACount = 0;
            sscanf(line.c_str(), "%d", &vMACount);

            for (int i = 0; i < vMACount && getline(fin, line); )
            {
                line = trim(line);
                if(line[0] == '#') { continue;}
                // local start, end, write protected, file mapped VMA vaiables
                int lstart = 0, lend = 0, lwp = 0, lfm = 0;
                bool lbwp = false, lbfm = false;
                sscanf(line.c_str(), "%d %d %d %d", &lstart, &lend, &lwp, &lfm);
                lbwp = lwp == 1;
                lbfm = lfm == 1;
                VMA lVMA(lstart, lend, lbwp, lbfm);
                p.vMAs.push_back(lVMA);
                i++;
            }

            allProcesses.push_back(p);
            procId++;
        }

        // Read Instructions
        while (getline(fin, line))
        {
            char ins;
            int val;
            line = trim(line);
            if(line[0] == '#') { continue;}
            sscanf(line.c_str(), "%c %d", &ins, &val);
            switch (ins)
            {
                case 'c':
                    instructions.push_back(make_pair(CONTEXT, val));
                    break;
                case 'e':
                    instructions.push_back(make_pair(EXIT, val));
                    break;
                case 'r':
                    instructions.push_back(make_pair(READ, val));
                    break;
                case 'w':
                    instructions.push_back(make_pair(WRITE, val));
                    break;
            }
        }
    }

    fin.close();
}

frame_t* allocate_frame_from_free_list()
{
    if(freeMemoryPool.empty())
    {
        return NULL;
    }
    else
    {
        frame_t* frame = freeMemoryPool.front();
        freeMemoryPool.pop_front();
        return frame;
    }
}

frame_t* get_frame()
{
    frame_t *frame = allocate_frame_from_free_list();
    if (frame == NULL)
    {
        frame = pager->select_victim_frame();
    }
    return frame;
}

void HandlePageFault(pte_t* pageTableEntry, int vPageNumber)
{
    if (currentProcess != NULL && currentProcess->CheckVPageAccess(vPageNumber))
    {
        frame_t* frame = get_frame();

        if(frame->pid != -1)
        {
            Process* victimProcess = &allProcesses[frame->pid];
            pte_t* victimPTE = &victimProcess->pageTable[frame->vPageNumber];

            for (auto iter = allProcesses.begin(); iter != allProcesses.end(); iter++)
            {
                if(iter->pid == frame->pid)
                {
                    iter->PrintUNMAP(frame->vPageNumber);
                }
            }

            if(victimPTE->modified == 1)
            {
                if(victimPTE->fileMapped != 1)
                {
                    victimPTE->pagedOut = 1;
                    victimProcess->PrintOUT();
                }
                else
                {
                    victimProcess->PrintFOUT();
                }
            }
            victimPTE->Clear();
        }

        pte_t* currentPTE = &currentProcess->pageTable[vPageNumber];
        if(currentPTE->fileMapped == 1)
        {
            currentProcess->PrintFIN();
        }
        else
        {
            if (currentPTE->pagedOut == 1)
            {
                currentProcess->PrintIN();
            }
            else
            {
                currentProcess->PrintZERO();
            }
        }

        currentProcess->PrintMAP(frame->frameAddress);
        frame->age = 0;
        pager->ClearFrameLastAcessed(frame);
        currentPTE->modified = 0;
        currentPTE->referenced = 0;
        currentPTE->valid = 1;
        currentPTE->frameAddress = frame->frameAddress;

        frame->pid = currentProcess->pid;
        frame->vPageNumber = vPageNumber;
    }
    else
    {
        currentProcess->PrintSEGV();
    }
}


void HandleProcessExit()
{
    if (oohNohOption)
    {
        printf("EXIT current process %d\n",currentProcess->pid);
    }
    pte_t* currentProcessPageTable = currentProcess->pageTable;
    for(int i=0; i < MAX_VPAGES; i++)
    {
        currentProcessPageTable[i].pagedOut = 0;
        if(currentProcessPageTable[i].valid == 1)
        {
            currentProcess->PrintUNMAP(i);
            if(currentProcessPageTable[i].modified == 1 && currentProcessPageTable[i].fileMapped == 1)
            {
                currentProcess->PrintFOUT();
            }
            frame_t* frame = &frameTable[currentProcessPageTable[i].frameAddress];
            frame->pid = -1;
            frame->vPageNumber = -1;
            freeMemoryPool.push_back(frame);
            currentProcessPageTable[i].valid = 0;
        } 
    }
}

bool get_next_instruction(INSTRUCTION* instruction, int* value)
{
    if (instructions.empty())
    {
        return false;
    }
    auto pair_ins_value = instructions.front();
    *instruction = pair_ins_value.first;
    *value = pair_ins_value.second;
    instructions.pop_front();
    return true;
}

void Simulation()
{
    INSTRUCTION operation;
    int value;
    while (get_next_instruction(&operation, &value))
    {
        switch (operation)
        {
            case CONTEXT:
                {
                    if (oohNohOption)
                    {
                        printf("%llu: ==> %c %d\n", inst_count, 'c', value);
                    }
                    ctx_switches++;
                    // Process* p;
                    for (auto iter = allProcesses.begin(); iter != allProcesses.end(); iter++)
                    {
                        if (iter->pid == value)
                        {
                            currentProcess = &(*iter);
                        }
                    }
                    
                    break;
                }
            case EXIT:
                {
                    if (oohNohOption)
                    {
                        printf("%llu: ==> %c %d\n", inst_count, 'e', value);
                    }
                    HandleProcessExit();
                    process_exits++;
                    break;
                }
            case READ:
                {
                    if (oohNohOption)
                    {
                        printf("%llu: ==> %c %d\n", inst_count, 'r', value);
                    }
                    pte_t* pTE = &currentProcess->pageTable[value];
                    if(!pTE->valid)
                    {
                        // handle page fault
                        HandlePageFault(pTE, value);
                    }
                    pTE->referenced = 1;
                    readWriteInstructions++;
                    break;
                }
            case WRITE:
                {
                    if (oohNohOption)
                    {
                        printf("%llu: ==> %c %d\n", inst_count, 'w', value);
                    }
                    pte_t* pTE = &currentProcess->pageTable[value];
                    if(!pTE->valid)
                    {
                        // handle page fault
                        HandlePageFault(pTE, value);
                    }
                    pTE->referenced = 1;
                    if (pTE->writeProtected == 1)
                    {
                        currentProcess->PrintSEGPROT();
                    }
                    else
                    {
                        pTE->modified = 1;
                    }
                    readWriteInstructions++;
                    break;
                }
        }

        inst_count++;
    }
}

void PrintPageTable()
{
    if (!pageTableOption)
    {
        return;
    }

    for (auto proc = allProcesses.begin(); proc != allProcesses.end(); proc++)
    {
        pte_t* processPageTable = proc->pageTable;
        printf("PT[%d]:",proc->pid);

        for(int i=0; i < MAX_VPAGES; i++)
        {
            if(processPageTable[i].valid == 0)
            {
                if(processPageTable[i].pagedOut == 1)
                {
                    printf(" #");
                }
                else if(processPageTable[i].fileMapped == 0)
                {
                    printf(" *");
                }
                else
                {
                    printf(" *");
                }
            }
            else
            {
                printf(" %d:%c%c%c",i,
                    (processPageTable[i].referenced == 1) ? 'R' : '-',
                    (processPageTable[i].modified == 1) ? 'M' : '-',
                    (processPageTable[i].pagedOut == 1) ? 'S' : '-'
                );
            }
        }
        printf("\n");
    }
}

void PrintFrameTable()
{
    // cout << __func__ << " frame table option " << (frameTableOption ? "true" : "false") << endl;
    if(!frameTableOption)
    {
        return;
    }
    printf("FT:");
    for (int i = 0; i < totalFrames; i++)
    {
        if (frameTable[i].pid == -1)
        {
            printf(" *");
        }
        else
        {
            printf(" %d:%d", frameTable[i].pid,frameTable[i].vPageNumber);
        }
    }
    printf("\n");

}

void PrintCosts()
{
    if (!statsNSummaryOption)
    {
        return;
    }
    int totalUnmaps = 0, totalMaps = 0, totalIns = 0, totalOuts = 0, 
    totalFins = 0, totalFouts = 0, totalZeros = 0, totalSegv = 0, totalSegprot = 0;

    for (auto proc = allProcesses.begin(); proc != allProcesses.end(); proc++)
    {
        proc->PrintStats();
        totalUnmaps += proc->pstats.unmaps;
        totalMaps += proc->pstats.maps;
        totalIns += proc->pstats.ins;
        totalOuts += proc->pstats.outs;
        totalFins += proc->pstats.fins;
        totalFouts += proc->pstats.fouts;
        totalZeros += proc->pstats.zeros;
        totalSegv += proc->pstats.segv;
        totalSegprot += proc->pstats.segprot;
    }

    unsigned long long cost = 0;
    cost += readWriteInstructions * costMap[KREAD_WRITE];
    cost += ctx_switches    * costMap[KCONTEXT_SWITCH];
    cost += process_exits   * costMap[KEXIT];
    cost += totalUnmaps   * costMap[KUNMAP];
    cost += totalMaps     * costMap[KMAP];
    cost += totalIns      * costMap[KIN];
    cost += totalOuts     * costMap[KOUT];
    cost += totalFins     * costMap[KFIN];
    cost += totalFouts    * costMap[KFOUT];
    cost += totalZeros    * costMap[KZERO];
    cost += totalSegv     * costMap[KSEGV];
    cost += totalSegprot  * costMap[KSEGPROT];

    printf("TOTALCOST %lu %lu %lu %llu %lu\n",
              inst_count, ctx_switches, process_exits, cost, sizeof(pte_t));

}

void PrintSummary()
{
    PrintPageTable();
    PrintFrameTable();
    PrintCosts();
}


int main (int argc, char** argv)
{
    // Parse Command Line Options
    ParseCommandLineOptions(argc, argv);

    // Read random numbers
    ReadRandomNumbers();

    // Read input file
    ReadProcessInput();

    // Run simulation
    Simulation();

    // Print final output
    PrintSummary();

}