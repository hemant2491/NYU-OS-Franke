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

class IOScheduler;
class IOOperation;

// FIFO (i), SSTF (j), LOOK (s), CLOOK (c), and FLOOK (f)
enum IOSCHTYPE {FIFO=0, SSTF, LOOK, CLOOK, FLOOK};

bool verboseOption = false;
bool queueInfoOption = false;
bool flookQInfoOption = false;
IOSCHTYPE ioSchType = FIFO;

string INPUT_FILE = "";
IOScheduler* scheduler;
vector<IOOperation> allOperations;
long int total_time;
long int tot_movement;
long int avg_turnaround;
long int avg_waittime;
long int max_waittime;
bool isIOActive = false;

inline std::string trim(const std::string& line)
{
    std::size_t start = line.find_first_not_of(" \t");
    std::size_t end = line.find_last_not_of(" \t");
    return line.substr(start, end - start + 1);
}

class IOOperation
{
    public:
        long int arrival_time = 0;
        long int track = 0;
        long int start_time = 0;
        long int end_time = 0;

        long int scheduledTime = 0;
        long int waitTime = 0;
    
    public:
        IOOperation(long int ist, long int trk)
        {
            arrival_time = ist;
            track = trk;
        }
};

class IOScheduler
{
    public:
        virtual void AddRequest(IOOperation* operation) {};
        virtual IOOperation* GetOperation() { return NULL;};
        virtual bool IsIdle() { return false;};

};

class FIFOIOScheduler : public IOScheduler
{
    private:
        deque<IOOperation*> operationsQueue;

    public:
        void AddRequest(IOOperation* operation)
        {
            operationsQueue.push_back(operation);
        }

        IOOperation* GetOperation()
        {
            if (operationsQueue.empty())
            {
                return NULL;
            }
            else
            {
                IOOperation* op = operationsQueue.front();
                operationsQueue.pop_front();
                return op;
            }
        }
        
        bool IsIdle()
        {
            return operationsQueue.empty();
        }
};

class SSTFIOScheduler : public FIFOIOScheduler
{

};

class LOOKIOScheduler : public FIFOIOScheduler
{

};

class CLOOKIOScheduler : public FIFOIOScheduler
{

};

class FLOOKIOScheduler : public FIFOIOScheduler
{

};

