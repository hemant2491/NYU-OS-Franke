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


using namespace std;

enum PROCESS_STATE {STATE_CREATED=0, STATE_READY, STATE_RUNNING, STATE_BLOCKED};
string ProcessStateStrings[4] = {"CREATED", "READY", "RUNNG", "BLOCK"};
enum TRANSITION_TYPE {TRANS_TO_READY=0, TRANS_TO_RUN, TRANS_TO_BLOCK, TRANS_TO_PREEMPT, TRANS_TO_DONE};
string TransitionTypeStrings[5] = {"TRANS_TO_READY", "TRANS_TO_RUN", "TRANS_TO_BLOCK", "TRANS_TO_PREEMPT", "TRANS_TO_DONE"};
enum SCHEDULER_TYPE {FCFS=0, LCFS, SRTF, RR, PRIO, PREPRIO};

class Process;
class Event;
class Scheduler;
int GetRandom(int burst);

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
int simulationCPUUtilization = 0;
int simulationIOUtilization = 0;
int cPUUtilizationWindowBegin = 0;
int cPUUtilizationWindowEnd = 0;
int iOUtilizationWindowBegin = 0;
int iOUtilizationWindowEnd = 0;
Scheduler* scheduler;
Process* currentRunningProcess;
SCHEDULER_TYPE schedulerType;
bool printVerbose = false;

class Process
{
    public:
        const int pid;
        const int arrivalTime;
        const int totalCPU;
        const int maxCPUBurst;
        const int maxIOBurst;
        int staticPriority;
        int dynamicPriority;
        PROCESS_STATE state, lastState;
        int nextBurst = 0;
        int totalRunTime = 0;
        int finishingTime = 0;
        int iOTime = 0;
        int cPUWaitingTime = 0;
        int lastStateTS = 0;
        int lastRunTime = 0;
        int lastIOBurst = 0;

    Process(int _pid, int _arrivalTime, int _totalCPU, int _maxCPUBurst, int _maxIOBurst) :
    pid(_pid),
    arrivalTime(_arrivalTime),
    totalCPU(_totalCPU),
    maxCPUBurst(_maxCPUBurst),
    maxIOBurst(_maxIOBurst)
    {
        state = STATE_CREATED;
    }

    // Process(){}

    void SetPriority(int _static, int _dynamic)
    {
        staticPriority = _static;
        dynamicPriority = _dynamic;
    }

    void Print()
    {
        printf("Process %d - %04d(AT) %d(TC) %d(CB) %d(IB)", pid, arrivalTime, totalCPU, maxCPUBurst, maxIOBurst);
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
        processPtr = &(*_processPtr);
        // printf("Event constructor: ");
        // this->Print();
        // printf("\n");
    }

    // Event(){}

    void Print()
    {
        printf("Event [%s @ %04d on ", TransitionTypeStrings[(int)transition].c_str(), eventTime);
        processPtr->Print();
        printf("]\n");
    }
};

class Scheduler
{
    public:
        virtual const char* GetSchedulerInfo() = 0;
        virtual void AddProcess(Process* p) = 0;
        virtual Process* GetNextProcess() = 0;
        virtual bool TestPreempt(Process* p, int curtime) = 0;
    
    public:
        int quantum = 10000;
        int maxprio = 4;

};

class FCFSScheduler : public Scheduler
{
    private:
        queue<Process*> runQueue;

    public:
        const char* GetSchedulerInfo() { return "FCFS";}
        void AddProcess(Process* p)
        {
            p->nextBurst = GetRandom(p->maxCPUBurst);
            printf("Adding process to run queue ");
            p->Print();
            printf("\n");
            // for (auto iter = allProcesses.begin(); iter != allProcesses.end(); iter++)
            // {
            //     if (iter->pid == proc->pid)
            //     {
            //         printf("Adding process to run queue ");
            //         iter->Print();
            //         printf("\n");
            //         runQueue.push_back(&(*iter));
            //     }
            // }
            runQueue.push(p);
            // printf("Run queue size %d\n", runQueue.size());
            // PrintRunQueue(runQueue);
        }
        Process* GetNextProcess()
        {
            if(runQueue.size()==0){
                return NULL;
            }
            Process* front = runQueue.front();
            runQueue.pop();
            return front;
        }
        bool TestPreempt(Process* p, int curtime) { return false;}
        void PrintRunQueue(queue<Process*> q)
        {
            printf("Run Queue(%d):\n", q.size());
            while (!q.empty())
            {
                q.front()->Print();
                printf("\n");
                q.pop();
            }
        }
    
};
// class LCFSScheduler : public Scheduler
// {
//     private:
//         stack<Process*> runQueue;

