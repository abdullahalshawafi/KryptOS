#include "headers.h"

////------------- global  declerations -------------////
int processesNum_schHas = 0;
int Index = 0;
int msgq_id_PrcSch;
int current_process;

///------------- MAIN -------------////
int main(int argc, char *argv[])
{
    initClk();

    // msq queue to talk to process generator

    key_t key1 = ftok("keyfile", msgq_genSchKey);
    msgq_id_GenSch = msgget(key1, 0666 | IPC_CREAT);
    if (msgq_id_GenSch == -1)
    {
        perror("Error in create msg queue");
        exit(-1);
    }

    // msq queue to talk to process file

    key_t key2 = ftok("kefile", msgq_id_PrcSch);
    msgq_id_PrcSch = msgget(key2, 0666 | IPC_CREAT);
    if (msgq_id_PrcSch == -1)
    {
        perror("Error in create msg queue");
        exit(-1);
    }

    // save all ready received processes in this array
    Process *ReadyProcesses = (Process *)malloc(processesNum * sizeof(Process));

    // still not sure but at least we know it will be working till the process generator stop sending processes to it
    // but we didn't cover the process side yet ???
    while (processesNum_sent_toSCH < processesNum)
    {

        // if we received a process add it to the array
        Message process_msg;
        rec_process = msgrcv(msgq_id_GenSch, &process_msg, sizeof(process_msg.NewProcess), 0, !IPC_NOWAIT);

        if (rec_process == -1)
        {
            perror("No process is ready now");
            exit(-1);
        }

        ReadyProcesses[Index++] = process_msg.NewProcess; // make my process equals to the process coming from the msg queue
        printf("Clk: %d\n", getClk());
        printf("Process arrival time: %d\n", process_msg.NewProcess.arrival_time);
        // ReadyProcesses[Index] = newPuff.NewProcess;  ///// uncomment meeeee

        // switch (schedulingAlgorithm)
        // {
        // case 1:
        //     FCFS();
        //     break;
        // case 2:
        //     SJF();
        //     break;
        // case 3:
        //     HPF();
        //     break;
        // case 4:
        //     SRTN();
        //     break;
        // case 5:
        //     RR(quantum);
        //     break;
        // }
    }

    // TODO: implement the scheduler.
    // TODO: upon termination release the clock resources.

    destroyClk(true);
}

int ProcessExecution()
{
    int pid = fork();

    if (pid == -1)
        perror("error in fork");

    else if (pid == 0) // running process
        system("./process.out");

    return pid; // return pid to know which child process will be terminated due to an algorithm
}

// need to return boolean value after that for each call in the algorithms to check whether i recieved a process or not

Process checkingRecievedProcess()
{
    Process process;
    Message process_msg;
    rec_process = msgrcv(msgq_id_GenSch, &process_msg, sizeof(process_msg.NewProcess), 0, !IPC_NOWAIT);

    if (rec_process == -1)
    {
        perror("No process is ready now");
        exit(-1);
    }

    process = process_msg.NewProcess; // make my process equals to the process coming from the msg queue
    printf("Clk: %d\n", getClk());
    printf("Process arrival time: %d\n", process.arrival_time);
    // Index++;
    // processesNum++;
    return process;
}

void FCFS() {}

void SJF() {}

void HPF()
{
    Message process_msg;
    processSchedulermsgbuff message;
    int current_turn = 0;
    int rec_val;
    int sen_val;
    rec_val = msgrcv(msgq_id_GenSch, &process_msg, sizeof(process_msg.NewProcess), 0, !IPC_NOWAIT);
    if (rec_val == -1)
    {
        perror("No new process is ready now");
        return; //?????????????
    }
    else
    {
        processesNum_schHas++;
        // add the new recieved process to the priority queue
        enqueue_priority(process_msg.NewProcess, process_msg.NewProcess.priority);
    }

    // get process with highest priority make it running and send it to the process
    current_turn = peek_priority();
    current_process = current_turn;
    ProcessExecution();
    message.running_process = HPF_Queue[current_turn].myProcess;
    sen_val = msgsnd(msgq_id_PrcSch, &message, sizeof(message.running_process), !IPC_NOWAIT);
}

void SRTN() {}

void RR(int quantum)
{
}