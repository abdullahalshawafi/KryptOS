#include "headers.h"

 
// Process can receive the changes in its state in a message queue
// Or send timeTaken to the scheduler
struct processSchedulermsgbuff
{
    long mtype;
    int timeTaken;
    enum STATE state;
    struct process running_process;
};

// We store the remaining time to decrement it if the clock value
// increased and the process was running
int remainingtime, startingTime, pid, lastClk , runtime, arrivaltime;

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
        key_t key ;
        key = ftok("process_to_sch", 66); 
        int  msgq_id_PrcSch ;
        msgq_id_PrcSch = msgget(key, 0666 | IPC_CREAT);
     if (msgq_id_PrcSch == -1)
        {
            perror("Error in creating message queue in process: " + pid);
            exit(-1);
        }

    /// recieve which process's PCB will be running
        struct processSchedulermsgbuff message;
                int recValue = msgrcv(msgq_id_PrcSch, &message, sizeof(message.running_process), pid, IPC_NOWAIT);
                if (recValue == -1)
                {
                    perror("Process.c :Error in recieveing the process: " );
        }
        // give the prcoess its parameters (sent from scheduler)
            startingTime = getClk();  
            runtime = message.running_process.runtime;
            arrivaltime= message.running_process.arrival_time;


    while (remainingtime > 0)
    {
        
            // When clock changes (increases) reduce remainingtime
            int newClk = getClk();
            if (newClk != lastClk)
                remainingtime--;
            lastClk = newClk;
    
    }

    /// bye
    return 0;
}
