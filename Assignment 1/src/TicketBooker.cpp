/*
 *  Author(s): Cyrus Villacampa - a1709135
 *             Mohammad Idrees Rezai - a1687415
 *
 *  Operating Systems - COMP SCI 3004 - Assignment 1
 */

#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>

/* ================= Process struct ================= */
struct Process {
    long arrival;   // Initial arrival time of the process
    long totalTickets;  // Number of tickets to be processed
    long running = 0;   // Total time the process has ran
    long runningInQueueOne = 0; // Total time the process has ran in queue one
    long remainingTimeQuota = 0;    // Total time quota in queue two
    long waiting = 0;   // Total time the process has waited since it's first run
    int age = 0;    // Ageing mechanism used to process in queue two
    long end = -1;   // The time the process used up it's totalTickets
    int priority;   // Priority number of the process
    long ready = -1;     // The time it first ran
    std::string processIndex;   // Process ID
};

/* ================= Queue class ================= */
class Queue {
public:
    // Pop head of the queue
    Process* popHead();
    // Insert an element in the tail
    void tailInsert(Process*);
    // Return the head of the queue but does not remove it
    Process* getHead();
    // Remove the head of the queue
    void removeHead();
    // Check if the queue is empty
    bool isEmpty();
    // Print contents of it's queue
    void printQueue();
    // Increment waiting time of all process if a process has already ran
    void incrementWaiting(int);
protected:
    std::vector<Process*> queue;
};

/* ================= QueueOne class ================= */
class QueueOne : public Queue {
public:
    // Insert an element in queue one(sorted by priority then arrival time then process index)
    void insertProcess(Process*);
};

/* ================= QueueTwo class ================= */
class QueueTwo : public Queue {
public:
    // Insert an element in queue two(sorted by arrival time then process index)
    void insertProcess(Process*);
    // Age the processes
    void ageing(int);
    // Promote aged processes
    void promote(QueueOne*);
};

/* ================= HasNotArrived class ================= */
class HasNotArrived : public Queue {
public:
    // Insert an element in queue two(sorted by arrival time then process index)
    void insertProcess(Process*);
};

/* ================= Scheduling class ================= */
class Scheduling {
public:
    Scheduling(bool);
    static int const THRESHOLD = 2;
    int const QUEUE_ONE_TIME_QUOTA = 5;
    int const QUEUE_TWO_TIME_QUOTA = 20;
    int const NUM_RUNS = 25;
    int const AGE_THRESHOLD = 8;
    static long timer;
    // Start the scheduling algorithm
    void start();
    // Read the file and initialize all queues
    void initializeQueues(char*);
    // Print content of queues
    void printQueuesContent();
    // Check new arrival processes and insert into the correct queue
    void insertNewArrivalProcess();
    
private:
    QueueOne queue_one;
    QueueTwo queue_two;
    HasNotArrived hasNotArrived;
    std::vector<Process*> terminatedProcesses;
    bool debug_mode;
};

long Scheduling::timer = 0;

/* ====================== Main program =================== */
int main(int argc, char *argv[]) {
    u_int arg = 1;
    bool debug_mode = true;   // 0 to disable and 1 to enable debug mode
    
    if (argc < 2) {
        std::cout << "USAGE: ./TicketBooker.o [filename]\n" << "or\t" << "./TicketBooker.o [-d or --debug] [filename]" ;
        exit(EXIT_FAILURE);
    } else if (argc == 2) { // Check if a file name was provided
        debug_mode = false; 
    } else if (argc == 3) {  // Check if a file name was provided and debug mode was enabled
        if (strcmp(argv[1], "-d") != 0 && strcmp(argv[1], "--debug") != 0) {
            std::cout << "USAGE: ./TicketBooker.o [filename]\n" << "or\t" << "./TicketBooker.o [-d or --debug] [filename]" ;
            exit(EXIT_FAILURE);
        }
        arg = 2;
    }
    
    Scheduling ticketBooker(debug_mode); 

    ticketBooker.initializeQueues(argv[arg]);
    ticketBooker.start();
    ticketBooker.printQueuesContent();
}

/* ====================== Class member function implementations ================== */

