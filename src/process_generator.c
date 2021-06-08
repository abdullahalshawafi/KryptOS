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
        system("./scheduler.out");
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

    key_t key1 = ftok("keyfile", msgq_genSchKey);
    msgq_id_GenSch = msgget(key1, 0666 | IPC_CREAT);
    if (msgq_id_GenSch == -1)
    {
        perror("Error in create msg queue");
        exit(-1);
    }

    // 6. Send the information to the scheduler at the appropriate time.

    int i = 0, sen_val;

    // loop untill all the processes in the file are sent to scheduler
    while (processesNum_sent_toSCH < processesNum)
    {
        if (processes[i].arrival_time == getClk())
        {
            Message process_msg;
            process_msg.NewProcess = processes[i];
            if (sen_val = msgsnd(msgq_id_GenSch, &process_msg, sizeof(process_msg.NewProcess), !IPC_NOWAIT) != 1)
                perror("Errror in sending process to scheduler");
            processesNum_sent_toSCH++;
        }

        // we may reach end of the array but still there are some processes not sent to scheduler yet
        // if (i == processesNum)
        //     i = 0;
        // else
        //     i++;
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
