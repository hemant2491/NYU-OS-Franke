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
double avg_turnaround;
double avg_waittime;
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
        long int id = -1;
        long int arrival_time = 0;
        long int track = 0;
        long int start_time = 0;
        long int end_time = 0;
        long int waitTime = 0;
    
    public:
        IOOperation(long int ist, long int trk, long int _id)
        {
            arrival_time = ist;
            track = trk;
            id = _id;
        }
};

class IOScheduler
{
    public:
        virtual void AddRequest(IOOperation* operation) {};
        virtual IOOperation* GetOperation(long int current_head, long int last_direction) { return NULL;};
        virtual bool IsFinished() { return false;};

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

        IOOperation* GetOperation(long int current_head, long int last_direction)
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
        
        bool IsFinished()
        {
            return operationsQueue.empty();
        }
};

class SSTFIOScheduler : public IOScheduler
{
    private:
        deque<IOOperation*> operationsQueue;
    
    public:
        void AddRequest(IOOperation* operation)
        {
            operationsQueue.push_back(operation);
        }

        IOOperation* GetOperation(long int current_head, long int last_direction)
        {
            if (operationsQueue.empty())
            {
                return NULL;
            }
            else
            {
                long int minDist = INT_MAX;
                auto popIter = operationsQueue.begin();
                for (auto iter = operationsQueue.begin(); iter != operationsQueue.end(); iter++)
                {
                    long int dist = abs((*iter)->track - current_head);
                    if (minDist > dist)
                    {
                        minDist = dist;
                        popIter = iter;
                    }
                }
                
                IOOperation* op = *popIter;
                operationsQueue.erase(popIter);
                return op;
            }
        }
        
        bool IsFinished()
        {
            return operationsQueue.empty();
        }

};

class LOOKIOScheduler : public IOScheduler
{
    private:
        deque<IOOperation*> operationsQueue;

    public:
        void AddRequest(IOOperation* operation)
        {
            operationsQueue.push_back(operation);
        }

        IOOperation* GetOperation(long int current_head, long int last_direction)
        {
            if (operationsQueue.empty())
            {
                return NULL;
            }

            IOOperation* op = NULL;

            auto closestPosIter = operationsQueue.rbegin();
            auto closestNegIter = operationsQueue.rbegin();

            long int minPosDist = INT_MAX;
            long int minNegDist = INT_MAX;

            bool posFound = false;
            bool negFound = false;

            for (auto iter = operationsQueue.rbegin(); iter != operationsQueue.rend(); iter++)
            {
                if ((*iter)->track >= current_head)
                {
                    posFound = true;
                    long int pDist = (*iter)->track - current_head;
                    if (pDist <= minPosDist)
                    {
                        minPosDist = pDist;
                        closestPosIter = iter;
                    }
                }

                if ((*iter)->track <= current_head)
                {
                    negFound = true;
                    long int nDist =  current_head - (*iter)->track;
                    if (nDist <= minNegDist)
                    {
                        minNegDist = nDist;
                        closestNegIter = iter;
                    }
                }
            }

            if(last_direction > 0 && posFound)
            {
                op = *closestPosIter;
                operationsQueue.erase(next(closestPosIter).base());
            }
            else if(last_direction > 0 && !posFound)
            {
                op = *closestNegIter;
                operationsQueue.erase(next(closestNegIter).base());
            }
            if(last_direction < 0 && negFound)
            {
                op = *closestNegIter;
                operationsQueue.erase(next(closestNegIter).base());
            }
            else if(last_direction < 0 && !negFound)
            {
                op = *closestPosIter;
                operationsQueue.erase(next(closestPosIter).base());
            }

            return op;
        }

        bool IsFinished()
        {
            return operationsQueue.empty();
        }

};

class CLOOKIOScheduler : public IOScheduler
{
    private:
        deque<IOOperation*> operationsQueue;

    public:
        void AddRequest(IOOperation* operation)
        {
            operationsQueue.push_back(operation);
        }

        IOOperation* GetOperation(long int current_head, long int last_direction)
        {
            if (operationsQueue.empty())
            {
                return NULL;
            }

            IOOperation* op = NULL;

            auto closestPosIter = operationsQueue.rbegin();
            auto smallestTrackIter = operationsQueue.rbegin();

            long int minPosDist = INT_MAX;
            long int smallestTrack = INT_MAX;

            bool posFound = false;
            // bool negFound = false;

            for (auto iter = operationsQueue.rbegin(); iter != operationsQueue.rend(); iter++)
            {
                if ((*iter)->track >= current_head)
                {
                    posFound = true;
                    long int pDist = (*iter)->track - current_head;
                    if (pDist <= minPosDist)
                    {
                        minPosDist = pDist;
                        closestPosIter = iter;
                    }
                }

                if ((*iter)->track <= smallestTrack)
                {
                    smallestTrackIter = iter;
                    smallestTrack = (*iter)->track;
                }
            }

            if(posFound)
            {
                op = *closestPosIter;
                operationsQueue.erase(next(closestPosIter).base());
            }
            else
            {
                op = *smallestTrackIter;
                operationsQueue.erase(next(smallestTrackIter).base());
            }

            return op;
        }

        bool IsFinished()
        {
            return operationsQueue.empty();
        }

};

class FLOOKIOScheduler : public IOScheduler
{
    private:
        deque<IOOperation*> *operationsQueueAdd = new deque<IOOperation*>();
        deque<IOOperation*> *operationsQueueActive = new deque<IOOperation*>();

    public:
        void AddRequest(IOOperation* operation)
        {
            operationsQueueAdd->push_back(operation);
        }

