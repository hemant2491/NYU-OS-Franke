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


using namespace std;

enum PROCESS_STATE {STATE_CREATED=0, STATE_READY, STATE_RUNNING, STATE_BLOCKED, STATE_PREEMPT, STATE_DONE};
string ProcessStateStrings[6] = {"CREATED", "READY", "RUNNG", "BLOCK", "PREEMPT", "Done"};
enum TRANSITION_TYPE {TRANS_TO_READY=0, TRANS_TO_RUN, TRANS_TO_BLOCK, TRANS_TO_PREEMPT, TRANS_TO_DONE};
string TransitionTypeStrings[5] = {"TRANS_TO_READY", "TRANS_TO_RUN", "TRANS_TO_BLOCK", "TRANS_TO_PREEMPT", "TRANS_TO_DONE"};
enum SCHEDULER_TYPE {FCFS=0, LCFS, SRTF, RR, PRIO, PREPRIO};

class Process;
class Event;
class Scheduler;
int GetRandom(int burst);

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
        PROCESS_STATE lastState, state, nextState;
        int activeBurst = 0;
        int totalRunTime = 0;
        int finishingTime = 0;
        int iOTime = 0;
        int cPUWaitingTime = 0;
        int lastStateTransitionTS = 0;
        int lastRunTime = 0;
        int iOBurst = 0;

    Process(int _pid, int _arrivalTime, int _totalCPU, int _maxCPUBurst, int _maxIOBurst) :
    pid(_pid),
    arrivalTime(_arrivalTime),
    totalCPU(_totalCPU),
    maxCPUBurst(_maxCPUBurst),
    maxIOBurst(_maxIOBurst)
    {
        state = STATE_CREATED;
    }

    void SetPriority(int _static, int _dynamic)
    {
        staticPriority = _static;
        dynamicPriority = _dynamic;
    }

    void UpdateNextState(int currentTS, PROCESS_STATE next_state)
    {
        lastState = state;
        state = nextState;
        nextState = next_state;
        lastStateTransitionTS = currentTS;
    }

    void CalculateNextCPUBurst()
    {
        if (activeBurst == 0)
        {
            activeBurst = GetRandom(maxCPUBurst);
        }
    }

    tuple<int, TRANSITION_TYPE, PROCESS_STATE> Run(int currentTS)
    {
        // check for either preemption or blocking
        int runtime;
        TRANSITION_TYPE trans;
        PROCESS_STATE next_state;
        
        if (activeBurst > scheduler->quantum)
        {
            runtime = scheduler->quantum;
            activeBurst -= runtime;
            trans = TRANS_TO_PREEMPT;
            next_state = STATE_PREEMPT;
        }
        else
        {
            runtime = activeBurst;
            activeBurst = 0;
            trans = TRANS_TO_BLOCK;
            next_state = STATE_BLOCKED;
        }
        // check if total CPU demand met and add transition to stop process
        if (totalRunTime + runtime >= totalCPU)
        {
            runtime = totalCPU - totalRunTime;
            finishingTime = currentTS + runtime;
            trans = TRANS_TO_DONE;
            next_state = STATE_DONE;
        }
        totalRunTime += runtime;
        lastRunTime = runtime;
        return {runtime, trans, next_state};
    }

    int GetIOBurst()
    {
        iOBurst = GetRandom(maxIOBurst);
        iOTime += iOBurst;
        return iOBurst;
    }

    int Pause(int currentTS)
    {
        int nextBlock = GetRandom(maxIOBurst);
        iOTime += nextBlock;
        iOBurst = nextBlock;
        return nextBlock;
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
        Process* proc;

    Event(int _eventTime, TRANSITION_TYPE _transition, Process* _processPtr)
    {
        eventTime = _eventTime;
        transition = _transition;
        proc = &(*_processPtr);
 
    }

    void Print()
    {
        switch (transition)
        {
            case TRANS_TO_READY:
                printf("%d %d %d: %s -> %s\n", eventTime, proc->pid, proc->lastState == STATE_RUNNING ? proc->lastRunTime : proc->iOBurst, 
                            ProcessStateStrings[proc->state].c_str(), ProcessStateStrings[proc->nextState].c_str());
                break;
            case TRANS_TO_RUN:
                printf("%d %d 0: %s -> %s cb=%d rem=%d prio=%d\n", eventTime, proc->pid, 
                            ProcessStateStrings[proc->state].c_str(), ProcessStateStrings[proc->nextState].c_str(),
                            proc->activeBurst, proc->totalCPU - proc->totalRunTime, proc->dynamicPriority);
                break;
            case TRANS_TO_BLOCK:
                printf("%d %d %d: %s -> %s ib=%d rem=%d\n", eventTime, proc->pid, proc->lastState == STATE_RUNNING ? proc->lastRunTime : 0, 
                            ProcessStateStrings[proc->state].c_str(), ProcessStateStrings[proc->nextState].c_str(),
                            proc->iOBurst, proc->totalCPU - proc->totalRunTime);
                break;
            case TRANS_TO_PREEMPT:
                printf("%d %d 0: %s -> %s cb=%d rem=%d prio=%d\n", eventTime, proc->pid, 
                            ProcessStateStrings[proc->state].c_str(), ProcessStateStrings[proc->nextState].c_str(),
                            proc->activeBurst, proc->totalCPU - proc->totalRunTime, proc->dynamicPriority);
                break;
            case TRANS_TO_DONE:
                printf("%d %d %d: %s\n", eventTime, proc->pid, proc->lastRunTime, ProcessStateStrings[proc->nextState].c_str());
                break;
        }
    }
};



