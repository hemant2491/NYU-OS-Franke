#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <deque>
#include <string>
#include <string.h>
#include <fstream>
#include <algorithm>


using namespace std;

enum PROCESS_STATE {STATE_CREATED, STATE_READY, STATE_RUNNING, STATE_BLOCKED};
enum TRANSITION_TYPE {TRANS_TO_READY, TRANS_TO_RUN, TRANS_TO_BLOCK, TRANS_TO_PREEMPT};

class Process
{
    public:
        int arrivalTime;
        int totalCPU;
        int maxCPUBurst;
        int maxIOBurst;
        int staticPriority;
        int dynamicPriority;
        PROCESS_STATE state;
        int nextBurst = 0;
        int totalRunTime = 0;

    Process(int _arrivalTime, int _totalCPU, int _maxCPUBurst, int _maxIOBurst)
    {
        arrivalTime = _arrivalTime;
        totalCPU = _totalCPU;
        maxCPUBurst = _maxCPUBurst;
        maxIOBurst = _maxIOBurst;
        state = STATE_CREATED;
    }

    Process(){}

    void setPriority(int _static, int _dynamic)
    {
        staticPriority = _static;
        dynamicPriority = _dynamic;
    }
};

class Event
{
    public:
        int eventTime;
        TRANSITION_TYPE transition;
        Process* processPtr;

    Event(int _eventTime, TRANSITION_TYPE _transition, Process* _processPtr)
    {
        eventTime = _eventTime;
        transition = _transition;
        processPtr = _processPtr;
    }

    Event(){}

};

class Scheduler
{
    public:
        virtual void AddProcess(Process* p) = 0;
        virtual Process* GetNextProcess() = 0;
        virtual bool TestPreempt(Process* p, int curtime) = 0;
    
    public:
        int quantum = 1000000;
};

char DELIMS[3] = {' ', '\t', '\n'};
// const int PROCESS_COUNT = 4;
string INPUT_FILE = "";
string RFILE = "";
vector<int> randomValues;
vector<int>::iterator randomValuesIter;
deque<Event> eventQueue;
// deque<Process> runQueue;
// Process allProcesses[PROCESS_COUNT];
vector<Process> allProcesses;
int simulationCurrentTime;
int maxprio;
int quantum;
Scheduler* scheduler;
Process* currentRunningProcess;

void AddEventToQueue(int eventTime, TRANSITION_TYPE transition, Process* processPtr)
{
    // printf("Event time %d > ", eventTime);
    auto index = find_if(eventQueue.begin(), eventQueue.end(), [eventTime] (auto e)
    {
        // printf("%d ", e.eventTime);
        return e.eventTime > eventTime;
    });
    // printf("\n");

    eventQueue.insert(index, Event(eventTime, transition, processPtr));
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

    // int count=0;
    // for (auto iter = randomValues.begin(); iter != randomValues.end() ; iter++)
    // {
    //     count++;
    //     printf("%d: '%d'\n", count, *iter);
    // }
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

    return (1 + (nextRandom % burst));
}

void ReadProcessInput()
{
    ifstream fin;
    string line;
    int lineNumber = 0;
    vector<Process>::iterator proc = allProcesses.begin();

    try
    {
        fin.open(INPUT_FILE);
    }
    catch(std::exception const& e)
    {
        // cout << "There was an error in opening input file " << INPUT_FILE << ": " << e.what() << endl;
        printf("There was an error in opening input file %s: %s\n", INPUT_FILE.c_str(), e.what());
        exit(1);
    }

    while (fin) {
        // Read a line from input file
        getline(fin, line);
        if (line.find_first_not_of(" \t") == line.npos) { continue;}
        lineNumber++;
        int processDetails [4];
        int detailIndex = 0;
        int r;
        char line_cstr[line.length() + 1];
        strcpy(line_cstr, line.c_str());
        char *token = strtok(line_cstr, DELIMS);
        while (token != NULL)
        {
            try
            {
                r = stoi(token);
            }
            catch(std::exception const& e)
            {
                printf("There was an error reading input %s at line number %d in input file %s: %s\n", 
                        token, lineNumber, INPUT_FILE.c_str(), e.what());
                exit(1);
            }
            processDetails[detailIndex] = r;
            detailIndex++;
            token = strtok(NULL, DELIMS);
        }

        auto [at, tc, cb, io] = processDetails;
        // printf("Read from input: %d - Process(%d, %d, %d, %d) from %s\n", lineNumber, at, tc, cb, io, line.c_str());
        
        // Process proc = Process(at, tc, cb, io);
        // Process* procPtr = &proc;
        // allProcesses.push_back(proc);
        allProcesses.push_back(Process(at, tc, cb, io));
        proc++;
        Process* procPtr = &(*proc);
        // simulationCurrentTime = processDetails[0];
        AddEventToQueue(at, TRANS_TO_READY, &allProcesses.back());
    }

    // for (Event evt : eventQueue)
    // {
    //     Process* tmpProc = evt.processPtr;
    //     printf("Event at %d: Process(%d, %d, %d, %d) transition %d\n", 
    //             evt.eventTime, tmpProc->arrivalTime, tmpProc->totalCPU, tmpProc->maxCPUBurst, tmpProc->maxIOBurst, evt.transition);
    // }

    fin.close();
}

