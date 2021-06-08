#include "headers.h"
#include <string.h>
#include <stdio.h>

//////////////////// -------------------  General buffers -----------------------------

///  buffer struct for process generator to scheduler queue 
struct buff_GenSch
{
    int mtype;
    struct process NewProcess; // process that will be sent from process generator to scheduler
   
};

///  buffer struct for process file to scheduler queue 
struct processSchedulermsgbuff
{
    long mtype;
    int timeTaken;
    struct process running_process;
};


//////////////////  -------------------- PCB---------------------------------------
struct PCB 
{

    int id, //this id read from the input file
    process_id, // this the actual id in the system 
	arrival_time, 
	runtime, 
	priority,
	remainingtime, 
	startingTime,
	stopped_time,
	resume_time,
    waiting_time;
    enum STATE state;

};

//--------------------------------------global declerations -------------------------------------
typedef struct process process;
typedef struct PCB PCB;

int  msgq_id_PrcSch ;
 int shmid;
Queue *Ready_queue; 
process addedProcess;
PCB * PCB_LIST;
int Ready_NUm_processes=0;

/// BUFFERS
struct processSchedulermsgbuff message;
struct buff_GenSch process_msg;

// variables used to write in the files
FILE *pFile;
float CPU_U;
float WT;
float WTA;
float WTA_total;
float WT_total;
// need to compute them
int sys_start_time;
int  total_exec_time; 

// data structures used for algorithms 
int DS_Queue=1;
int DS_PrioirtyQ=2;
int myUsedDS;


///----------------------------------------------- MAIN --------------------------------------------------------
int main(int argc, char *argv[])
{
    initClk();
    sys_start_time = getClk();
    PCB_LIST = (PCB *)malloc(processesNum* sizeof(PCB));
    pFile = fopen("Scheduler.log", "w");
    //fclose(pFile); // close the file emta msh 3arfa rabna ysahl

///----------------------------------- Message queues initializations------------------------

//  msq queue to talk to process generator
        key_t key1 ;
        key1= ftok("genrator_to_sch", 65); 
        
        msgq_id_GenSch = msgget(key1, 0666 | IPC_CREAT);
        if (  msgq_id_GenSch  == -1)
            {
                perror("Error in create up queue");
                exit(-1);
            }
      //  msq queue to talk to process file
        key_t key2 ;
        key2 = ftok("processto_sch", 66); 
        msgq_id_PrcSch = msgget(key2, 0666 | IPC_CREAT);
        if (  msgq_id_PrcSch   == -1)
            {
                perror("Error in create up queue");
                exit(-1);
            }
/// Create shared memory to write /read   remaining time / execuation time in it =>> Process.c
    key_t key2 ;
    key2 = ftok("shm_running_time", 55); 
    shmid = shmget(key2, 4096, IPC_CREAT | 0644);  
    if (shmid == -1)
    {
        perror("Scheduler : Error in create shared memory");
        exit(-1);
    }     
            switch (schedulingAlgorithm)
            {
                case 1: FCFS();
                    break;
                case 2: SJF();
                    break;
                case 3: HPF();
                    break;
                case 4: SRTN();
                    break;
                case 5: RR(quantum);
                    break;    
            }
     close(pFile); // close the file emta msh 3arfa rabna ysahl

            pFile = fopen("Scheduler.pref", "w");
            CPU_U = (getClk() - sys_start_time)/ total_exec_time ;
            fprintf(pFile, "A CPU utilization = %d\t \n", CPU_U);
            fprintf(pFile, "Avg WTA = %d\t \n",  WT_total/processesNum);
            fprintf(pFile, "Avg Waiting = %d\t \n", WT_total/processesNum);
            close(pFile); 
//TODO: upon termination release the clock resources. ?????????????

    destroyClk(true);
}

int ProcessExecution(){

    int pid = fork();

    if (pid  == -1)
        perror("error in fork");


    else if (pid == 0) // running process 
        {
            system("./process.out");
            exit(-1);
        }

    return pid; // return pid to know which child process will be terminated due to an algorithm
}

// need to return boolean value after that for each call in the algorithms to check whether i recieved a process or not

