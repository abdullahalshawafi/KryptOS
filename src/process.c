#include "headers.h"

// Each process have a state of only two state running or blocked
enum STATE
{
    RUNNING,
    BLOCKED
};

// Process can receive the changes in its state in a message queue
// Or send timeTaken to the scheduler
struct processSchedulermsgbuff
{
    long mtype;
    int timeTaken;
    enum STATE state;
};

// We store the remaining time to decrement it if the clock value
// increased and the process was running
int remainingtime, startingTime, pid, lastClk;

int main(int agrc, char *argv[])
{
    // Initialize clock and set remaining time and startTime
    initClk();
    remainingtime = atoi(argv[1]);
    startingTime = lastClk = getClk();

    // Set the current state to be a running proccess
    enum STATE currentState = RUNNING;

    // Get current PID to know which messages are sent to you
    pid = getpid();

    // Create the message queue on which you will talk to the scheduler
    key_t msgqid = msgget(5555, IPC_CREAT | 0644);
    if (msgqid == -1)
    {
        perror("Error in creating message queue in process: " + pid);
        exit(-1);
    }

    while (remainingtime > 0)
    {
        // Check for any messages from the scheduler, if any update the state
        struct processSchedulermsgbuff message;
        int recValue = msgrcv(msgqid, &message, sizeof(message.state), pid, IPC_NOWAIT);
        if (recValue != -1)
        {
            currentState = message.state;
        }

        if (currentState == RUNNING)
        {
            // When clock changes (increases) reduce remainingtime
            int newClk = getClk();
            if (newClk != lastClk)
                remainingtime--;
            lastClk = newClk;
        }
    }

    // When the process is about to finish it should send the time taken to finish to the scheduler
    int timeTaken = getClk() - startingTime;
    struct processSchedulermsgbuff message;
    message.mtype = pid;
    message.timeTaken = timeTaken;
    int sendValue = msgsnd(msgqid, &message, sizeof(message.timeTaken), !IPC_NOWAIT);
    if (sendValue = -1)
        perror("Error in sending taken time from process: " + pid);
    destroyClk(false);
    return 0;
}