class FCFSScheduler : public Scheduler
{
    private:
        queue<Process*> runQueue;

    public:
        const char* GetSchedulerInfo() { return "FCFS";}
        void AddProcess(Process* p)
        {
            
            // printf("Adding process to run queue ");
            // p->Print();
            // printf("\n");
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


void AddEventToQueue(int eventTime, TRANSITION_TYPE transition, Process* proc)
{
    auto index = find_if(eventQueue.begin(), eventQueue.end(), [eventTime] (auto e)
    {
        return e.eventTime > eventTime;
    });

    Event evt(eventTime, transition, proc);
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
        proc.nextState = STATE_READY;
        int staticprio = GetRandom(scheduler->maxprio);
        proc.SetPriority(staticprio, staticprio-1);
        Process* procPtr = &proc;
        allProcesses.push_back(proc);
        // AddEventToQueue(at, TRANS_TO_READY, &allProcesses.back());
        // printf("Process(%d, %d, %d, %d)\n", proc.arrivalTime, proc.totalCPU, proc.maxCPUBurst, proc.maxIOBurst);
    }

    for (auto iter = allProcesses.begin(); iter != allProcesses.end() ; iter++)
    {
        AddEventToQueue(iter->arrivalTime, TRANS_TO_READY, &(*iter));
    }

    // PrintEventQueue();

    fin.close();
}

// void StartCPUUtilization(int currentTS, Process* proc)
// {
    
//     cPUUtilizationWindowBegin = currentTS;
// }

// void StopCPUUtilization(int currentTS)
// {
//     currentRunningProcess = NULL;
//     simulationCPUUtilization += (currentTS - cPUUtilizationWindowBegin);
//     cPUUtilizationWindowBegin = 0;
// }

void UpdateIOUtilization(int start, int end)
{
    // printf("%s: total io %d current io window start %d end %d new io start %d end %d\n",
    //             __func__, simulationIOUtilization, 
    //             iOUtilizationWindowBegin, iOUtilizationWindowEnd,
    //             start, end);
    if (iOUtilizationWindowEnd < start)
    {
        simulationIOUtilization += (iOUtilizationWindowEnd - iOUtilizationWindowBegin);
        iOUtilizationWindowBegin = start;
        iOUtilizationWindowEnd = end;
    }
    else if (iOUtilizationWindowEnd < end)
    {
        iOUtilizationWindowEnd = end;
    }
}

void UpdateRemainingIO()
{
    if (iOUtilizationWindowBegin < iOUtilizationWindowEnd)
    {
        simulationIOUtilization += (iOUtilizationWindowEnd - iOUtilizationWindowBegin);
    }
}


void Simulation()
{
    // Event event;
    // int lastRunTime;
    while (!eventQueue.empty())
    {
        Event event = eventQueue.front();
        Process* proc = &(*event.proc);
        simulationCurrentTime = event.eventTime;
        TRANSITION_TYPE transition = event.transition;
        bool callScheduler = false;
        int eventBurst = 0;
        eventQueue.pop_front();
        // if(printVerbose) { event.Print();}
        // printf(">>>>>>>>    Processing ");
        // event.Print();
        // PrintEventQueue();
        // int timeInPrevState = simulationCurrentTime - process.stateTimestamp;

        switch(transition)
        {
            case TRANS_TO_READY:
            {    
                // Add to RunQueue
                if(printVerbose) { event.Print();}
                scheduler->AddProcess(&(*proc));
                proc->UpdateNextState(simulationCurrentTime, STATE_RUNNING);
                callScheduler = true;
                break;
            }
            case TRANS_TO_RUN:
            {
                proc->cPUWaitingTime += (simulationCurrentTime - proc->lastStateTransitionTS);
                proc->CalculateNextCPUBurst();
                if(printVerbose) { event.Print();}
                auto [runtime, trans, next_state] = proc->Run(simulationCurrentTime);
                currentRunningProcess = proc;
                simulationCPUUtilization += runtime;
                AddEventToQueue(simulationCurrentTime + runtime, trans, &(*proc));
                proc->UpdateNextState(simulationCurrentTime, next_state);
                break;
            }
            case TRANS_TO_BLOCK:
            {   
                // create Event for when process becomes Ready to run
                int nextBlock = proc->GetIOBurst();
                if(printVerbose) { event.Print();}
                currentRunningProcess = NULL;
                UpdateIOUtilization(simulationCurrentTime, simulationCurrentTime + nextBlock);
                AddEventToQueue(simulationCurrentTime + nextBlock, TRANS_TO_READY, &(*proc));
                proc->UpdateNextState(simulationCurrentTime, STATE_READY);
                callScheduler = true;
                break;
            }
            case TRANS_TO_PREEMPT:
            { 
                if(printVerbose) { event.Print();}
                scheduler->AddProcess(&(*proc));
                currentRunningProcess = NULL;
                proc->UpdateNextState(simulationCurrentTime, STATE_RUNNING);
                callScheduler = true;
                break;
            }
            case TRANS_TO_DONE:
            {
                if(printVerbose) { event.Print();}
                // proc->UpdateNextState(simulationCurrentTime, STATE_DONE);
                currentRunningProcess = NULL;
                // printf("%d Process completed\n",proc->pid);
                break;
            }
        }

        if (callScheduler)
        {
            if (eventQueue.front().eventTime == simulationCurrentTime) { continue;}
            callScheduler = false;
            if (currentRunningProcess == NULL)
            {
                Process* p = scheduler->GetNextProcess();
                if (p == NULL) { continue;}
                AddEventToQueue(simulationCurrentTime, TRANS_TO_RUN, p);
            }
        }
        // printf("Event Queue after processing this event.\n");
        // PrintEventQueue();
    }
    UpdateRemainingIO();
}

void PrintSummary()
{
    printf("%s\n",scheduler->GetSchedulerInfo());
    int processCounter = 0;
    int totalTurnAroundTime = 0;
    int totalCPUWaitingTime = 0;
    for (Process proc : allProcesses)
    {
        processCounter++;
        int turnAroundTime = proc.finishingTime - proc.arrivalTime;
        totalTurnAroundTime += turnAroundTime;
        // printf("After proces %d total turn around time = %d\n", proc.pid, totalTurnAroundTime);
        totalCPUWaitingTime += proc.cPUWaitingTime;
        printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n", 
        proc.pid, proc.arrivalTime, proc.totalCPU, proc.maxCPUBurst, proc.maxIOBurst, proc.staticPriority,
        proc.finishingTime, turnAroundTime, proc.iOTime, proc.cPUWaitingTime);
    }
    // printf("\
    // Process Counter %d\n\
    // simulation total CPU utilization %d\n\
    // simulation total IO utilization %d\n\
    // total turn around time %d\n\
    // total cpu waiting time %d\n",
    //         processCounter, simulationCPUUtilization, simulationIOUtilization, totalTurnAroundTime, totalCPUWaitingTime);

    double percCPUUtilization = ((double) simulationCPUUtilization / simulationCurrentTime) * 100.00;
    double percIOUtilization = ((double) simulationIOUtilization / simulationCurrentTime) * 100.00;
    double avgTurnAroundTime = (double) totalTurnAroundTime / processCounter;
    double avgCPUWaitingTime = (double) totalCPUWaitingTime / processCounter;
    double processPerHunderdTimeUnits = ((double) processCounter / simulationCurrentTime) * 100.00;
    printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n", simulationCurrentTime, percCPUUtilization, percIOUtilization,
    avgTurnAroundTime, avgCPUWaitingTime, processPerHunderdTimeUnits);
    // printf("Simulation total time %d\n", simulationCurrentTime);
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
