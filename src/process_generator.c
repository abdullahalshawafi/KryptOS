#include "headers.h"

void clearResources(int);

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);

    // Initializations
    FILE *inputFile;
    char *buffer = NULL;
    size_t bufsize = 0;
    int index = 0;

    // 1. Read the input files.
    inputFile = fopen(argv[1], "r");

    while (getline(&buffer, &bufsize, inputFile) != EOF)
        if (buffer[0] != '#')
            processesNum++;

    fclose(inputFile);

    // 2. Read the chosen scheduling algorithm and its parameters, if there are any from the argument list.
    schedulingAlgorithm = atoi(argv[2]);
    if (schedulingAlgorithm < 1 || schedulingAlgorithm > 5)
    {
        printf("Invalid scheduling algorithm. Please choose a number between 1 and 5.\n");
        return -1;
    }
    if (schedulingAlgorithm == 5)
        quantum = atoi(argv[3]);

    // 3. Initiate and create the scheduler and clock processes.
    int clk_processId = fork();
    if (clk_processId == -1)
        perror("error in fork");

    else if (clk_processId == 0)
    {
        system("./clk.out");
        exit(0);
    }

    int scheduler_processId = fork();
    if (scheduler_processId == -1)
        perror("error in fork");

    else if (scheduler_processId == 0)
    {
        char *runCommand = (char *)malloc(23 * sizeof(char));
        char buffer[10];
        if (schedulingAlgorithm == 5)
            sprintf(buffer, "%d %d %d", processesNum, schedulingAlgorithm, quantum);
        else
            sprintf(buffer, "%d %d", processesNum, schedulingAlgorithm);
        strcpy(runCommand, "./scheduler.out ");
        strcat(runCommand, buffer);
        system(runCommand);
        exit(0);
    }

    // 4. Use this function after creating the clock process to initialize clock.
    initClk();

    // TODO Generation Main Loop

    // 5. Create a data structure for processes and provide it with its parameters.
    Process *processes = (Process *)malloc(processesNum * sizeof(Process));

    inputFile = fopen(argv[1], "r");

    while (getline(&buffer, &bufsize, inputFile) != EOF)
    {
        fscanf(inputFile, "%d\t%d\t%d\t%d", &processes[index].id, &processes[index].arrival_time, &processes[index].runtime, &processes[index].priority);
        index++;
    }

    fclose(inputFile);

    // start sending the ready processes to the scheduler

    msgq_id_GenSch = initMsgq(msgq_genSchKey);
    // 6. Send the information to the scheduler at the appropriate time.

    Message message;
    // loop untill all the processes in the file are sent to scheduler
    while (processesNum_sent_toSCH < processesNum)
    {
        if (processes[processesNum_sent_toSCH].arrival_time == getClk())
        {
            printf("send process: %d\n", processes[processesNum_sent_toSCH].id);
            message.process = processes[processesNum_sent_toSCH];
            if (msgsnd(msgq_id_GenSch, &message, sizeof(message.process), !IPC_NOWAIT) == -1)
                perror("Error in sending the process");
            // sendMsg(msgq_id_GenSch, processes[processesNum_sent_toSCH]);
            processesNum_sent_toSCH++;
        }
    }

    // 7. Clear clock resources
    msgctl(msgq_id_GenSch, IPC_RMID, (struct msqid_ds *)0);
    destroyClk(true);
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
    msgctl(msgq_id_GenSch, IPC_RMID, (struct msqid_ds *)0);
    exit(0);
}
