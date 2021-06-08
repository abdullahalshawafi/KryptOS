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
        key_t key ;
        key = ftok("process_to_sch", 66); 
        int  msgq_id_PrcSch ;
        msgq_id_PrcSch = msgget(key, 0666 | IPC_CREAT);
     if (msgq_id_PrcSch == -1)
        {
            perror("Error in creating message queue in process: " + pid);
            exit(-1);
        }
/// Create shared memory to write el remaining time and execuation time in it =>> scheduler
    key_t key2 ;
    key2 = ftok("shm_running_time", 55); 
    int shmid;
    shmid = shmget(key2, 4096, IPC_CREAT | 0644);  
    if (shmid == -1)
    {
        perror("process.c : Error in create shared memory");
        exit(-1);
    }
      
     void *shmaddr = shmat(shmid, (void *)0, 0);
    if (atoi(shmaddr) == -1)
    {
        perror("process.c: Error in attach in writer (running time)");
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
        //TODO:  msh 3arfa ek trteb hna hyfr2 wla la2 y3ny a-check remaining time  first wla azwd el running time
        
           // calculate the real running time
            int newClk = getClk();
            if (newClk != lastClk)
               execution_time++;
            lastClk = newClk;

        /// read the remaining time from the scheduler
                remainingtime=  atoi ((char *)shmaddr);
    }

     
    /// before termination write the execution_time to 
    //shared memory so that el scheduler can read it ( to sum execution_time  of all prcoesses)
       strcpy((char *)shmaddr,  itoa(execution_time));
    // free it
    shmdt(shmaddr);
    return 0;
}