/* 
 *  ================================================================================
 *  INPUT: char*(the name of the file)
 *  OUTPUT: NA
 *  DESCRIPTION: This function reads the given file and initializes the queues with
 *               the read contents of the file.
 *  ================================================================================
 */
void Scheduling::initializeQueues(char *filename) {
    // Open the file
    std::ifstream processFile (filename);
    
    // Check if file exist or it was successful in opening the file
    if (!processFile.is_open()) {
        std::cout << "Error opening file \"" << filename << "\"\n";
        exit(EXIT_FAILURE);
    }

    std::string line;
    std::string str;
    Process *proc;
    
    while (std::getline(processFile,line)) {
        int attr = 1;
        int i = 0;

        // Dynamically create process objects
        proc =  new Process;        

        // Extract process index
        while (line[i] != ' ') {
            str += line[i];
            i += 1;
        }

        // Store process index
        proc->processIndex = str;
        str = "";

        // Extract and store process attributes
        while (line[i] != '\0') {
            i += 1;
            if (line[i] != ' ' && line[i + 1] != '\0') {
                str += line[i];
            } else {
                switch (attr) {
                    case 1: // initial arrival time
                        proc->arrival = std::stol(str, nullptr);
                        break;
                    case 2: // priority
                        proc->priority = std::stoi(str, nullptr);
                        break;
                    case 3: // age
                        proc->age = std::stoi(str, nullptr);
                        break;
                    case 4: // total tickets
                        str += line[i];
                        proc->totalTickets = std::stol(str, nullptr);
                        break;
                    default:
                        break;
                }
                attr += 1;
                str = "";
            }
        }

        // Store process into the correct queue
        if (proc->arrival == 0) {
            if (proc->priority > THRESHOLD) {
                queue_one.insertProcess(proc);
            } else {
                proc->remainingTimeQuota = QUEUE_TWO_TIME_QUOTA;
                queue_two.insertProcess(proc);
            }
        } else {
            hasNotArrived.insertProcess(proc);
        }
    }

    // Close the file
    processFile.close();
}

/* 
 *  ================================================================================
 *  INPUT: NA
 *  OUTPUT: NA
 *  DESCRIPTION: This function is the implementation of the scheduling algorithm as
 *               specified in the assignment handouts.
 *  ================================================================================
 */
