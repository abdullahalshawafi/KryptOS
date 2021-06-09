#include "headers.h"

void clearResources(int);

void FCFS();
void SJF();
void HPF();
void SRTN();
void RR(int);

int msgq_id_PrcSch;
int shmid;
int *remainingTime;
Queue *Ready_queue;
Process addedProcess;
PCB *PCB_LIST;
int Ready_NUm_processes = 0;

/// BUFFERS
Message message;
Message process_msg;

// variables used to write in the files
FILE *pFile;
float CPU_U, WT, WTA, WTA_total, WT_total;

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

    PCB_LIST = (PCB *)malloc(processesNum * sizeof(PCB));
    //pFile = fopen("Scheduler.log", "w");
    //fclose(pFile); // close the file emta msh 3arfa rabna ysahl

    ///----------------------------------- Message queues initializations------------------------

    // msg queue to talk to process generator
    msgq_id_GenSch = initMsgq(msgq_genSchKey);

    // msg queue to talk to process file
    msgq_id_PrcSch = initMsgq(msgq_prcSchKey);

    // create shared memory to write /read   remaining time / execuation time in it =>> Process.c
    remainingTime = (int *)initShm(shmKey, &shmid);

    Ready_queue = (Queue *)malloc(sizeof(Queue));
    initialize(Ready_queue);

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

    // fclose(pFile); // close the file emta msh 3arfa rabna ysahl

    // pFile = fopen("Scheduler.pref", "w");
    // CPU_U = (getClk() - sys_start_time) / total_exec_time;
    // fprintf(pFile, "A CPU utilization = %f\t \n", CPU_U);
    // fprintf(pFile, "Avg WTA = %f\t \n", WT_total / processesNum);
    // fprintf(pFile, "Avg Waiting = %f\t \n", WT_total / processesNum);
    // fclose(pFile);
    //TODO: upon termination release the clock resources. ?????????????

    shmctl(shmid, IPC_RMID, NULL);
    destroyClk(true);
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
    shmctl(shmid, IPC_RMID, NULL);
    exit(0);
}

int ProcessExecution()
{
    int pid = fork();

    if (pid == -1)
        perror("error in fork");

    else if (pid == 0) // running process
    {
        printf("remaining time: %d\n", *remainingTime);
        char *runCommand = (char *)malloc(18 * sizeof(char));
        char buffer[10];
        sprintf(buffer, "%d", *remainingTime);
        strcpy(runCommand, "./process.out ");
        strcat(runCommand, buffer);
        system(runCommand);
        exit(0);
    }

    return pid; // return pid to know which child process will be terminated due to an algorithm
}

// need to return boolean value after that for each call in the algorithms to check whether i recieved a process or not

void checkRecievedProcess()
{
    int added;

    // from process generator
    addedProcess = receiveMsg(msgq_id_GenSch);
    added = addedProcess.id - 1; ////// remove -1

    if (myUsedDS == DS_Queue)
        enqueue(Ready_queue, addedProcess);
    else if (myUsedDS = DS_PrioirtyQ)
        enqueue_priority(addedProcess, addedProcess.priority);

    printf("received process: %d\n", added + 1);
    PCB_LIST[added].arrival_time = addedProcess.arrival_time;
    PCB_LIST[added].priority = addedProcess.priority;
    //PCB_LIST[added].process_id=addedProcess.process_id;
    PCB_LIST[added].id = addedProcess.id;
    PCB_LIST[added].runtime = addedProcess.runtime;
    Ready_NUm_processes++;
    printf("recieved processes: %d\n", Ready_NUm_processes);
}

void stopProcess(int turn)
{
    int id = PCB_LIST[turn].process_id;
    PCB_LIST[turn].state = waiting;
    kill(id, SIGSTOP); //stopping it to be completed later
}

void resumeProcess(int turn)
{
    int id = PCB_LIST[turn].process_id;
    PCB_LIST[turn].state = running;
    kill(id, SIGCONT); //continue the stopped process
}

