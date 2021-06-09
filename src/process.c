#include "headers.h"

// We store the remaining time to decrement it if the clock value
// increased and the process was running
int remainingtime, startingTime, pid, lastClk, runtime, arrivaltime;
int execution_time;

int main(int agrc, char *argv[])
{
    // Initialize clock and set remaining time and startTime
    initClk();
    remainingtime = atoi(argv[1]);
    startingTime = lastClk = getClk();
    int id = atoi(argv[2]);
    printf("hello i am in process.c with is %d\n", getpid());
    printf("i am process with is  %d  my remaining time %d\n", id, remainingtime);

    // Set the current state to be a running proccess
    enum STATE currentState = running;

    // Get current PID to know which messages are sent to you
    pid = getpid();

    // Create the message queue on which you will talk to the scheduler
    //int msgq_id_PrcSch = initMsgq(msgq_prcSchKey);

    // recieve which process's PCB will be running
    //   Message message;
    // int recValue = msgrcv(msgq_id_PrcSch, &message, sizeof(message.process), 0, IPC_NOWAIT);
    //if (recValue == -1)
    //  perror("Process.c: Error in recieveing the process: ");

    // give the prcoess its parameters (sent from scheduler)
    startingTime = getClk();

    while (remainingtime > 0)
    {
        //TODO: msh 3arfa ek trteb hna hyfr2 wla la2 y3ny a-check remaining time first wla azwd el running time

        // calculate the real running time
        int newClk = getClk();
        if (newClk != lastClk)
            remainingtime--;
        lastClk = newClk;
    }

    /// before termination write the execution_time to
    //shared memory so that el scheduler can read it ( to sum execution_time  of all prcoesses)

    return 0;
}