void Scheduling::start() {
    while (!queue_one.isEmpty() || !queue_two.isEmpty() || !hasNotArrived.isEmpty()) {
        int processTime = 0;
        Process *inQueueProcess, *newArrivalProcess;
        while (!queue_one.isEmpty()) {  // Process queue_one first
            processTime = 0;
            inQueueProcess = queue_one.popHead();   // Process the highest priority first
            
            if (inQueueProcess->ready == -1) {
                inQueueProcess->ready = timer;
            }

            while (inQueueProcess->totalTickets > 0 && processTime < QUEUE_ONE_TIME_QUOTA) {
                timer += 1;
                inQueueProcess->totalTickets -= 1;
                processTime += 1;
    /* ============================ INSERT NEW ARRIVAL PROCESS FIRST(START) ========================= */
                insertNewArrivalProcess();
    /* ============================ INSERT NEW ARRIVAL PROCESS FIRST(END) =========================== */
            }
            inQueueProcess->running += processTime;
            inQueueProcess->runningInQueueOne += processTime;
            // Increment waiting time of all the processes in both queues if a process has already ran
            queue_one.incrementWaiting(processTime);
            queue_two.incrementWaiting(processTime);
            queue_two.ageing(8);    // Age the processes in queue_two
    /* ============================ INSERT PRE-EMPTED PROCESS SECOND(START) ========================= */
            if (inQueueProcess->totalTickets != 0) {
                if (inQueueProcess->runningInQueueOne == NUM_RUNS) {
                    inQueueProcess->priority -= 1;  // Decrease priority
                    inQueueProcess->runningInQueueOne = 0;  // Reset counter for total time running in queue_one
                    if (inQueueProcess->priority <= THRESHOLD) {    // Demote process
                        inQueueProcess->remainingTimeQuota = QUEUE_TWO_TIME_QUOTA;  // Initialize time quota
                        queue_two.tailInsert(inQueueProcess);
                    } else {
                        queue_one.insertProcess(inQueueProcess); // Insert back into queue_one
                    }
                } else {
                    queue_one.insertProcess(inQueueProcess); // Insert back into queue_one
                }
            } else {    // Pre-empted process has finished its job
                inQueueProcess->end = timer;
                terminatedProcesses.insert(terminatedProcesses.end(), inQueueProcess);
            }
    /* ============================ INSERT PRE-EMPTED PROCESS SECOND(END) ========================= */
    /* ============================ INSERT PROMOTED PROCESS THIRD(START) ========================= */
            queue_two.promote(&queue_one);    // Promote processes in queue_two
    /* ============================ INSERT PROMOTED PROCESS THIRD(END) =========================== */

            /* ====== DEBUGGING PURPOSES ====== */
            if (debug_mode) {
                if (timer % 5 == 0) {
                    std::cout << "Time (" << timer << ")\n";
                    printQueuesContent();
                }
            }
        }

        while (!queue_two.isEmpty() && queue_one.isEmpty()) { // Process queue_two
            processTime = 0;
            inQueueProcess = queue_two.popHead();
            if (inQueueProcess->ready == -1) {   // Store the time this process first ran
                inQueueProcess->ready = timer;
            }
            inQueueProcess->age = 0;    // Reset age
            while (inQueueProcess->totalTickets > 0 && inQueueProcess->remainingTimeQuota > 0 && queue_one.isEmpty()) {
                timer += 1;
                processTime += 1;
                inQueueProcess->totalTickets -= 1;
                inQueueProcess->remainingTimeQuota -= 1;
    /* ============================ INSERT NEW ARRIVAL PROCESS FIRST(START) ========================= */
                insertNewArrivalProcess();
    /* ============================ INSERT NEW ARRIVAL PROCESS FIRST(END) =========================== */
            } 
            inQueueProcess->running += processTime;
            queue_two.incrementWaiting(processTime);
            queue_two.ageing(AGE_THRESHOLD);    // Age the processes in queue_two
    /* ============================ INSERT PRE-EMPTED PROCESS SECOND(START) ========================= */
            if (inQueueProcess->totalTickets != 0) {
                if (inQueueProcess->remainingTimeQuota == 0) {  // No new process arrived in queue_one
                    inQueueProcess->remainingTimeQuota = QUEUE_TWO_TIME_QUOTA;
                }
                queue_two.tailInsert(inQueueProcess);
            } else {
                inQueueProcess->end = timer;
                terminatedProcesses.insert(terminatedProcesses.end(), inQueueProcess);
            }
    /* ============================ INSERT PRE-EMPTED PROCESS SECOND(END) ========================== */
    /* ============================ INSERT PROMOTED PROCESS THIRD(START) ========================= */
            queue_two.promote(&queue_one);    // Promote processes from queue_two to queue_one
    /* ============================ INSERT PROMOTED PROCESS THIRD(END) =========================== */
            
            /* ====== DEBUGGING PURPOSES ====== */
            if (debug_mode) {
                std::cout << "Time (" << timer << ")\n"; 
                printQueuesContent();
            }
        }  

        if (queue_one.isEmpty() && queue_two.isEmpty() && !hasNotArrived.isEmpty()) {   // Fast forward time
            newArrivalProcess = hasNotArrived.getHead();
            timer = newArrivalProcess->arrival;
            do {
                if (newArrivalProcess->priority > THRESHOLD) {  // Insert into queue_one
                    queue_one.insertProcess(newArrivalProcess);
                } else {    // Insert into queue_two
                    queue_two.insertProcess(newArrivalProcess);
                }
                hasNotArrived.removeHead();
                newArrivalProcess = hasNotArrived.getHead();
            } while (newArrivalProcess->arrival == timer);  // Check if there are still process that has arrived

        }
    }
}

/* 
 *  ================================================================================
 *  INPUT: NA
 *  OUTPUT: NA
 *  DESCRIPTION: This function checks if there are processes has arrived and calls
 *               a function to insert those processes in the appropriate queue.
 *  ================================================================================
 */