int startProcess(Process turnProcess)
{
    printf("start process: %d\n", turnProcess.id);
    sendMsg(msgq_id_PrcSch, turnProcess);
    *remainingTime = turnProcess.runtime;
    // run process.c
    return (ProcessExecution());
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

void Check_finshed_processes(int ID)
{
    if (PCB_LIST[ID].remainingtime == 0)
    {
        // write remaining time = 0 to process.c
        sharedMemory_func(0, PCB_LIST[ID].remainingtime);

        // read the real execution time from process.c
        sharedMemory_func(1, 0);
        // TA = finish - arr (total life time)
        WTA = (getClk() - PCB_LIST[ID].arrival_time) / PCB_LIST[ID].runtime;
        WTA_total += WTA;
        // finsh getclk -  arr - run
        WT = getClk() - PCB_LIST[ID].arrival_time - PCB_LIST[ID].runtime;
        WT_total += WT;

        fprintf(pFile, "At time %d\t process %d\t finished arr %d\t total %d\t remain %d\t wait\n",
                getClk(), PCB_LIST[ID].id, PCB_LIST[ID].arrival_time, PCB_LIST[ID].runtime,
                0, PCB_LIST[ID].waiting_time);

        //TODO:free (PCB_LIST[id]);
    }
}

void FCFS()
{
    printf("In FCFS\n");
    myUsedDS = DS_Queue; // I'll use a normal queue
    while (Ready_NUm_processes <= processesNum - 1)
    {
        checkRecievedProcess();
        // startProcess(addedProcess);
    }
}

void SJF() {}
//-------------------------- Highest Priority First --------------------------
void HPF()
{

    myUsedDS = DS_PrioirtyQ; /// I'll use Priority queue

    int current_turn = 0;
    int current_process_index = -1;
    int first_time = 1;
    int turn;
    // first time to recieve ?????????
    //loop till the queue is empty
    while (Ready_NUm_processes > 0 || HPF_Queue_size == -1) // or first time
    {
        // check if the currently running process is finished
        if (HPF_Queue_size != -1) // check from the seocnd time
            Check_finshed_processes(message.process.id);

        checkRecievedProcess();

        // no processes currently to schedule
        if (HPF_Queue_size == -1)
            continue;

        // get process with highest priority make it running and send it to the process
        current_process_index = current_turn;
        current_turn = peek_priority();

        // check if the added process has higher priority that the current running process
        // TODO: need handle first time
        if (current_process_index != current_turn) // awl mara???
        {
            // save context switch
            // remain = total run - time stopped - started
            PCB_LIST[message.process.id].remainingtime = message.process.runtime - getClk() - PCB_LIST[message.process.id].startingTime;
            PCB_LIST[message.process.id].stopped_time = getClk();
            //TODO:send remaining time to process.c
            sharedMemory_func(0, PCB_LIST[message.process.id].remainingtime);

            // inform process file to stop the process
            stopProcess(message.process.id);

            fprintf(pFile, "At time %d\t process %d\t stopped arr %d\t total %d\t remain %d\t wait\n",
                    getClk(), message.process.id, message.process.arrival_time, message.process.runtime,
                    PCB_LIST[message.process.id].remainingtime, PCB_LIST[message.process.id].waiting_time);

            // rejoin it to the ready queue
            enqueue_priority(message.process, message.process.priority);
            Ready_NUm_processes++;
        }

        //if process with the next turn is waiting in the ready queue:
        if (PCB_LIST[HPF_Queue[current_turn].myProcess.id].state == waiting)
        {
            resumeProcess((HPF_Queue[current_turn].myProcess.id));

            PCB_LIST[HPF_Queue[current_turn].myProcess.id].waiting_time += getClk() - PCB_LIST[HPF_Queue[current_turn].myProcess.id].stopped_time;

            fprintf(pFile, "At time %d\t process %d\t resumed arr %d\t total %d\t remain %d\t wait\n",
                    getClk(), HPF_Queue[current_turn].myProcess.id, HPF_Queue[current_turn].myProcess.arrival_time, HPF_Queue[current_turn].myProcess.runtime,
                    PCB_LIST[HPF_Queue[current_turn].myProcess.id].remainingtime, PCB_LIST[HPF_Queue[current_turn].myProcess.id].waiting_time);
            //TODO: send remaining time to prcosess
        }
        // first time to run
        else
        {
            message.process = HPF_Queue[current_turn].myProcess;
            PCB_LIST[message.process.id].startingTime = getClk();
            PCB_LIST[message.process.id].remainingtime = message.process.runtime;
            // write to the prcoess.c that remaining time is the whole run time
            sharedMemory_func(1, PCB_LIST[message.process.id].runtime);
            fprintf(pFile, "At time %d\t process %d\t started arr %d\t total %d\t remain %d\t wait %d\n",
                    getClk(), message.process.id, message.process.arrival_time, message.process.runtime, message.process.runtime, 0);
            // send the process parameters to process file
            int process_id = startProcess(message.process);

            if (process_id != -1)
            { //no errors occur during sending data of the process
                //     PCB_LIST[turn].process_id=process_id;
            }
        }
        // remove it from the ready queue ( ready =>> running)
        dequeue_priority();
        Ready_NUm_processes--;
    }
}

void SRTN() {}

//------------------------------------------ For Round Robin -------------------------------------------

void RR(int quantum)
{

    int currentTime;
    int turn;

    // if its the first time for the algorithm or if ready processes has ended
    while (Queue_length == -1 || Ready_NUm_processes > 0)
    {

        // check if i received any new process then add it to my PCB
        checkRecievedProcess();

        Process process_turn = dequeue(Ready_queue);
        Ready_NUm_processes--;
        turn = process_turn.id;

        if (PCB_LIST[turn].state == waiting)
        { //has started before

            //TODO: retrieve its data from the pcb
            resumeProcess(PCB_LIST[turn].process_id);
        }

        else
        { // first time
            int process_id = startProcess(process_turn);

            if (process_id != -1)
            { //no errors occur during sending data of the process
                PCB_LIST[turn].process_id = process_id;
            }
        }

        // habaaalllllll-------------------------------------
        currentTime = getClk();

        while (currentTime + quantum > getClk())
        {
            int newClk = getClk();
            if (newClk != currentTime)
                currentTime++;
            currentTime = newClk;
        }

        stopProcess(turn);

        if (PCB_LIST[turn].remainingtime == 0) //// check thissssss
        {
            // delete its pcb ---> create a finish function
        }

        else
        {
            enqueue(Ready_queue, process_turn);
            Ready_NUm_processes++;
        }
    }
}