/*
* Matthew Voggel - COMP7500 - DR.QIN  
* AUBatch.h header file for structure definitions 
* and variable outlines for use in the main program
* for the scheduling system
* 
*****Pthread Condition and Mutex variables!!!******
*These are used and outlined at the top of the main source
*code in aubatch.c, as well as the producer and consumer
*functions. These are the pthread essentials to the whole
*assignment. Other pthread library elements are used, too, but 
*not needed to include in header per Piazza questions.
*/

#ifndef AUBATCH_AUBATCH_H
#define AUBATCH_AUBATCH_H
#define MAX_JOB_NUMBER  100

pthread_mutex_t cmd_queue_lock;
pthread_cond_t cmd_buf_not_full;
pthread_cond_t cmd_buf_not_empty;

//Job structure information for scheduling times 
struct Job {
    char name[50];
    int cputime;
    int actualCpuTime;
    int priority;
    time_t arrival_time;
    float turnarount_time;
    float wait_time;
    struct tm time;
    char status[10];
    int number;
};
//DIFFERENT PRIORITY SETS TO USE
//Priority, First come first serve, shortest job first
enum Scheduler {
  PRI, FCFS, SJF
};

//COMMANDS TO USE WITHIN THE SCHEDULER
//LISTED FROM 'HELP' MENU
enum Commands {
  init, help, run, test, list, end, fcfs, sjf, pri
};

//init is another definition of "command" under commandline
//and parser methods. Save some time. 
enum Commands command = init;
enum Scheduler prev_scheduleType = FCFS;
enum Scheduler scheduleType = FCFS;

//BASELINE DATA TO COLLECT FROM BENCHMARK TESTING
struct Benchmark {
    char name[50];
    char sType[50];
    int num_of_jobs;
    int priority_levels;
    int min_CPU_time;
    int max_CPU_time;
};
struct Job new_queue[MAX_JOB_NUMBER];
struct filledOutQueue {
    struct Job job;
    struct filledOutQueue *next_job;
};
struct filledOutQueue *chead = NULL;
struct Job *RunningJob = NULL;
typedef struct Job *JobPtr;

//DEFINITIONS FOR MATH OPERATIONS ON FINAL STATS PRINTOUT
int totalcount = 0;
int testDone = 0;
int count = 0;
int head = 0;
int tail = 0;
int testRunning = 0;
float avgTurnaround = 0;
float avgWaitTime = 0;
float avgCpuTime = 0;
int totalCompletedJobs = 0;
float avgThroughput = 0;
FILE *fp;


/***************FUNCTION REFERENCES******************/
//QUEUE-MAKING FUNCTIONS, INCLUDES THREAD COMMANDS TOO
void initNewQueue();
void runJobInfo();
void clearJob(struct Job *job);
void clearFilledOutQueue();
void clearStats();
void inc_reset_tail();
//PRINT FUNCTIONS
void printNewQueue();
void printStats();
void printFilledOutQueue();
void printFileStats(FILE *fp);

//PRODUCER AND SCHEDULER METHODS - includes commandparser
void *scheduler();
void schedule(struct Job *job);
void helpInfo();
void commandline();
void commandParser(char *argument, char *param[], int *paramSize);
void insertSortSJF(int *currentHead);
void insertSortPri(int *currentHead);
void insertSortFCFS(int *currentHead);

//CONSUMER AND DISPATCHER METHODS
void *dispatcher();
void dispatch(struct Job *job);
void insertFilledOutQueue(struct Job *job);
void execute(JobPtr currentJobPtr, struct Job *completedJobPtr, JobPtr jobPtr);

//MAIN COMMANDS WITHIN THE SYSTEM
void run_command(char *const *commandv, int commandc);
void quit_command();
void sjf_command(enum Commands *command, int *tempHead);
void pri_command(enum Commands *command, int *tempHead);
void fcfs_command(enum Commands *command, int *tempHead);
void list_command(char *commandv, int commandc);
void help_command(char *commandv, int commandc);
void inc_reset_tail();
void test_command(char *const *commandv, size_t bufsize, int commandc);
void send_job(char *const *commandv);
JobPtr createJob(char *const *commandv);

  
#endif //AUBATCH_AUBATCH_H

