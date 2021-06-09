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
   // printf("hello i am in process.c with is %d\n", getpid());
  //  printf("i am process with is  %d  my remaining time %d\n", id, remainingtime);

   
    // Get current PID to know which messages are sent to you
    pid = getpid();

        // create shared memory to detect the remaining time
    shm_remainingTime= (int *)initShm(remainKey, &shm_remainingTime_ID);

    while ((*shm_remainingTime)> 0)
    {
        
        // calculate the real running time
        int newClk = getClk();
        if (newClk != lastClk)
            (*shm_remainingTime)--;
        lastClk = newClk;
    }

    /// before termination write the execution_time to
    //shared memory so that el scheduler can read it ( to sum execution_time  of all prcoesses)

    return 0;
}
