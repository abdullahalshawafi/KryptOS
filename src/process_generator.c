#include "headers.h"

void clearResources(int);

struct process
{
    int id, arrival_time, runtime, priority;
};

typedef struct process process;

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);

    // Initializations
    FILE *inputFile;
    char *buffer = NULL;
    size_t bufsize = 0;
    int processesNum = 0, index = 0, schedulingAlgorithm, algorithmParameter = -1;

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
    if (schedulingAlgorithm == 5)
        algorithmParameter = atoi(argv[2]);

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
