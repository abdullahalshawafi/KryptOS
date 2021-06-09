#include "headers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void clearResources(int);

void FCFS();
void SJF();
void HPF();
void SRTN();
void RR(int);

int msgq_id_PrcSch;
int shmid;
int *remainingTime;
int Ready_NUm_processes = 0;

Queue *Ready_queue;
Process addedProcess;
Process runningProcess;
PCB *PCB_LIST;

/// BUFFERS
Message message;
Message process_msg;

// variables used to write in the files
FILE *prefFile, *logFile;
float CPU_U, WTA, WTA_total;
int WT, WT_total;

// need to compute them
int sys_start_time;
int total_exec_time;

// data structures used for algorithms
int DS_Queue = 1;
int DS_PrioirtyQ = 2;
int myUsedDS;

///----------------------------------------------- MAIN --------------------------------------------------------
int main(int argc, char *argv[])
{
    initClk();
    signal(SIGINT, clearResources);

    sys_start_time = getClk();

    processesNum = atoi(argv[1]);
    schedulingAlgorithm = atoi(argv[2]);
    if (schedulingAlgorithm == 5)
        quantum = atoi(argv[3]);

    PCB_LIST = (PCB *)malloc((processesNum + 1) * sizeof(PCB));
    PCB NullPCBItem;
    NullPCBItem.runtime = 0;
    NullPCBItem.waiting_time = 0;
    PCB_LIST[0] = NullPCBItem;

    // msg queue to talk to process generator
    msgq_id_GenSch = initMsgq(msgq_genSchKey);

    // create shared memory to detect the finished state of the scheduler
    finished = (int *)initShm(finishedKey, &shmFinishedId);

    // create shared memory to detect the remaining time
    shm_remainingTime = (int *)initShm(remainKey, &shm_remainingTime_ID);

    // initializing the ready queue
    Ready_queue = (Queue *)malloc(sizeof(Queue));
    initialize(Ready_queue);

    logFile = fopen("Scheduler.log", "w");
    fprintf(logFile, "#At time x process y state arr w total z remain y wait k\n");

    switch (schedulingAlgorithm)
    {
    case 1:
        FCFS();
        break;
    case 2:
        SJF();
        break;
    case 3:
        HPF();
        break;
    case 4:
        SRTN();
        break;
    case 5:
        RR(quantum);
        break;
    }

    fclose(logFile);

    // prefFile = fopen("Scheduler.pref", "w");
    // CPU_U = (getClk() - sys_start_time) / total_exec_time;
    // fprintf(prefFile, "A CPU utilization = %.0f%%\n", CPU_U);
    // fprintf(prefFile, "Avg WTA = %.2f\n", WT_total / processesNum);
    // fprintf(prefFile, "Avg Waiting = %.2f\n", WT_total / processesNum);
    // fclose(prefFile);

    //TODO: upon termination release the clock resources. ?????????????

    shmctl(shm_remainingTime_ID, IPC_RMID, NULL);
    destroyClk(true);
    *finished = 1;
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
    shmctl(shm_remainingTime_ID, IPC_RMID, NULL);
    exit(0);
}

int startProcess(Process turnProcess)
{
    int pid = fork();

    if (pid == -1)
        perror("error in fork");

    else if (pid == 0) // running process
    {
        execl("./process.out", "./process.out", NULL);
        exit(0);
    }

    fprintf(logFile, "At time %d process %d started arr %d total %d remain %d wait %d\n",
            getClk(),
            addedProcess.id,
            addedProcess.arrival_time,
            addedProcess.runtime,
            addedProcess.runtime,
            getClk() - addedProcess.arrival_time);

    return pid; // return pid to know which child process will be terminated due to an algorithm
}

void checkRecievedProcess()
{
    int addedId;

    // from process generator
    addedProcess = receiveMsg(msgq_id_GenSch);
    addedId = addedProcess.id;

    if (myUsedDS == DS_Queue)
    {
        enqueue(Ready_queue, addedProcess);
        Queue_length = 0;
    }

    else if (myUsedDS == DS_PrioirtyQ)
    {
        enqueue_priority(addedProcess, addedProcess.priority);
        printf("priority enqueue id: %d\n", addedProcess.id);
        printf("HPF  id: %d\n", HPF_Queue[peek_priority()].myProcess.id);
    }

    printf("received process: %d\n", addedId);

    PCB_LIST[addedId].arrival_time = addedProcess.arrival_time;
    PCB_LIST[addedId].priority = addedProcess.priority;
    //PCB_LIST[addedId].process_id=addedProcess.process_id;
    PCB_LIST[addedId].id = addedProcess.id;
    PCB_LIST[addedId].runtime = addedProcess.runtime;

    Ready_NUm_processes++;
}

void stopProcess(int turn)
{
    // *shm_remainingTime = -1;
    int id = PCB_LIST[turn].process_id;
    PCB_LIST[turn].state = waiting;
    kill(id, SIGSTOP); //stopping it to be completed later
}

void resumeProcess(int turn)
{
    // *shm_remainingTime=PCB_LIST[turn].remainingtime;
    int id = PCB_LIST[turn].process_id;
    PCB_LIST[turn].state = running;
    kill(id, SIGCONT); //continue the stopped process
}

