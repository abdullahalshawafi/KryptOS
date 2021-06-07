#include <stdio.h> //if you don't use scanf/printf change this include
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

///==============================
//don't mess with this variable//
int *shmaddr; //
//===============================

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
   running, waiting
};
// struct for process contains its parameters
struct process
{
    int id, //this id read from the input file
    process_id, // this the actual id in the system 
	 arrival_time, 
	 runtime, 
	 priority;
};

// ----------------------------------- general variables declerations ---------------------------------------
int schedulingAlgorithm, quantum = -1;
int msgq_id_GenSch;
int rec_process;
int processesNum = 0; // no.of el processes we read from the input file
//int Current_processesNum = 0; // no.of el processes remaining (not finished)
int processesNum_sent_toSCH=0; //no.of el processes sent to el scheduler (to stop process generator loop)


// dol msh 3arfa les hyt7sbo fen w ezay but we need them
int actual_processing_time ;// total of all the processes to compute CPU utilization
int totalelapsedtime;

  
 
  
  
 
  
////------------- DATA STRUCTURES USED IN SCHEDULING ALGORITHMS --------------------------------------------//////////////////////
///// ---------------- --------------Normal queue --------------------
struct Node{
	struct process myprocess;
	struct Node *next;
};

typedef struct Node Node;

int Queue_length=-1;

struct Queue{
    Node *front;
    Node *rear;  
};


typedef struct Queue Queue;


void enqueue(Queue *myqueue, struct process new_process)
	{
		Node *newnode;
		newnode->myprocess = new_process;
		if (Queue_length ==0)
		{
			myqueue->front = newnode;
			newnode->next = NULL;
			myqueue->rear=myqueue->front;
		}
		else
		{
			newnode->next = NULL;
			myqueue->rear->next = newnode;
			myqueue->rear = newnode;
		}
        Queue_length++;
	}

void dequeue(Queue *myqueue)
	{
		if (Queue_length==0)
		{
			return;
		}
		else
		{
			Node *temp = myqueue->front;
			if (myqueue->front == myqueue->rear)
				myqueue->front = myqueue->rear = NULL;
			else
			{
				myqueue->front = myqueue->front->next;
				temp->next = NULL;
				free(temp);
			}
			Queue_length--;
		}
	}

// bool isEmpty(Queue *q)
//     {
//         return (q->length ==0); 
//     }
// ------------------------------------------------------------------priority queue-----------------------------------
struct item {
 struct process myProcess;
    int priority;
};

// Store the element of a priority queue
struct  item HPF_Queue[100000];
  
// Pointer to the last index
int HPF_Queue_size = -1;
  
// Function to insert a new element
// into priority queue
void enqueue_priority(struct process process, int priority)
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
  
    // Check for the element with
    // highest priority
    for (int i = 0; i <= HPF_Queue_size; i++) {
  
        // If priority is same choose
        // the element with the
        // highest value
        if (highestPriority ==  HPF_Queue[i].priority && ind > -1) 
        {
            highestPriority = HPF_Queue[i].priority;
            ind = i;
        }
        else if (highestPriority >  HPF_Queue[i].priority) 
        {  
            highestPriority = HPF_Queue[i].priority;
            ind = i;
        }
    }
  
    // Return position of the element
    return ind;
}
  
// Function to remove the element with
// the highest priority
void dequeue_priority()
{
    // Find the position of the element
    // with highest priority
    int ind = peek_priority();
  
    // Shift the element one index before
    // from the postion of the element
    // with highest priortity is found
    for (int i = ind; i < HPF_Queue_size; i++) {
        HPF_Queue[i] = HPF_Queue[i + 1];
    }
     // Decrease the size of the
    // priority queue by one
    HPF_Queue_size--;
}
	