void ParseCommandLineOptions (int argc, char** argv)
{
    opterr = 0;
    int cmdopt;

    // printf("Number of arguments = %d\n", argc);
    // printf("optind: %d : %s\n", optind, argv[optind-1]);

    // while ((cmdopt = getopt(argc, argv, "vteps:")) != -1)
    while ((cmdopt = getopt(argc, argv, "vqfs:")) != -1)
    {
        // printf("optind: %d : ", optind);
        switch (cmdopt)
        {
            case 'v':
                {
                    verboseOption = true;
                    break;
                }
            case 'q':
                {
                    queueInfoOption = true;
                    break;
                }
            case 'f':
                {
                    flookQInfoOption = true;
                    break;
                }
            case 's':
                {
                    // FIFO (i), SSTF (j), LOOK (s), CLOOK (c), and FLOOK (f)
                    char _schType;
                    sscanf(optarg, "%c", &_schType);
                    switch (_schType)
                    {
                        case 'i':
                            ioSchType = FIFO;
                            scheduler = new FIFOIOScheduler();
                            break;
                        case 'j':
                            ioSchType = SSTF;
                            scheduler = new SSTFIOScheduler();
                            break;
                        case 's':
                            ioSchType = LOOK;
                            scheduler = new LOOKIOScheduler();
                            break;
                        case 'c':
                            ioSchType = CLOOK;
                            scheduler = new CLOOKIOScheduler();
                            break;
                        case 'f':
                            ioSchType = FLOOK;
                            scheduler = new FLOOKIOScheduler();
                            break;
                    }
                    break;
                }
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
        printf("Error: The command is missing input file argument\n");
    }
    else if (argc - optind > 2)
    {
        printf("Error: more than 2 non-option arguments\n");
    }
    else if (argc - optind == 1)
    {
        INPUT_FILE = argv[optind];
        // RFILE = argv[optind+1];
    }

    // printf("Input file: %s\n", INPUT_FILE.c_str());
    // printf("Random number file: %s\n", RFILE.c_str());
}

void ReadIOOperationInput()
{
    ifstream fin;
    string line;
    int lineNumber = 0;
    long int issueTime, track;
    vector<IOOperation>::iterator operation = allOperations.begin();
    // printf("Reading IOOperation details from %s\n", INPUT_FILE.c_str());

    try
    {
        fin.open(INPUT_FILE);
    }
    catch(std::exception const& e)
    {
        printf("There was an error in opening input file %s: %s\n", INPUT_FILE.c_str(), e.what());
        exit(1);
    }

    while (fin)
    {
        while(getline(fin, line))
        {
            line = trim(line);
            if(line[0] != '#')
            {
                sscanf(line.c_str(), "%ld %ld", &issueTime, &track);
                allOperations.push_back(IOOperation(issueTime, track));
                break;
            }
        }
    }

    fin.close();
}

void Simulation()
{
    auto currentOperation = allOperations.begin();
    int currentTime = 0;
    int currentHead = 0;
    bool hasTrackMoved = false;
    bool isIOActive = false;
    int direction = 0;
    while (true)
    {
        // if a new I/O arrived to the system at this current time
        if(currentOperation != allOperations.end() && currentOperation->arrival_time == currentTime)
        {
            // → add request to IO-queue
            scheduler->AddRequest(&(*currentOperation));
            currentOperation++;
        }
        // if an IO is active and completed at this time
        if (isIOActive && currentOperation->end_time == currentTime)
        {
            // → Compute relevant info and store in IO request for final summary if no IO request active now
            isIOActive = false;
        }
        // if requests are pending
        if(!isIOActive && !scheduler->IsIdle())
        {
            // → Fetch the next request from IO-queue and start the new IO.
            IOOperation* op = scheduler->GetOperation();
            op->waitTime = currentTime - op->arrival_time;
            op->start_time = currentTime;
            op->end_time = abs(op->track - currentHead);
            if (op->track > currentHead)
            {
                direction = 1;
            }
            else if (op->track < currentHead)
            {
                direction = -1;
            }
            else
            {
                direction = 0;
            }
            isIOActive = true;
        }
        // else if all IO from input file processed
        if (currentOperation == allOperations.end() && scheduler->IsIdle())
        {
            // → exit simulation
            break;
        }
        
        // if an IO is active
        if(isIOActive)
        {
            // → Move the head by one unit in the direction its going (to simulate seek)
            currentHead += direction;
            tot_movement++;
        }
        // Increment time by 1
        if (hasTrackMoved)
        {
            currentTime++;
            total_time++;
        }

    }
    // */
}

void PrintSummary()
{
    // Print for each operation
    // printf("%5d: %5d %5d %5d\n",i, req->arrival_time, r->start_time, r->end_time);
        // - IO-op#,
        // - its arrival to the system (same as inputfile)
        // - its disk service start time
        // - its disk service end time
    
    int i = 0;
    long int total_wait_time = 0;
    long int total_turn_around_time = 0;
    for (auto iter = allOperations.begin(); iter != allOperations.end(); iter++)
    {
        printf("%5d: %5d %5d %5d\n",i, iter->arrival_time, iter->start_time, iter->end_time);
        total_wait_time += iter->waitTime;
        total_turn_around_time += (iter->end_time - iter->arrival_time);
        max_waittime = max_waittime < iter->waitTime ? iter->waitTime : max_waittime;
        i++;
    }

    avg_turnaround = total_turn_around_time / i;
    avg_waittime = total_wait_time / i;

    // printf("SUM: %d %d %.2lf %.2lf %d\n",
    // total_time, tot_movement, avg_turnaround, avg_waittime, max_waittime);
    // 
    // total_time:      total simulated time, i.e. until the last I/O request has completed.
    // tot_movement:    total number of tracks the head had to be moved
    // avg_turnaround:  average turnaround time per operation from time of submission to time of completion
    // avg_waittime:    average wait time per operation (time from submission to issue of IO request to start disk operation)
    // max_waittime:    maximum wait time for any IO operation.
    printf("SUM: %d %d %.2lf %.2lf %d\n",
    total_time, tot_movement, avg_turnaround, avg_waittime, max_waittime);
}

int main (int argc, char** argv)
{
    // Parse Command Line Options
    ParseCommandLineOptions(argc, argv);

    // Read random numbers
    // ReadRandomNumbers();

    // Read input file
    ReadIOOperationInput();

    // Run simulation
    Simulation();

    // Print final output
    PrintSummary();

}