//     public:
//         char* GetSchedulerInfo() { return "LCFS";}
//         void AddProcess(Process* p)
//         {
//             //Todo
//         }
//         Process* GetNextProcess()
//         {
//             Process* front = runQueue.front();
//             runQueue.pop();
//             return front;
//         }
//         bool TestPreempt(Process* p, int curtime) { return false;}
    
// };

// class SRTFScheduler : public Scheduler
// {
//     private:
//         queue<Process*> runQueue;

//     public:
//         char* GetSchedulerInfo() { return "SRTF";}
//         void AddProcess(Process* p)
//         {
//             //Todo
//         }
//         Process* GetNextProcess()
//         {
//             Process* front = runQueue.front();
//             runQueue.pop();
//             return front;
//         }
//         bool TestPreempt(Process* p, int curtime) { return false;}
    
// };

// class RRScheduler : public Scheduler
// {
//     private:
//         queue<Process*> runQueue;

//     public:
//         char* GetSchedulerInfo()
//         { 
//             stringstream ss;
//             ss << "RR " << quantum;
//             // string tmpStr = "RR" + " " + quantum;
//             return ss.str().c_str();
//         }
//         void AddProcess(Process* p)
//         {
//             //Todo
//         }
//         Process* GetNextProcess()
//         {
//             Process* front = runQueue.front();
//             runQueue.pop();
//             return front;
//         }
//         bool TestPreempt(Process* p, int curtime) { return true;}
//         void SetQuantum(int q) { quantum = q;}
    
// };

// class PrioScheduler : public Scheduler
// {
//     private:
//         queue<Process*> runQueue;

//     public:
//         char* GetSchedulerInfo()
//         { 
//             stringstream ss;
//             ss << "PRIO " << quantum;
//             return ss.str().c_str();
//         }
//         void AddProcess(Process* p)
//         {
//             //Todo
//         }
//         Process* GetNextProcess()
//         {
//             Process* front = runQueue.front();
//             runQueue.pop();
//             return front;
//         }
//         bool TestPreempt(Process* p, int curtime) { return true;}
//         void SetQuantum(int q) { quantum = q;}
// };

// class PrePrioScheduler : public Scheduler
// {
//     private:
//         queue<Process*> runQueue;

//     public:
//         char* GetSchedulerInfo()
//         { 
//             stringstream ss;
//             ss << "PREPRIO " << quantum;
//             return ss.str().c_str();
//         }
//         void AddProcess(Process* p)
//         {
//             //Todo
//         }
//         Process* GetNextProcess()
//         {
//             Process* front = runQueue.front();
//             runQueue.pop();
//             return front;
//         }
//         bool TestPreempt(Process* p, int curtime) { return true;}
//         void SetQuantum(int q) { quantum = q;}
// };

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

void PrintEventQueue()
{
    printf("\n===================  Event queue  ====================\n");
    for (auto evtIter = eventQueue.begin(); evtIter != eventQueue.end(); evtIter++)
    {
        evtIter->Print();
        // Process* tmpProc = evt.processPtr;
        // Process* tmpProc = evtIter->processPtr;
        // printf("Event at %04d: Process(%04d, %d, %d, %d) transition %d\n", 
                // evt.eventTime, tmpProc->arrivalTime, tmpProc->totalCPU, tmpProc->maxCPUBurst, tmpProc->maxIOBurst, evt.transition);
                // evtIter->eventTime, evtIter->processPtr->arrivalTime, evtIter->processPtr->totalCPU, evtIter->processPtr->maxCPUBurst, evtIter->processPtr->maxIOBurst, evtIter->transition);
    }
    printf("======================================================\n\n");
}


void AddEventToQueue(int eventTime, TRANSITION_TYPE transition, Process* processPtr)
{
    auto index = find_if(eventQueue.begin(), eventQueue.end(), [eventTime] (auto e)
    {
        return e.eventTime > eventTime;
    });

    Event evt(eventTime, transition, processPtr);
    // printf("Adding event to queue - ");
    // evt.Print();
    eventQueue.insert(index, evt);
    // PrintEventQueue();
}

