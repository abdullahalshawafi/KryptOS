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

    // Set the current state to be a running proccess
    enum STATE currentState = running;

    // Get current PID to know which messages are sent to you
    pid = getpid();

    // Create the message queue on which you will talk to the scheduler
    int msgq_id_PrcSch = initMsgq(msgq_prcSchKey);

    // Create shared memory to write el remaining time and execuation time in it =>> scheduler
    int shmid;
    void *shm_addr = initShm(shmKey, &shmid);

    // recieve which process's PCB will be running
    Message message;
    int recValue = msgrcv(msgq_id_PrcSch, &message, sizeof(message.process), pid, IPC_NOWAIT);
    if (recValue == -1)
        perror("Process.c: Error in recieveing the process: ");

    // give the prcoess its parameters (sent from scheduler)
    startingTime = getClk();
    runtime = message.process.runtime;
    arrivaltime = message.process.arrival_time;

    while (remainingtime > 0)
    {
        //TODO: msh 3arfa ek trteb hna hyfr2 wla la2 y3ny a-check remaining time first wla azwd el running time

        // calculate the real running time
        int newClk = getClk();
        if (newClk != lastClk)
            execution_time++;
        lastClk = newClk;

        /// read the remaining time from the scheduler
        remainingtime = atoi((char *)shm_addr);
    }

    /// before termination write the execution_time to
    //shared memory so that el scheduler can read it ( to sum execution_time  of all prcoesses)
    char buffer[10];
    sprintf(buffer, "%d", execution_time);
    strcpy((char *)shm_addr, buffer);
    // free it
    shmdt(shm_addr);
    return 0;
}
