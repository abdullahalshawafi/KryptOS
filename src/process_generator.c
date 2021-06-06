#include "headers.h"

///  buffer struct for process generator to scheduler queue 
struct buff_GenSch
{
    int mtype;
    struct process NewProcess; // process that will be sent from process generator to scheduler
   
};
void clearResources(int);

typedef struct process process;

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

    process *processes = (process *)malloc(processesNum * sizeof(process));

    inputFile = fopen(argv[1], "r");

    while (getline(&buffer, &bufsize, inputFile) != EOF)
    {
        fscanf(inputFile, "%d\t%d\t%d\t%d", &processes[index].id, &processes[index].arrival_time, &processes[index].runtime, &processes[index].priority);
        index++;
    }

    // 2. Read the chosen scheduling algorithm and its parameters, if there are any from the argument list.
    schedulingAlgorithm = atoi(argv[1]);
    if (schedulingAlgorithm == 5) //RR
        quantum = atoi(argv[2]);

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
        sleep(5);
        system("./scheduler.out");
        exit(0);
    }
    //  start sending the ready processes to the scheduler
        key_t key1 ;
        key1= ftok("genrator_to_sch", 65); 
        int sen_val;
        msgq_id_GenSch = msgget(key1, 0666 | IPC_CREAT);
        if (  msgq_id_GenSch  == -1)
            {
                perror("Error in create up queue");
                exit(-1);
            }
        // loop untill all the processes in the file sent to scheduler
             int i =0;
           while ( processesNum_sent_toSCH < processesNum)
        {
            if( processes[i].arrival_time == getClk() )
            {
                struct buff_GenSch process_msg;
                process_msg.NewProcess= processes[i];
                sen_val = msgsnd(msgq_id_GenSch, &process_msg, sizeof(process_msg.NewProcess), !IPC_NOWAIT);
                processesNum_sent_toSCH ++;
            }
            // we may reach end of the array but still there are some processes not sent to scheduler yet
            if( i == processesNum)
              i=0;
             else
             i++;
           
        }
    
    // 4. Use this function after creating the clock process to initialize clock.
    initClk();
    // To get time use this function.
    int x = getClk();
    while (1)
    {
        if (getClk() != x)
        {
            x = getClk();
            printf("Current Time is %d\n", x);
        }
    }

    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    // 7. Clear clock resources
    destroyClk(true);
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
    exit(0);
}
