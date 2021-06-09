#include "headers.h"

// We store the remaining time to decrement it if the clock value
// increased and the process was running
int remainingtime, startingTime, pid, lastClk, runtime, arrivaltime;
int execution_time;

int main(int agrc, char *argv[])
{
    // Initialize clock and set remaining time
    initClk();
    int lastClk = -1;
    shm_remainingTime = (int *)initShm(remainKey, &shm_remainingTime_ID);

    while (*shm_remainingTime > 0)
    {
        while (getClk() == lastClk)
            ;
        if (getClk() != lastClk)
        {
            (*shm_remainingTime)--;
            lastClk = getClk();
        }
    }

    destroyClk(false);

    return 0;
}