void sharedMemory_func(int RW, int remainTime)
{
    if (*remainingTime == -1)
    {
        perror("Scheduler: Error in attach in writer/reader");
        exit(-1);
    }
    // read rexecution  time from the shared memory
    if (RW == 1)
    {
        total_exec_time += atoi((char *)remainingTime);
    }
    // write remaining time to the shared memory
    else
    {
        char buffer[10];
        sprintf(buffer, "%d", remainTime);
        strcpy((char *)remainingTime, buffer);
    }
    //TODO: not sure : destroy
    shmdt(remainingTime);
}

void finishProcess(Process runningProcess)
{
    printf("finishing process: %d\n", runningProcess.id);

    WTA = (getClk() - runningProcess.arrival_time) * 1.0 / runningProcess.runtime;
    WTA_total += WTA;

    WT = getClk() - runningProcess.arrival_time - runningProcess.runtime;
    WT_total += WT;

    fprintf(logFile, "At time %d process %d finished arr %d total %d remain 0 wait %d TA %d WTA %.2f\n",
            getClk(),
            runningProcess.id,
            runningProcess.arrival_time,
            runningProcess.runtime,
            WT,
            getClk() - runningProcess.arrival_time,
            WTA);

    *shm_remainingTime = -1;
}

void finshed_processe(int ID)
{
    printf("in finish: \n");

    printf("process %d finished: \n", ID);

    // TA = finish - arr (total life time)
    WTA = (getClk() - PCB_LIST[ID].arrival_time) / PCB_LIST[ID].runtime;
    WTA_total += WTA;
    // finsh getclk -  arr - run
    WT = getClk() - PCB_LIST[ID].arrival_time - PCB_LIST[ID].runtime;
    WT_total += WT;

    printf("At time %d\t process %d\t finished arr %d\t total %d\t remain %d\t wait %d\n",
           getClk(), PCB_LIST[ID].id, PCB_LIST[ID].arrival_time, PCB_LIST[ID].runtime,
           0, PCB_LIST[ID].waiting_time);

    //TODO:free (PCB_LIST[id]);
}

void FCFS()
{
    printf("In FCFS\n");
    Process runningProcess;
    *shm_remainingTime = -1;
    myUsedDS = DS_Queue; // I'll use a normal queue

    while (Ready_NUm_processes < processesNum)
    {
        checkRecievedProcess();

        if (*shm_remainingTime == 0)
            finishProcess(runningProcess);
        else if (!isEmpty(Ready_queue) && *shm_remainingTime == -1)
        {
            dequeue(Ready_queue, &runningProcess);
            *shm_remainingTime = runningProcess.runtime + 1;
            runningProcess.process_id = startProcess(runningProcess);
        }
    }
}

void SJF() {}
//-------------------------- Highest Priority First --------------------------
void HPF()
{
}

void SRTN() {}

//------------------------------------------ For Round Robin -------------------------------------------

void RR(int quantum)
{

    myUsedDS = 1;
    int turn;
    int quantum_check = quantum;
    Process process_turn;
    int theEnd = 0;
    int lastClk;

    while ((Queue_length == -1 || Ready_queue->count > 0) && theEnd == 0)
    {

        printf("Clock now is %d", getClk());
        // check if i received any new process then add it to my PCB

        checkRecievedProcess();
        quantum_check--; //in each iteration receive new processes and keep running the current process for a quantum

        if (*shm_remainingTime == 0)
        {
            finshed_processe(runningProcess.id);
            quantum_check = quantum;
        }

        if (Queue_length == -1)
            continue;

        if (Ready_queue->count == 1)
        {
            runningProcess = addedProcess;
            turn = runningProcess.id;
        }

        printf("Quantum till now =%d\n", quantum_check);

        // printf("Turn is %d\n",turn);
        // printf("PCB[turn] %d\n",PCB_LIST[turn].id);

        if (quantum_check == 0)
        {
            // --------------- saving data before stopping ----------------------

            // --------------------------------------------------------------------------------------
            //   printf("Stopped Process with id= %d and remaining time=%d\n",PCB_LIST[turn].id,*shm_remainingTime);

            enqueue(Ready_queue, runningProcess);
            quantum_check = quantum;
            stopProcess(turn);
        }

        else if (PCB_LIST[turn].state == waiting && quantum_check == quantum)
        { //has started before

            // ------------------ Get data before resuming ---------------------------------------

            // ------------------------------------------------------------------------------------

            //    printf("Process will continue working, with id= %d and has remaining time=%d\n",PCB_LIST[turn].id,PCB_LIST[turn].remainingtime);

            resumeProcess(PCB_LIST[turn].process_id);
        }

        else
        { // first time
            int process_id = startProcess(runningProcess);

            if (process_id != -1)
            { //no errors occur during sending data of the process

                printf("First visit\n");
                PCB_LIST[turn].process_id = process_id;

                // ------------------ Saving the first data for the process -------------------------

                //----------------------------------------------------------------------------------

                if (Ready_queue->front == NULL)
                {
                    theEnd = 1;
                }
                else
                {
                    dequeue(Ready_queue, &process_turn);
                    runningProcess = process_turn;
                    turn = runningProcess.id;
                }
            }
        }

        bool flag = false;
        while (!flag)
        {
            int newClk = getClk();
            if (newClk != lastClk)
            {
                flag = true;
                // printf("Process with id= %d has remaining time=%d after 1 clock cycle\n",PCB_LIST[turn].id,PCB_LIST[turn].remainingtime);
            }
            lastClk = newClk;
        }
    }
}