void Scheduling::insertNewArrivalProcess() {
    Process *newProcess;

    while (!hasNotArrived.isEmpty()) {
        newProcess = hasNotArrived.getHead();
        if (newProcess->arrival == timer) {   // Check if there are processes that has arrived
            if (newProcess->priority > THRESHOLD) {  // Insert into queue_one
                queue_one.insertProcess(newProcess);
            } else {    // Insert into queue_two
                newProcess->remainingTimeQuota = QUEUE_TWO_TIME_QUOTA;
                queue_two.insertProcess(newProcess);
            }
            hasNotArrived.removeHead();
        } else {
            break;
        }
    }
}

// Constructor
Scheduling::Scheduling(bool debug_mode) {
    this->debug_mode = debug_mode;
}

void Scheduling::printQueuesContent() {
    std::vector<Process*>::iterator it;
    if (debug_mode) {
        std::cout << "===========================================\n";
        std::cout << "Queue 1\n";
        std::cout << "name" << "\t\t" << "arrival" << "\t\t" << "end" << "\t\t" << "ready" << "\t\t" << "running" << "\t\t" << "waiting" << "\t\t" << "age" << "\t\t" << "priority" << '\n';
        queue_one.printQueue();
        std::cout << "===========================================\n";
        std::cout << "Queue 2\n";
        std::cout << "name" << "\t\t" << "arrival" << "\t\t" << "end" << "\t\t" << "ready" << "\t\t" << "running" << "\t\t" << "waiting" << "\t\t" << "age" << "\t\t" << "priority" << '\n';
        queue_two.printQueue();
        std::cout << "===========================================\n";
        std::cout << "HasNotArrived\n";
        std::cout << "name" << "\t\t" << "arrival" << "\t\t" << "end" << "\t\t" << "ready" << "\t\t" << "running" << "\t\t" << "waiting" << "\t\t" << "age" << "\t\t" << "priority" << '\n';
        hasNotArrived.printQueue();
        std::cout << "===========================================\n";
        std::cout << "Terminated Processes\n";
        std::cout << "name" << "\t\t" << "arrival" << "\t\t" << "end" << "\t\t" << "ready" << "\t\t" << "running" << "\t\t" << "waiting" << '\n';
        for (it = terminatedProcesses.begin(); it < terminatedProcesses.end(); it++) {
            std::cout << (*it)->processIndex << "\t\t" << (*it)->arrival << "\t\t" << (*it)->end << "\t\t" << (*it)->ready << "\t\t" << (*it)->running << "\t\t" << (*it)->waiting << "\t\t" << '\n';
        }
        std::cout << "===========================================\n";
    } else {
        std::cout << "name" << "\t\t" << "arrival" << "\t\t" << "end" << "\t\t" << "ready" << "\t\t" << "running" << "\t\t" << "waiting" << '\n';
        for (it = terminatedProcesses.begin(); it < terminatedProcesses.end(); it++) {
            std::cout << (*it)->processIndex << "\t\t" << (*it)->arrival << "\t\t" << (*it)->end << "\t\t" << (*it)->ready << "\t\t" << (*it)->running << "\t\t" << (*it)->waiting << "\t\t" << '\n';
        }
    }
        
}

/* 
 *  ================================================================================
 *  INPUT: Process*(pointer to a process object)
 *  OUTPUT: NA
 *  DESCRIPTION: This function insert a process into queue one based on the priority.
 *  ================================================================================
 */
void QueueOne::insertProcess(Process *proc) {
    u_int i = 0;
    if (queue.size() > 0) {   // Check if the queue is not empty
        while (i < queue.size() && queue[i]->priority >= proc->priority) {    // Find insert position
            i += 1;
        }
    }

    if (i == 0) {   // Process has highest priority
        queue.insert(queue.begin(), proc);
    } else if (i == queue.size()) {   // Process has lowest priority
        queue.insert(queue.end(), proc);
    } else {
        queue.insert(queue.begin() + i, proc);
    }
}

/* 
 *  ================================================================================
 *  INPUT: Process*(pointer to a process object)
 *  OUTPUT: NA
 *  DESCRIPTION: This function insert a process into queue two based on the arrival
 *               time.
 *  ================================================================================
 */
void QueueTwo::insertProcess(Process *proc) {
    u_int i = 0;
    if (queue.size() > 0) {   // Check if the queue is not empty
        while (i < queue.size() && queue[i]->arrival <= proc->arrival) {    // Find insert position
            i += 1;
        }
    }

    if (i == 0) {   // Process has earliest arrival time
        queue.insert(queue.begin(), proc);
    } else if (i == queue.size()) {   // Process has latest arrival time
        queue.insert(queue.end(), proc);
    } else {
        queue.insert(queue.begin() + i, proc);
    }
}

