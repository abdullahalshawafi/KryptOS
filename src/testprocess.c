

#include "headers.h"



// Process can receive the changes in its state in a message queue
// Or send timeTaken to the scheduler
struct processSchedulermsgbuff
{
    long mtype;
    int timeTaken;
    struct process running_process;
    enum STATE state;
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
    enum STATE currentState = running;

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

/// recieve which process's PCB will be running
 struct processSchedulermsgbuff message;
        int recValue = msgrcv(msgq_id_PrcSch, &message, sizeof(message.running_process), pid, IPC_NOWAIT);
        if (recValue == -1)
        {
              perror("Process.c :Error in recieveing the process: " );
        }

  current_running_process = message.running_process;
  current_running_process.startingTime = getClk(); // i am not sure Marim edit
  current_running_process.remainingtime = -1; // i am not sure Marim edit
  current_running_process.state= run;

    while (current_running_process.remainingtime > 0)
    {
        
        // Check for any messages from the scheduler, if any update the state
        
          recValue = msgrcv(msgq_id_PrcSch, &message, sizeof(message.state), pid, IPC_NOWAIT);
        if (recValue != -1)
        {
            currentState = message.state;

        }

        if ( currentState == STOPPED)
        {
                // stop it => add it again to the ready queue to take a turn again
                enqueue(Ready_Processes,current_running_process);
                    // set that it stopped at time ... ( we will write it later in external file as it will be changed each time we block the process)
                    
                    current_running_process.stopped_time= getClk();
                     kill( getpid() ,SIGSTOP); //stopping it to be completed later
        }
        else if ( currentState == RESUMED)
        {
            
            kill(getpid(),SIGCONT); 
            dequeue( Ready_Processes);
        }
    
      //  else => we didn't need else ? msh 3arfa 
      // ana msh 3arfa el logic da el mafroy y7sal f ay state wl eh ? sorry Marim
        {
        // When clock changes (increases) reduce remainingtime
                    int newClk = getClk();
                    if (newClk != lastClk)
                    current_running_process.remainingtime--;
                    lastClk = newClk;
        }
         
    }
       
    // When the process is about to finish it should send the time taken to finish to the scheduler
    int timeTaken = getClk() - current_running_process.startingTime;
    struct processSchedulermsgbuff message;
    message.mtype = pid;
    message.timeTaken = timeTaken;
    int sendValue = msgsnd(msgq_id_PrcSch , &message, sizeof(message.timeTaken), !IPC_NOWAIT);
    if (sendValue = -1)
        perror("Error in sending taken time from process: " + pid);
    destroyClk(false);
    return 0;
}