void Simulation()
{
    Event event;
    while (!eventQueue.empty())
    {
        event = eventQueue.front();
        Process* proc = event.processPtr;
        simulationCurrentTime = event.eventTime;
        int transition = event.transition;
        bool callScheduler = false;
        eventQueue.pop_front();
        // printf("%d: Process(%d, %d, %d, %d) transition %d\n", 
        //         simulationCurrentTime, proc->arrivalTime, proc->totalCPU, proc->maxCPUBurst, proc->maxIOBurst, transition);
        // continue;
        // int timeInPrevState = simulationCurrentTime - process.stateTimestamp;

        switch(transition)
        {
            case TRANS_TO_READY:
                // must come from BLOCK or PRE-EMPTION
                if (proc->state == STATE_RUNNING)
                {
                    printf("Error: recieved Running state event in TRANS_TO_READY\n");
                }
                proc->state = STATE_READY;
                // Add to RunQueue
                scheduler->AddProcess(proc);
                callScheduler = true;
                break;
            case TRANS_TO_RUN:
                currentRunningProcess = proc;
                // create Event for either preemption or blocking
                // if (scheduler->TestPreempt(proc, simulationCurrentTime))
                if (proc->nextBurst > scheduler->quantum)
                {
                    proc->nextBurst = proc->nextBurst - scheduler->quantum;
                    AddEventToQueue(simulationCurrentTime + scheduler->quantum, TRANS_TO_PREEMPT, proc);
                }
                else
                {
                    AddEventToQueue(simulationCurrentTime + proc->nextBurst, TRANS_TO_BLOCK, proc);
                }
                break;
            case TRANS_TO_BLOCK:
                // create Event for when process becomes Ready to run
                currentRunningProcess = NULL;
                callScheduler = true;
                if (proc->totalRunTime < proc->totalCPU)
                {
                    int nextBlock = GetRandom(proc->maxIOBurst);
                    AddEventToQueue(simulationCurrentTime + nextBlock, TRANS_TO_READY, proc);
                }
                break;
            case TRANS_TO_PREEMPT:
                scheduler->AddProcess(proc);
                currentRunningProcess = NULL;
                callScheduler = true;
                break;
        }

        if (callScheduler)
        {
            if (eventQueue.front().eventTime == simulationCurrentTime) { continue;}
            callScheduler = false;
            if (currentRunningProcess == NULL)
            {
                Process* p = scheduler->GetNextProcess();
                if (p ==NULL) { continue;}
                AddEventToQueue(simulationCurrentTime, TRANS_TO_RUN, p);
            }
        }
    }
}

int main (int argc, char** argv)
{
    opterr = 0;
    int cmdopt;

    printf("Number of arguments = %d\n", argc);
    printf("optind: %d : %s\n", optind, argv[optind-1]);

    while ((cmdopt = getopt(argc, argv, "vteps:")) != -1)
    {
        printf("optind: %d : ", optind);
        switch (cmdopt)
        {
            case 'v':
                printf("v value\n");
                break;
            case 't':
                printf("t value\n");
                break;
            case 'e':
                printf("e value\n");
                break;
            case 'p':
                printf("p value\n");
                break;
            case 's':
                printf("s value\n");
                printf("s optarg: %s\n", optarg);
                // sscanf(optarg, “%d . %d”, &quantum, &maxprio);
                // printf("quantum %d maxprio %d", quantum, maxprio);
                break;
            case '?':
                printf("?: optopt = -%c\n", optopt);
                if (optopt == 's') { printf("Option -%c requires an argument\n", optopt); }
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

    printf("Input file: %s\n", INPUT_FILE.c_str());
    printf("Random number file: %s\n", RFILE.c_str());

    // Read random numbers
    ReadRandomNumbers();

    // Read Process Input file
    ReadProcessInput();

    Simulation();

}