/* 
 *  ================================================================================
 *  INPUT: integer
 *  OUTPUT: NA
 *  DESCRIPTION: This function ages the processes in a queue by incrementing age of
 *               each process in the queue. The function also increases the process'
 *               priority if their age has reached a specific limit.
 *  ================================================================================
 */
void QueueTwo::ageing(int ageLimit) {
    u_int i = 0;
    if (queue.size() > 0) {
        while (i < queue.size()) {
            if (queue[i]->arrival != Scheduling::timer) {
                queue[i]->age += 1;
                if (queue[i]->age == ageLimit) {
                    queue[i]->priority += 1;
                    queue[i]->age = 0;
                }
            }
            i += 1;
        }
    }
}

/* 
 *  ================================================================================
 *  INPUT: QueueOne*(pointer to a higher priority queue)
 *  OUTPUT: NA
 *  DESCRIPTION: This function promotes(i.e. transfer a process from a lower priority
 *               queue to a higher priority queue) a process in a queue if their
 *               priority is greater than the threshold.
 *  ================================================================================
 */
void QueueTwo::promote(QueueOne *q1) {
    u_int i = 0;
    if (queue.size() > 0) {
        while (i < queue.size()) {
            if (queue[i]->priority > Scheduling::THRESHOLD) {
                (*q1).insertProcess(queue[i]);
                queue.erase(queue.begin() + i);
                i -= 1;
            }
            i += 1;
        }
    }
}

/* 
 *  ================================================================================
 *  INPUT: Process*(pointer to a process object)
 *  OUTPUT: NA
 *  DESCRIPTION: This function insert a process into the queue based on the arrival
 *               time.
 *  ================================================================================
 */
void HasNotArrived::insertProcess(Process *proc) {
    u_int i = 0;
    if (queue.size() > 0) {   // Check if the queue is not empty
        while (i < queue.size() && queue[i]->arrival <= proc->arrival) {    // Find insert position
            if (queue[i]->arrival == proc->arrival) {
                if ((queue[i]->processIndex).compare(proc->processIndex) > 0) {
                    break;
                }
            }
            i += 1;
        }
    }

    if (i == 0) {   // Process has highest priority
        queue.insert(queue.begin(), proc);
    } else if (i == queue.size()) {   // Process has lowest priority
        queue.insert(queue.end(), proc);
    } else {
        queue.insert(queue.begin() + i, proc);
    }
}

/* 
 *  ================================================================================
 *  INPUT: NA
 *  OUTPUT: NA
 *  DESCRIPTION: Prints out the content of the queue
 *  ================================================================================
 */
void Queue::printQueue() {
    std::vector<Process*>::iterator it;
    for (it = queue.begin(); it < queue.end(); it++) {
        std::cout << (*it)->processIndex << "\t\t" << (*it)->arrival << "\t\t" << (*it)->end << "\t\t" << (*it)->ready << "\t\t" << (*it)->running << "\t\t" << (*it)->waiting << "\t\t" << (*it)->age << "\t\t" << (*it)->priority << '\n';
    }
}

bool Queue::isEmpty() {
    bool empty = true;
    if (queue.size() > 0) {
        empty = false;
    }
    return empty;
}

Process* Queue::getHead() {
    Process *proc = queue.front();
    return proc;
}

void Queue::removeHead() {
    queue.erase(queue.begin());
}

Process* Queue::popHead() {
    Process *proc = queue.front();
    queue.erase(queue.begin());
    return proc;
}

void Queue::tailInsert(Process *proc) {
    queue.insert(queue.end(), proc);
}

/* 
 *  ================================================================================
 *  INPUT: integer
 *  OUTPUT: NA
 *  DESCRIPTION: This function increments the waiting time of the process of the
 *               queue.
 *  ================================================================================
 */
void Queue::incrementWaiting(int time) {
    std::vector<Process*>::iterator it;
    for (it = queue.begin(); it < queue.end(); it += 1) {
        if ((*it)->ready != -1) {
            (*it)->waiting += time;
        }
    }
}