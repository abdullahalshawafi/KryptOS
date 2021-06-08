#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

typedef short bool;
#define true 1
#define false 0

#define SHKEY 300

//===============================//
// don't mess with this variable
int *shmaddr;
char msgq_genSchKey = 'A';
char msgq_prcSchKey = 'B';
char shmKey = 'C';
//===============================//

int getClk()
{
    return *shmaddr;
}

/*
 * All processes call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
 */
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        //Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *)shmat(shmid, (void *)0, 0);
}

int initMsgq(int key)
{
    key_t msgqKey = ftok("keyfile", key);
    int msgqId = msgget(msgqKey, 0666 | IPC_CREAT);
    if (msgqId == -1)
    {
        perror("Error in creating the msg queue");
        exit(-1);
    }

    return msgqId;
}

void *initShm(char key, int *id)
{
    key_t shmKey = ftok("keyfile", key);
    int shmId = shmget(shmKey, 4096, 0666 | IPC_CREAT);
    *id = shmId;

    if (!~shmId)
    {
        perror("Error in creating of message queue");
        exit(-1);
    }

    void *addr = shmat(shmId, (void *)0, 0);
    return addr;
}

/*
 * All processes call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
 */
void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}
// Each process have a state of only two state running or blocked
enum STATE
{
    running,
    waiting
};
// struct for process contains its parameters
struct Process
{
    int id,         //this id read from the input file
        process_id, // this the actual id in the system
        arrival_time,
        runtime,
        priority;
};
typedef struct Process Process;

// Buffer struct for process generator to scheduler msg queue
struct Message
{
    int mtype;
    Process NewProcess; // process that will be sent from process generator to scheduler
};
typedef struct Message Message;

// Process can receive the changes in its state in a message queue
// Or send timeTaken to the scheduler
struct processSchedulermsgbuff
{
    long mtype;
    int timeTaken;
    Process running_process;
};
typedef struct processSchedulermsgbuff processSchedulermsgbuff;

//
struct PCB
{
    int id,         // this id read from the input file
        process_id, // this the actual id in the system
        arrival_time,
        runtime,
        priority,
        remainingtime,
        startingTime,
        stopped_time,
        resume_time,
        waiting_time;
    enum STATE state;
};
typedef struct PCB PCB;

////------------- general variables declerations -------------////
int schedulingAlgorithm, quantum = -1;
int msgq_id_GenSch;
int rec_process;
int processesNum = 0; // no.of the processes read from the input file
//int Current_processesNum = 0; // no.of the processes remaining (not finished)
int processesNum_sent_toSCH = 0; //no.of the processes sent to the scheduler

// dol msh 3arfa les hyt7sbo fen w ezay but we need them
int actual_processing_time; // total of all the processes to compute CPU utilization
int totalelapsedtime;

////------------- DATA STRUCTURES USED IN SCHEDULING ALGORITHMS -------------////

////------------- Normal Queue -------------////
struct Node
{
    Process myprocess;
    struct Node *next;
};
typedef struct Node Node;

int Queue_length = -1;

struct Queue
{
    Node *front;
    Node *rear;
};
typedef struct Queue Queue;

void initialize(Queue *queue)
{
    queue->front = NULL;
    queue->rear = NULL;
}

void enqueue(Queue *myqueue, Process new_process)
{
    Node *newnode = (Node *)malloc(sizeof(Node));
    newnode->myprocess = new_process;
    newnode->next = NULL;
    if (Queue_length == -1)
    {
        myqueue->front = newnode;
        myqueue->rear = myqueue->front;
        Queue_length += 2;
    }
    else
    {
        myqueue->rear->next = newnode;
        myqueue->rear = newnode;
        Queue_length++;
    }
}

Process dequeue(Queue *myqueue)
{
    if (Queue_length == -1)
        return;
    Node *temp = myqueue->front;
    Process deleted = temp->myprocess;
    if (myqueue->front == myqueue->rear)
        myqueue->front = myqueue->rear = NULL;
    else
    {
        myqueue->front = myqueue->front->next;
        temp->next = NULL;
        free(temp);
    }
    Queue_length--;

    return deleted;
}

// bool isEmpty(Queue *q)
//     {
//         return (q->length ==0);
//     }

////------------- priority Queue -------------////
struct item
{
    Process myProcess;
    int priority;
};

// Store the element of a priority queue
struct item HPF_Queue[100000];

// Pointer to the last index
int HPF_Queue_size = -1;

// Function to insert a new element into priority queue
void enqueue_priority(Process process, int priority)
{
    // Increase the size
    HPF_Queue_size++;

    // Insert the element
    HPF_Queue[HPF_Queue_size].myProcess = process;
    HPF_Queue[HPF_Queue_size].priority = priority;
}

// Function to return index with highest priority
int peek_priority()
{
    //NOTE:**** el highest priority the least value of priority
    int highestPriority = 2147483648; //INT_MAX
    int ind = -1;

    // Check for the element with highest priority
    for (int i = 0; i <= HPF_Queue_size; i++)
    {
        // If priority is same choose
        // the element with the
        // highest value
        if (highestPriority == HPF_Queue[i].priority && ind > -1)
        {
            highestPriority = HPF_Queue[i].priority;
            ind = i;
        }
        else if (highestPriority >= HPF_Queue[i].priority)
        {
            highestPriority = HPF_Queue[i].priority;
            ind = i;
        }
    }

    // Return position of the element
    return ind;
}

// Function to remove the element with the highest priority
void dequeue_priority()
{
    // Find the position of the element
    // with highest priority
    int ind = peek_priority();

    // Shift the element one index before
    // from the postion of the element
    // with highest priortity is found
    for (int i = ind; i < HPF_Queue_size; i++)
    {
        HPF_Queue[i] = HPF_Queue[i + 1];
    }
    // Decrease the size of the
    // priority queue by one
    HPF_Queue_size--;
}
