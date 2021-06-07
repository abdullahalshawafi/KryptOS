#include "headers.h"
#include <string.h>


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
	resume_time;   
    enum STATE state;

};

//--------------------------------------global declerations -------------------------------------
typedef struct process process;
typedef struct PCB PCB;

int  msgq_id_PrcSch ;
Queue *Running_queue; 
process currentProcess;
PCB * PCB_LIST;
int Ready_NUm_processes=0;

// data structures used for algorithms 
int DS_Queue=1;
int DS_PrioirtyQ=2;
int myUsedDS;
   
///----------------------------------------------- MAIN --------------------------------------------------------
int main(int argc, char *argv[])
{
    initClk();
    PCB_LIST = (PCB *)malloc(processesNum* sizeof(PCB));


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
      
    

    //TODO: upon termination release the clock resources.

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

int checkRecievedProcess()
{
    int turn=0; 

// from process generator 
    struct buff_GenSch process_msg;
    rec_process = msgrcv(msgq_id_GenSch, &process_msg, sizeof( process_msg.NewProcess),0, !IPC_NOWAIT); 
    
    if (rec_process == -1){
        perror("Error in receiving process");
        return -1;
    } 
    currentProcess=process_msg.NewProcess; // make my process equals to the process coming from the msg queue

    turn=currentProcess.id -1; ////// remove -1 

    if(myUsedDS== DS_Queue){
        enqueue(Running_queue,currentProcess);
    }
    else if(myUsedDS=DS_PrioirtyQ){
        enqueue_priority(currentProcess,currentProcess.priority);
    }
   
    Ready_NUm_processes++;

    return turn;
}


void stopProcess(int id)
{
    kill(id,SIGSTOP); //stopping it to be completed later

}


void resumeProcess(int id){

    // msh 3arfa hena mafrod aghyar el state le running badal blocked wala la2
    kill(id,SIGCONT); //continue the stopped process

}



void FCFS() {}

void SJF(){}
//-------------------------- Highest Priority First --------------------------
void HPF ()
{

    myUsedDS= DS_PrioirtyQ; /// I'll use Priority queue

    struct buff_GenSch process_msg;
    struct processSchedulermsgbuff message;
    int current_turn=0;
    int current_process_index=-1;
    int rec_val;
    int sen_val;
    int first_time=1;
    // first time to recieve ????????? 
    //loop till the queue is empty
     while( Ready_NUm_processes > 0 ||  HPF_Queue_size ==-1   ) // or first time
     {

        rec_val = msgrcv(msgq_id_GenSch, &process_msg, sizeof( process_msg.NewProcess),0, !IPC_NOWAIT); 
            if (rec_val == -1)
            {
                perror("Sch: No new process is ready now");
                continue; //????????????/
            }
            else
            {
                // add the new recieved process to the priority queue
                enqueue_priority( process_msg.NewProcess ,  process_msg.NewProcess.priority);
                PCB_LIST[process_msg.NewProcess.id]. = process_msg.NewProcess ;
                Ready_NUm_processes++;
            }

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
                // inform process file to stop the process
                //PCB change
                PCB_LIST[message.running_process.id].arrival_time ;
                 stopProcess(HPF_Queue[current_turn].myProcess.process_id);

                // stop the current process + save its context switch + start runnig the process with highest priority
                enqueue_priority( message.running_process ,  message.running_process.priority);
                Ready_NUm_processes ++;
            }
            
          // if it was stopped we should make it resume working
            if (PCB_LIST[HPF_Queue[current_turn].myProcess.id].state == waiting )
            {
               resumeProcess(HPF_Queue[current_turn].myProcess.process_id);
               // print resumed at ...

                sen_val = msgsnd(msgq_id_PrcSch, &message,sizeof(message), !IPC_NOWAIT);
                if (sen_val == -1)
                    {
                        perror("error in scheduler sending resumed ");
                    
                    }

            } 
            else
            {
                 message.running_process = HPF_Queue[current_turn].myProcess;
                  sen_val = msgsnd(msgq_id_PrcSch, &message, sizeof(message), !IPC_NOWAIT);
                    if (sen_val == -1)
                    {
                        perror("error in scheduler sending new prcoess ");
                        continue;
                    
                    }
           
                /// run process.c
                    ProcessExecution();
            }     
            //// finished
            /// free pcb element
             
            // remove it from the ready queue ( ready =>> running)
            dequeue_priority();  
            Ready_NUm_processes --; 
          
      } 
    }

    void SRTN(){}


    //------------------------------------------ For Round Robin -------------------------------------------

    /*
    >>>> Steps to be followed inshallah: 
    1- receive the first process then start the loop.
    2- enqueue the received process.
    3- check the remaining time of the current process first, if it's ==0 then dequeue it. 
    4- call the process file and send its pid within the queue.. -->> RESUME ITS WORK
    5- perform the process for a quantum.
    6- check if the remaining time of the current process == 0 , then dequeue it.
    7- else stop the process to start a new one.
    8- dequeue and enqueue it again, if condition 7 isn't performed.
    9- receive a new process if found.
    */

    void RR(int quantum){


        myUsedDS= DS_Queue; /// I'll use queue

        int startingTime=0;
        int currentTime=0;
        struct processSchedulermsgbuff message;

        //1,2- for receiving and the enqueue of the first process 

//     while(!checkRecievedProcess()){

//         // do some logic here to increase the clk or smth till we receive a process
//     }

//     CurrentProcess=instantProcess; //the received process
//     enqueue(RR_processes,CurrentProcess);

//         while(!isEmpty(RR_processes)){ 
            
            
//             currentTime= getClk();

//             message.running_process = CurrentProcess;
//             int sen_val = msgsnd(msgq_id_PrcSch, &message, sizeof(message.running_process), !IPC_NOWAIT);


//             CurrentProcess.process_id= ProcessExecution();

//             // NOT SURE :""""""""""""""""

//             while (currentTime+quantum>getClk()){
//                 int newClk = getClk();
//                 if (newClk != currentTime)
//                 currentTime++;
//                 currentTime = newClk;
//             }
            
//             if(CurrentProcess.remainingtime==0)
//                 dequeue(RR_processes);

//             else {
//                 dequeue(RR_processes);
//                 enqueue(RR_processes,CurrentProcess);
//                 stopProcess(CurrentProcess);
//             }

        
//             if(checkRecievedProcess()){
//                 CurrentProcess=instantProcess; //the received process
//                 enqueue(RR_processes,CurrentProcess);
//             } 
//    }


        // if its the first time for the algorithm or if ready processes has ended
        while (Queue_length==-1 || Ready_NUm_processes >0){

            int process_turn_id;

            if(checkRecievedProcess()!=-1){ // I've received a process
                 
                 //PCB_LIST[]

            }




            
        }
        



}