void ReadProcessInput()
{
    ifstream fin;
    string line;
    int lineNumber = 0;
    vector<Process>::iterator proc = allProcesses.begin();
    printf("Reading process details from %s\n", INPUT_FILE.c_str());

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
        // printf("Input line %d - ac %d tc %d cb %d io %d parsed from %s\n", lineNumber, at, tc, cb, io, line.c_str());
        
        Process proc = Process(lineNumber-1, at, tc, cb, io);

        proc.staticPriority = GetRandom(scheduler->maxprio);
        Process* procPtr = &proc;
        allProcesses.push_back(proc);
        // AddEventToQueue(at, TRANS_TO_READY, &allProcesses.back());
        // printf("Process(%d, %d, %d, %d)\n", proc.arrivalTime, proc.totalCPU, proc.maxCPUBurst, proc.maxIOBurst);
    }

    for (auto iter = allProcesses.begin(); iter != allProcesses.end() ; iter++)
    {
        AddEventToQueue(iter->arrivalTime, TRANS_TO_READY, &(*iter));
    }

    PrintEventQueue();

    fin.close();
}

void Simulation()
{
    // Event event;
    // int lastRunTime;
    while (!eventQueue.empty())
    {
        Event event = eventQueue.front();
        Process* proc = &(*event.processPtr);
        simulationCurrentTime = event.eventTime;
        TRANSITION_TYPE transition = event.transition;
        bool callScheduler = false;
        int eventBurst = 0;
        eventQueue.pop_front();
        printf(">>>>>>>>    Processing ");
        event.Print();
        // PrintEventQueue();
        // int timeInPrevState = simulationCurrentTime - process.stateTimestamp;

        switch(transition)
        {
            case TRANS_TO_READY:
                proc->lastState = proc->state;
                proc->state = STATE_READY;
                // Add to RunQueue
                scheduler->AddProcess(&(*proc));
                callScheduler = true;
                proc->lastStateTS = simulationCurrentTime;
                break;
            case TRANS_TO_RUN:
                currentRunningProcess = proc;
                // create Event for either preemption or blocking
                // if (scheduler->TestPreempt(proc, simulationCurrentTime))
                proc->lastState = proc->state;
                proc->state = STATE_RUNNING;
                proc->cPUWaitingTime += (simulationCurrentTime - proc->lastStateTS);
                int runtime;
                TRANSITION_TYPE trans;
                if (proc->nextBurst > scheduler->quantum)
                {
                    runtime = scheduler->quantum;
                    proc->nextBurst -= runtime;
                    trans = TRANS_TO_PREEMPT;
                }
                else
                {
                    runtime = proc->nextBurst;
                    trans = TRANS_TO_BLOCK;
                }
                if (proc->totalRunTime + runtime >= proc->totalCPU)
                {
                    runtime = proc->totalCPU - proc->totalRunTime;
                    proc->finishingTime = simulationCurrentTime + runtime;
                    trans = TRANS_TO_BLOCK;
                }
                proc->totalRunTime += runtime;
                proc->lastRunTime = runtime;
                proc->lastStateTS = simulationCurrentTime;
                AddEventToQueue(simulationCurrentTime + runtime, trans, &(*proc));
                if (cPUUtilizationWindowEnd < simulationCurrentTime)
                {
                    simulationCPUUtilization += (cPUUtilizationWindowEnd - cPUUtilizationWindowBegin);
                    cPUUtilizationWindowBegin = simulationCurrentTime;
                    cPUUtilizationWindowEnd = runtime;
                }
                else if (cPUUtilizationWindowEnd < simulationCurrentTime)
                {
                    cPUUtilizationWindowEnd = simulationCurrentTime;
                }
                break;
            case TRANS_TO_BLOCK:
                // create Event for when process becomes Ready to run
                currentRunningProcess = NULL;
                callScheduler = true;
                proc->lastState = proc->state;
                proc->state = STATE_BLOCKED;
                proc->lastStateTS = simulationCurrentTime;
                if (proc->totalRunTime < proc->totalCPU)
                {
                    int nextBlock = GetRandom(proc->maxIOBurst);
                    proc->iOTime += nextBlock;
                    proc->lastIOBurst = nextBlock;
                    AddEventToQueue(simulationCurrentTime + nextBlock, TRANS_TO_READY, &(*proc));
                    if (iOUtilizationWindowEnd < simulationCurrentTime)
                    {
                        simulationIOUtilization += (iOUtilizationWindowEnd - iOUtilizationWindowBegin);
                        iOUtilizationWindowBegin = simulationCurrentTime;
                        iOUtilizationWindowEnd = runtime;
                    }
                    else if (iOUtilizationWindowEnd < simulationCurrentTime)
                    {
                        iOUtilizationWindowEnd = simulationCurrentTime;
                    }
                }
                else
                {
                    printf("%d Process completed\n",proc->pid);
                }
                break;
            case TRANS_TO_PREEMPT:
                scheduler->AddProcess(&(*proc));
                currentRunningProcess = NULL;
                callScheduler = true;
                break;
            case TRANS_TO_DONE:
                printf("%d Process completed\n",proc->pid);
        }

        if (printVerbose && proc->state == STATE_RUNNING)
        {
            printf("%d %d 0: %s -> %s cb=%d rem=%d prio=%d\n", simulationCurrentTime, proc->pid, 
                    ProcessStateStrings[proc->lastState].c_str(), ProcessStateStrings[proc->state].c_str(),
                    proc->nextBurst, proc->totalCPU - proc->totalRunTime, proc->staticPriority);
        }
        else if (printVerbose && proc->state == STATE_BLOCKED)
        {
            printf("%d %d %d: %s -> %s ib=%d rem=%d\n", simulationCurrentTime, proc->pid, proc->lastState == STATE_RUNNING ? proc->lastRunTime : 0, 
                    ProcessStateStrings[proc->lastState].c_str(), ProcessStateStrings[proc->state].c_str(),
                    proc->lastIOBurst, proc->totalCPU - proc->totalRunTime);
        }
        else
        {
            printf("%d %d %d: %s -> %s\n", simulationCurrentTime, proc->pid, proc->lastState == STATE_RUNNING ? proc->lastRunTime : proc->lastIOBurst, 
                    ProcessStateStrings[proc->lastState].c_str(), ProcessStateStrings[proc->state].c_str());
        }
        

        if (callScheduler)
        {
            cout << "AAAAA" << endl;
            if (eventQueue.front().eventTime == simulationCurrentTime) { continue;}
            callScheduler = false;
            cout << "BBBBB" << endl;
            if (currentRunningProcess == NULL)
            {
                cout << "CCCCC" << endl;
                Process* p = scheduler->GetNextProcess();
                if (p == NULL) { continue;}
                cout << "DDDDD" << endl;
                AddEventToQueue(simulationCurrentTime, TRANS_TO_RUN, p);
            }
        }
        // printf("Event Queue after processing this event.\n");
        // PrintEventQueue();
    }
}