        IOOperation* GetOperation(long int current_head, long int last_direction)
        {
            if (operationsQueueAdd->empty() && operationsQueueActive->empty())
            {
                return NULL;
            }

            if(operationsQueueActive->empty())
            {
                deque<IOOperation*> *tempOperationsQueue = operationsQueueActive;
                operationsQueueActive = operationsQueueAdd;
                operationsQueueAdd = tempOperationsQueue;
            }

            IOOperation* op = NULL;
            
            auto closestPosIter = operationsQueueActive->rbegin();
            auto closestNegIter = operationsQueueActive->rbegin();

            long int minPosDist = INT_MAX;
            long int minNegDist = INT_MAX;

            bool posFound = false;
            bool negFound = false;

            for (auto iter = operationsQueueActive->rbegin(); iter != operationsQueueActive->rend(); iter++)
            {
                if ((*iter)->track >= current_head)
                {
                    posFound = true;
                    long int pDist = (*iter)->track - current_head;
                    if (pDist <= minPosDist)
                    {
                        minPosDist = pDist;
                        closestPosIter = iter;
                    }
                }

                if ((*iter)->track <= current_head)
                {
                    negFound = true;
                    long int nDist =  current_head - (*iter)->track;
                    if (nDist <= minNegDist)
                    {
                        minNegDist = nDist;
                        closestNegIter = iter;
                    }
                }
            }

            if(last_direction > 0 && posFound)
            {
                op = *closestPosIter;
                operationsQueueActive->erase(next(closestPosIter).base());
            }
            else if(last_direction > 0 && !posFound)
            {
                op = *closestNegIter;
                operationsQueueActive->erase(next(closestNegIter).base());
            }
            if(last_direction < 0 && negFound)
            {
                op = *closestNegIter;
                operationsQueueActive->erase(next(closestNegIter).base());
            }
            else if(last_direction < 0 && !negFound)
            {
                op = *closestPosIter;
                operationsQueueActive->erase(next(closestPosIter).base());
            }

            return op;
        }

        bool IsFinished()
        {
            return (operationsQueueAdd->empty() && operationsQueueActive->empty());
        }

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

    long int i = 0;
    while (fin)
    {
        while(getline(fin, line))
        {
            line = trim(line);
            if(line[0] != '#')
            {
                sscanf(line.c_str(), "%ld %ld", &issueTime, &track);
                allOperations.push_back(IOOperation(issueTime, track,i));
                i++;
                break;
            }
        }
    }

    fin.close();
}

void Simulation()
{
    auto remainingOperation = allOperations.begin();
    IOOperation* currentOperation = NULL;
    long int currentHead = 0;
    long int currentTime = 0;
    bool isIOActive = false;
    long int direction = 0;
    long int lastDirection = 1;
    // cout << "Sim Start" << endl;
    while (true)
    {
        // if a new I/O arrived to the system at this current time
        if(remainingOperation != allOperations.end() && remainingOperation->arrival_time == currentTime)
        {
            // → add request to IO-queue
            scheduler->AddRequest(&(*remainingOperation));
            if(verboseOption)
            {
                printf("%d:\t%d add %d\n", currentTime, remainingOperation->id, remainingOperation->track);
            }
            // cout << endl;
            remainingOperation++;
        }
        // if an IO is active and completed at this time
        if (isIOActive && currentOperation->end_time == currentTime)
        {
            // → Compute relevant info and store in IO request for final summary if no IO request active now
            if(verboseOption)
            {
                printf("%d:\t%d finish %d\n", currentTime, currentOperation->id, (currentOperation->end_time - currentOperation->arrival_time));
            }
            // cout << endl;
            isIOActive = false;
            // currentOperation = NULL;
            // lastDirection = direction;
            // direction = 0;
        }
        // if requests are pending
        if(!isIOActive && !scheduler->IsFinished())
        {
            // → Fetch the next request from IO-queue and start the new IO.
            // IOOperation* op = scheduler->GetOperation();
            isIOActive = true;
            currentOperation = &(*scheduler->GetOperation(currentHead, lastDirection));
            currentOperation->waitTime = currentTime - currentOperation->arrival_time;
            currentOperation->start_time = currentTime;
            currentOperation->end_time = currentTime + abs(currentOperation->track - currentHead);

            if (currentOperation->track > currentHead)
            {
                direction = 1;
            }
            else if (currentOperation->track < currentHead)
            {
                direction = -1;
            }
            else
            {
                direction = 0;
                // isIOActive = false;
                // continue;
            }

            if (direction != 0)
            {
                lastDirection = direction;
            }
            
            if(verboseOption)
            {
                printf("%d:\t%d issue %d %d\n", currentTime, currentOperation->id, currentOperation->track, currentHead);
            }
            // cout << endl;
        }
        // else if all IO from input file processed
        else if(!isIOActive && (remainingOperation == allOperations.end()) && scheduler->IsFinished())
        {
            // → exit simulation
            break;
        }
        else if(!isIOActive)
        {
            direction = lastDirection;
        }
        
        // if an IO is active
        if(isIOActive)
        {
            // → Move the head by one unit in the direction its going (to simulate seek)
            currentHead += direction;
            if (direction != 0)
            {
                tot_movement++;
            }
        }
        // Increment time by 1
        // currentTime++;
        // total_time++;
        if (direction != 0)
        {
            currentTime++;
            total_time++;
            // cout << currentTime << " : " << total_time << endl;
        }

        // cout << "Sim time " << currentTime << endl;

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
    
    long int i = 0;
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

    avg_turnaround = (double)total_turn_around_time / i;
    avg_waittime = (double)total_wait_time / i;

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
