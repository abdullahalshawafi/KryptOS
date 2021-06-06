#include "headers.h"
#include <string.h>
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
    enum STATE state;
    struct process running_process;
};
//---------------------------------------------------------------------------global  declerations -------------------------------------
typedef struct process process;

  int processesNum_schHas =0;
  int index=0; 
  int  msgq_id_PrcSch ;
  int current_process; // el process with highrest
  Queue *RR_processes; 
  process instantProcess;
///----------------------------------------------- MAIN --------------------------------------------------------
int main(int argc, char *argv[])
{
    initClk();
//  mdq queue to talk to process generator
        key_t key1 ;
        key1= ftok("genrator_to_sch", 65); 
        
        msgq_id_GenSch = msgget(key1, 0666 | IPC_CREAT);
        if (  msgq_id_GenSch  == -1)
            {
                perror("Error in create up queue");
                exit(-1);
            }
      //  mdq queue to talk to process file
        key_t key2 ;
        key2 = ftok("processto_sch", 66); 
        msgq_id_PrcSch = msgget(key2, 0666 | IPC_CREAT);
        if (  msgq_id_PrcSch   == -1)
            {
                perror("Error in create up queue");
                exit(-1);
            }
      

/// we will save all ready recived processes in this array
       
     process * ReadyProcesses = (process *)malloc(processesNum * sizeof(process));


 
// still not sure but at least we know it will be working till the process generator stop sending processes to it
// but we didn't cover the process side yet ???
          //  while( processesNum_sent_toSCH < processesNum)
            //{
             
            // if we received a process add it to the array 

             // ReadyProcesses[index] = newPuff.NewProcess;  ///// uncomment meeeee
          
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
       // }
      

    

    //TODO: implement the scheduler.
    //TODO: upon termination release the clock resources.

    destroyClk(true);
}

int ProcessExecution(){

    int pid = fork();

    if (pid  == -1)
        perror("error in fork");


    else if (pid == 0) // running process 
        {
            if(instantProcess.remainingtime>0 && instantProcess.startingTime>0)
                resumeProcess(instantProcess);
            else     
                system("./process.out");
            exit(-1);
        }

    return pid; // return pid to know which child process will be terminated due to an algorithm
}


// need to return boolean value after that for each call in the algorithms to check whether i recieved a process or not

bool checkRecievedProcess(){

// from process generator 
    struct buff_GenSch process_msg;
    rec_process = msgrcv(msgq_id_GenSch, &process_msg, sizeof( process_msg.NewProcess),0, !IPC_NOWAIT); 
    
    if (rec_process == -1){
        perror("Error in receiving process");
        return false;
    }
    instantProcess=process_msg.NewProcess; // make my process equals to the process coming from the msg queue
    return true;
}


void stopProcess(process currentProcess){

    // msh 3arfa hena mafrod aghyar el state le blocked badal running wala la2
    kill(currentProcess.process_id,SIGSTOP); //stopping it to be completed later

}


void resumeProcess(process currentProcess){

    // msh 3arfa hena mafrod aghyar el state le running badal blocked wala la2
    kill(currentProcess.process_id,SIGCONT); //continue the stopped process

}



void FCFS() {}

void SJF(){}

void HPF ()
{
    struct buff_GenSch process_msg;
    struct processSchedulermsgbuff message;
    int current_turn=0;
    int rec_val;
    int sen_val;
    rec_val = msgrcv(msgq_id_GenSch, &process_msg, sizeof( process_msg.NewProcess),0, !IPC_NOWAIT); 
    if (rec_val == -1)
    {
        perror("No new process is ready now");
        return; //?????????????
    }
    else{
        processesNum_schHas ++;
         // add the new recieved process to the priority queue
        enqueue_priority( process_msg.NewProcess ,  process_msg.NewProcess.priority);
    }
   
    // get process with highest priority make it running and send it to the process   
      current_turn= peek_priority();
      current_process = current_turn;
      ProcessExecution();
     message.running_process = HPF_Queue[current_turn].myProcess;
     sen_val = msgsnd(msgq_id_PrcSch, &message, sizeof(message.running_process), !IPC_NOWAIT);

     
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

    process CurrentProcess;
    int startingTime=0;
    int currentTime=0;
    struct processSchedulermsgbuff message;

    //1,2- for receiving and the enqueue of the first process 

   while(!checkRecievedProcess()){

       // do some logic here to increase the clk or smth till we receive a process
   }

   CurrentProcess=instantProcess; //the received process
   enqueue(RR_processes,CurrentProcess);

    while(!isEmpty(RR_processes)){ 
        
        
        currentTime= getClk();

        message.running_process = CurrentProcess;
        int sen_val = msgsnd(msgq_id_PrcSch, &message, sizeof(message.running_process), !IPC_NOWAIT);


        CurrentProcess.process_id= ProcessExecution();

        // NOT SURE :""""""""""""""""

        while (currentTime+quantum>getClk()){
            int newClk = getClk();
            if (newClk != currentTime)
              currentTime++;
            currentTime = newClk;
        }
        
        if(CurrentProcess.remainingtime==0)
            dequeue(RR_processes);

        else {
            dequeue(RR_processes);
            enqueue(RR_processes,CurrentProcess);
            stopProcess(CurrentProcess);
        }

       
        if(checkRecievedProcess()){
             CurrentProcess=instantProcess; //the received process
             enqueue(RR_processes,CurrentProcess);
        } 
   }



}