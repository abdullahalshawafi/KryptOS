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
int lastClk;

int main(int agrc, char *argv[])
{
    // Initialize clock and set remaining time and startTime
    initClk();
  //  remainingtime = atoi(argv[1]);
   // startingTime = lastClk = getClk();

    // Set the current state to be a running proccess
    enum STATE currentState = RUNNING;

    // Get current PID to know which messages are sent to you
     int pid = getpid();

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

/// recieve which procee will be running
 struct processSchedulermsgbuff message;
        int recValue = msgrcv(msgq_id_PrcSch, &message, sizeof(message.running_process), pid, IPC_NOWAIT);
        if (recValue != -1)
        {
            currentState = message.state;
        }
struct process current_process = message.running_process;

current_process.startingTime = getClk(); // i am not sure Marim edit

    while (current_process.remainingtime > 0)
    {
        
        // Check for any messages from the scheduler, if any update the state
        struct processSchedulermsgbuff message;
        int recValue = msgrcv(msgq_id_PrcSch, &message, sizeof(message.state), pid, IPC_NOWAIT);
        if (recValue != -1)
        {
            currentState = message.state;

        }

        if (currentState == RUNNING)
        {
            // When clock changes (increases) reduce remainingtime
            int newClk = getClk();
            if (newClk != lastClk)
              current_process.remainingtime--;
            lastClk = newClk;
        }
        else
        {
            // set that it stopped at time ... ( we will write it later in external file as it will be changed each time we block the process)
              current_process.stopped_time= getClk();
              // add to blockd vector leh? 7alyn m3rfsh but we may need it :D
              sb_push(Blocked_Processes, current_process);
            // some blocking logic !!

        }
    }

    // When the process is about to finish it should send the time taken to finish to the scheduler
    int timeTaken = getClk() - current_process.startingTime;
    struct processSchedulermsgbuff message;
    message.mtype = pid;
    message.timeTaken = timeTaken;
    int sendValue = msgsnd(msgq_id_PrcSch , &message, sizeof(message.timeTaken), !IPC_NOWAIT);
    if (sendValue = -1)
        perror("Error in sending taken time from process: " + pid);
    destroyClk(false);
    return 0;
}