void checkRecievedProcess()
{
    int added; 

// from process generator 
    rec_process = msgrcv(msgq_id_GenSch, &process_msg, sizeof( process_msg.NewProcess),0, !IPC_NOWAIT); 
    
    if (rec_process == -1){
        perror("Error in receiving process");
        return;
    } 

    addedProcess=process_msg.NewProcess; // make my process equals to the process coming from the msg queue

    added=addedProcess.id; ////// remove -1 

    if(myUsedDS== DS_Queue){
        enqueue(Ready_queue,addedProcess);
    }
    else if(myUsedDS=DS_PrioirtyQ){
        enqueue_priority(addedProcess,addedProcess.priority);
    }
   
   PCB_LIST[added].arrival_time=addedProcess.arrival_time;
   PCB_LIST[added].priority=addedProcess.priority;
//    PCB_LIST[added].process_id=addedProcess.process_id;
   PCB_LIST[added].id=addedProcess.id;
   PCB_LIST[added].runtime=addedProcess.runtime;

   Ready_NUm_processes++;

}


void stopProcess(int turn)
{
    int id= PCB_LIST[turn].process_id;
    PCB_LIST[turn].state=waiting;
    kill(id,SIGSTOP); //stopping it to be completed later

}


void resumeProcess(int turn){

    int id= PCB_LIST[turn].process_id;
    PCB_LIST[turn].state=running;
    kill(id,SIGCONT); //continue the stopped process

}

int startProcess(process turnProcess){

    message.running_process = turnProcess;

    int sen_val = msgsnd(msgq_id_PrcSch, &message, sizeof(message), !IPC_NOWAIT);
    if (sen_val == -1)
    {
        perror("error in scheduler sending new prcoess ");
        return -1;
    }
           
    /// run process.c
   return (ProcessExecution());
   
}

void sharedMemory_func( int RW , int remainTime)
{
    void *shmaddr = shmat(shmid, (void *)0, 0);
      
    if (atoi(shmaddr) == -1)
    {
        perror("Scheduler: Error in attach in writer/reader ");
        exit(-1);
    }
    // read rexecution  time from the shared memory
       if( RW == 1)   
        {
             total_exec_time+= atoi ((char *)shmaddr);
        }
    // write remaining time to the shared memory
       else
       {
          strcpy((char *)shmaddr,  itoa(remainTime));
       } 
     //TODO: not sure : destroy 
      shmdt(shmaddr);
}
void Check_finshed_processes( int ID )
{
    if ( PCB_LIST[ID].remainingtime == 0 )
     {
        // write remaining time = 0 to process.c
          sharedMemory_func(0 , PCB_LIST[ID].remainingtime);

        // read the real execution time from process.c  
          sharedMemory_func(1,0);
        // TA = finish - arr (total life time)
              WTA= (getClk() -PCB_LIST[ID].arrival_time) / PCB_LIST[ID].runtime;
              WTA_total+=WTA;
        // finsh getclk -  arr - run
               WT=  getClk() -PCB_LIST[ID].arrival_time-PCB_LIST[ID].runtime;
               WT_total+=WT;

                  fprintf(pFile, "At time %d\t process %d\t finished arr %d\t total %d\t remain %d\t wait\n",
                  getClk(),PCB_LIST[ID].id , PCB_LIST[ID].arrival_time, PCB_LIST[ID].runtime,
                   0 , PCB_LIST[ID].waiting_time);

            //TODO:free (PCB_LIST[id]);
    }
}

void FCFS() {}