void PrintSummary()
{
    printf("%s\n",scheduler->GetSchedulerInfo());
    int processCounter;
    int totalTurnAroundTime;
    int totalCPUWaitingTime;
    for (Process proc : allProcesses)
    {
        processCounter++;
        int turnAroundTime = proc.finishingTime - proc.arrivalTime;
        totalTurnAroundTime += turnAroundTime;
        totalCPUWaitingTime += proc.cPUWaitingTime;
        printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n", 
        proc.pid, proc.arrivalTime, proc.totalCPU, proc.maxCPUBurst, proc.maxIOBurst, proc.staticPriority,
        proc.finishingTime, proc.finishingTime - proc.arrivalTime, proc.iOTime, proc.cPUWaitingTime);
    }
    double avgTurnAroundTime = totalTurnAroundTime / processCounter;
    double avgCPUWaitingTime = totalCPUWaitingTime / processCounter;
    // double processPerHunderdTimeUnits = (processCounter / simulationCurrentTime) * 100;
    // printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n", simulationCurrentTime, simulationCPUUtilization, simulationIOUtilization,
    // avgTurnAroundTime, avgCPUWaitingTime, processPerHunderdTimeUnits);
    printf("Simulation total time %d\n", simulationCurrentTime);
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
                printVerbose = true;
                printf("printVerbose = %s\n", printVerbose ? "true" : "false");
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
            {
                // printf("s value\n");
                // printf("s optarg: %s\n", optarg);
                int maxprio = 4;
                int quantum;
                char sched;
                sscanf(optarg, "%c%d:%d", &sched , &quantum, &maxprio);
                switch (sched)
                {
                    case 'F':
                        scheduler = new FCFSScheduler();
                        break;
                    case 'L':
                        break;
                    case 'S':
                        break;
                    case 'R':
                        scheduler->quantum = quantum;
                        scheduler->maxprio = maxprio;
                        break;
                    case 'P':
                        scheduler->quantum = quantum;
                        scheduler->maxprio = maxprio;
                        break;
                    case 'E':
                        scheduler->quantum = quantum;
                        scheduler->maxprio = maxprio;
                        break;
                }
                printf("sched %c quantum %d maxprio %d\n", sched, quantum, maxprio);
                break;
            }
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

    PrintSummary();

}