void SJF(){}
//-------------------------- Highest Priority First --------------------------
void HPF ()
{

    myUsedDS= DS_PrioirtyQ; /// I'll use Priority queue
    
    int current_turn=0;
    int current_process_index=-1;
    int first_time=1;
    int turn;
    // first time to recieve ????????? 
    //loop till the queue is empty
     while( Ready_NUm_processes > 0 ||  HPF_Queue_size ==-1   ) // or first time
     {
        // check if the currently running process is finished
         if(HPF_Queue_size != -1) // check from the seocnd time
         Check_finshed_processes( message.running_process.id);

         checkRecievedProcess();

        // no processes currently to schedule
        if(HPF_Queue_size == -1) 
            continue;
        
        // get process with highest priority make it running and send it to the process  
        current_process_index=current_turn; 
        current_turn= peek_priority();
      
        // check if the added process has higher priority that the current running process
        // TODO: need handle first time 
        if ( current_process_index != current_turn ) // awl mara???
            {
              // save context switch 
                // remain = total run - time stopped - started
                  PCB_LIST[message.running_process.id].remainingtime =  message.running_process.runtime- getClk() -PCB_LIST[message.running_process.id].startingTime;
                  PCB_LIST[message.running_process.id].stopped_time=getClk();
               //TODO:send remaining time to process.c
                sharedMemory_func(0 , PCB_LIST[message.running_process.id].remainingtime);

              // inform process file to stop the process
                stopProcess( message.running_process.id);

                 fprintf(pFile, "At time %d\t process %d\t stopped arr %d\t total %d\t remain %d\t wait\n",
                  getClk(), message.running_process.id, message.running_process.arrival_time, message.running_process.runtime,
                   PCB_LIST[message.running_process.id].remainingtime , PCB_LIST[message.running_process.id].waiting_time);
                    
                // rejoin it to the ready queue
                enqueue_priority( message.running_process ,  message.running_process.priority);
                Ready_NUm_processes ++;
            }
            
          //if process with the next turn is waiting in the ready queue:
            if (PCB_LIST[HPF_Queue[current_turn].myProcess.id].state == waiting )
            {
                resumeProcess((HPF_Queue[current_turn].myProcess.id));

                PCB_LIST[HPF_Queue[current_turn].myProcess.id].waiting_time +=  getClk() -PCB_LIST[HPF_Queue[current_turn].myProcess.id].stopped_time;
                
                fprintf(pFile, "At time %d\t process %d\t resumed arr %d\t total %d\t remain %d\t wait\n",
                  getClk(), HPF_Queue[current_turn].myProcess.id , HPF_Queue[current_turn].myProcess.arrival_time,HPF_Queue[current_turn].myProcess.runtime,
                  PCB_LIST[HPF_Queue[current_turn].myProcess.id].remainingtime ,PCB_LIST[HPF_Queue[current_turn].myProcess.id].waiting_time);
               //TODO: send remaining time to prcosess

            } 
            // first time to run 
            else
            {
                 message.running_process = HPF_Queue[current_turn].myProcess;
                 PCB_LIST[message.running_process.id].startingTime =getClk();
                 PCB_LIST[message.running_process.id].remainingtime=message.running_process.runtime;
                 // write to the prcoess.c that remaining time is the whole run time
                 sharedMemory_func(1,  PCB_LIST[message.running_process.id].runtime);
                 fprintf(pFile, "At time %d\t process %d\t started arr %d\t total %d\t remain %d\t wait %d\n",
                  getClk(), message.running_process.id,message.running_process.arrival_time, message.running_process.runtime, message.running_process.runtime,0 );
             // send the process parameters to process file
                int process_id=startProcess(message.running_process);

                if(process_id!=-1){ //no errors occur during sending data of the process
               //     PCB_LIST[turn].process_id=process_id;
                }
            }     
            // remove it from the ready queue ( ready =>> running)
            dequeue_priority();  
            Ready_NUm_processes --;  
      } 
    }



    void SRTN(){}


       //------------------------------------------ For Round Robin -------------------------------------------

void RR(int quantum){

    int currentTime;
    int turn;

        // if its the first time for the algorithm or if ready processes has ended
    while (Queue_length==-1 || Ready_NUm_processes >0){
            
        // check if i received any new process then add it to my PCB
        checkRecievedProcess(); 
            
        process process_turn=dequeue(Ready_queue);
        Ready_NUm_processes--; 
        turn=process_turn.id;

        if(PCB_LIST[turn].state==waiting) { //has started before

                //TODO: retrieve its data from the pcb 
                resumeProcess(PCB_LIST[turn].process_id); 
        }

        
        else { // first time     
            int process_id=startProcess(process_turn);

            if(process_id!=-1){ //no errors occur during sending data of the process
                    PCB_LIST[turn].process_id=process_id;
                }
            }
             
        // habaaalllllll-------------------------------------
            currentTime=getClk();

        while (currentTime+quantum>getClk()){
                int newClk = getClk();
                if (newClk != currentTime)
                currentTime++;
                currentTime = newClk;
        }

        stopProcess(turn);

        if(PCB_LIST[turn].remainingtime==0) //// check thissssss
        {
                // delete its pcb ---> create a finish function
        }

        else {
                enqueue(Ready_queue,process_turn);
                Ready_NUm_processes++; 
         }
}