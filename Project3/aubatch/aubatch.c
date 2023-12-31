/*
* Project 3: Aubatch.c - A Batch Scheduling System - Matthew Voggel
*
* Xiao Qin - COMP7500
* Department of Computer Science and Software Engineering
* Auburn University
* Feb. 20, 2018. Version 1.1
* Most recently incorporated, and edited by Matthew Voggel on 3.17.23
*
* This sample source code demonstrates the development of
* a batch-job scheduler using pthread.
*
* Compilation Instruction:
* gcc aubatch.c -o test -lpthread
* ./aubatch
*
* Learning Objecties:
* 1. To compile and run a program powered by the pthread library
* 2. To create two concurrent threads: scheduling and dispatching
* 3. To execute jobs in the AUbatch system by the dispatching thread
* 4. To synchronize the two concurrent threads using condition variables
*/

#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "aubatch.h" //header file for aubatch methods

//MAIN METHOD
int main(int argc, char *argv[]) {
    initNewQueue();
    pthread_t threads[3];
    pthread_attr_t attr;

    /* This is where we INITIALIZE OUR MUTEX AND CONDITION VARIABLES 
     * The second set builds out buffer space for producer 
     * and dispatcher to send and receive the information into the queue.
     * Third set is where they're joined again after being threaded, and then 
     * cleaning them up so that the threads aren't sitting idly
     * them for a potential next batch job. 
     */
    pthread_mutex_init(&cmd_queue_lock, NULL);
    pthread_cond_init(&cmd_buf_not_full, NULL);
    pthread_cond_init(&cmd_buf_not_empty, NULL);

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&threads[1], &attr, scheduler, NULL);
    pthread_create(&threads[2], &attr, dispatcher, NULL);

    pthread_join(threads[1], NULL);
    pthread_join(threads[2], NULL);

    pthread_attr_destroy(&attr);
    pthread_mutex_destroy(&cmd_queue_lock);
    pthread_cond_destroy(&cmd_buf_not_empty);
    pthread_exit(NULL);
}

//INITIALIZE NEW QUEUE UTILIZING ABOVE COND./MUTEX VARIABLES 
//(LOCK AND UNLOCK SEQUENCE (for critical sections)
//Sets our max values for Job #, CPU Time, Priority settings
void initNewQueue() {
    pthread_mutex_lock(&cmd_queue_lock);
    for (int i = 0; i < MAX_JOB_NUMBER; i++) {
        strcpy(new_queue[i].name, "initialize");
        new_queue[i].cputime = 250;
        new_queue[i].priority = 250;
    }
    pthread_mutex_unlock(&cmd_queue_lock);
}


//PRODUCER PROCESSES
void *scheduler() {
    printf("\n ***********************************************************\nWelcome to the batch process scheduler, 'AUBATCH,' for COMP7500. \nYou can start by typing in a command if you know it, or typing in 'help' to see the menu of items.\n ***********************************************************\n");
    commandline();
    pthread_exit(NULL);
}

void schedule(struct Job *job) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    int tempHead;
    (*job).arrival_time = t;
    (*job).time = *tm;
    //Pthread locking and unlocking for the scheduler
    pthread_mutex_lock(&cmd_queue_lock);
    pthread_cond_signal(&cmd_buf_not_full);
    count++;
    new_queue[head] = *job;
    head++;
    if (head == MAX_JOB_NUMBER) {
        head = 0;
    }
    //POLICY OUTLINES - CRITICAL SECTION
    switch (scheduleType) {
        case SJF:
            tail = 0;
            insertSortSJF(&tempHead);
            head = tempHead;
            break;
        case PRI:
            insertSortPri(&tempHead);
            tail = 0;
            head = tempHead;
            break;
        case FCFS:
        default:
            break;
    }

    pthread_mutex_unlock(&cmd_queue_lock);
    return;
}


/*Dispatcher AND execute method */
void *dispatcher() {
    JobPtr currentJobPtr = (JobPtr) malloc(sizeof(struct Job));
    JobPtr completedJobPtr = (JobPtr) malloc(sizeof(struct Job));
    JobPtr jobPtr = (JobPtr) malloc(sizeof(struct Job));
    pthread_mutex_lock(&cmd_queue_lock);
    while (command != end) {
        if (count == 0) {
	  pthread_cond_wait(&cmd_buf_not_full, &cmd_queue_lock);
            if (count == 0) {
                pthread_exit(NULL);
            }
        }
        count--;
        if (strcmp(new_queue[tail].status, "wait") == 0) {
            execute(currentJobPtr, completedJobPtr, jobPtr);
        } else {
            inc_reset_tail();
        }
        if (testRunning == 1 && count == 0) {
            printf("Benchmark Testing has Completed\n");
            printf("Press Enter to see Info:\n");
            testDone = 1;
        }
    }
    pthread_exit(NULL);
}
//Runs and Completes the jobs based on the time
void execute(JobPtr currentJobPtr, struct Job *completedJobPtr, JobPtr jobPtr) {
    strcpy(new_queue[tail].status, "run");
    RunningJob = currentJobPtr;
    *jobPtr = new_queue[tail];
    memcpy(currentJobPtr, jobPtr, sizeof(struct Job));
    clearJob(jobPtr);
    new_queue[tail] = *jobPtr;
    inc_reset_tail();

    pthread_mutex_unlock(&cmd_queue_lock);

    time_t start = time(NULL);
    dispatch(currentJobPtr);
    time_t end = time(NULL);
    int cpu = end - start;

    strcpy((*currentJobPtr).status, "complete");
    (*currentJobPtr).actualCpuTime = cpu;
    (*currentJobPtr).turnarount_time = end - (*currentJobPtr).arrival_time;
    (*currentJobPtr).wait_time = (*currentJobPtr).turnarount_time - cpu;
    memcpy(completedJobPtr, currentJobPtr, sizeof(struct Job));

    insertFilledOutQueue(completedJobPtr);
}
//Dispatching of the jobs within the batch
void dispatch(struct Job *job) {
    pid_t pid, c_pid;
    int status;
    char my_job[10];
    char buffer[3];
    sprintf(buffer, "%d", (*job).cputime);

    if (strcmp(job->name, "batch_job") == 0) {
      sprintf(my_job, "%s", (*job).name);
    } else {
        sprintf(my_job, "%s", "batch_job");
    }
    char *const parmList[] = {my_job, buffer, NULL};
    if ((pid = fork()) == -1)
        perror("fork error");
    else if (pid == 0) {
        execv(my_job, parmList);
    } else if (pid > 0) {
        //from manual page of https://linux.die.net/man/2/wait
        do {
            c_pid = waitpid(pid, &status, WUNTRACED | WCONTINUED);
            if (c_pid == -1) {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
}

//FUNCTION TO CALCULATE JOB STATISTICS
//Went with local, short term memory variables of time here for the given job queue per batch
void runJobInfo()        {
    float tempTurn = 0;
    float tempWait = 0;
    float tempCpu = 0;
    int tempCount = 0;
    struct filledOutQueue *temp = chead;
    if (temp == NULL) {}
    else {
        while (temp != NULL) {
	  tempCpu += (*temp).job.cputime;
	  tempWait += (*temp).job.wait_time;
	  tempTurn += (*temp).job.turnarount_time;
	  tempCount++;
	  temp = (*temp).next_job;
        }
        totalcount = tempCount;
        if (totalcount == 0) {}
        else {
            avgTurnaround = tempTurn / totalcount;
            avgWaitTime = tempWait / totalcount;
            avgCpuTime = tempCpu / totalcount;
            totalCompletedJobs = totalcount;
            avgThroughput = (1 / (avgTurnaround / totalcount));
        }
    }
}


/*Command Line and Parser, setting initial max buffer size to be used.*/
void commandline() {
    printf("\n>");
    char *buffer;
    char *commandv[7] = {NULL};
    size_t bufsize = 32;
    int commandc = -1;
    buffer = (char *) malloc(bufsize * sizeof(char));
    enum Commands command = init;
    int tempHead;
    do {
        if (testRunning == 1 && count > 0) {
            printf("\nYour Test is Currently Running\n");
            printf("A few commands work while this is ongoing: list , help , quit\n");
            printf("\n>");
        }
        if (testDone == 1) {
            printf("Your Test is Complete!\n");
            printStats();
            printf("\n>");
            testDone = 0;
        }
        fgets(buffer, bufsize, stdin);
        commandParser(buffer, commandv, &commandc);
	//DIFFERENT COMMANDS TO BE EXECUTED
	//COMMANDS TO BE USED/NOT USED DURING EXECUTION ALSO LISTED HERE,
	//INCLUDING THEIR ERROR MESSAGING
        if (commandv[0] != NULL) {
            if (strcmp(commandv[0], "help") == 0) {
                help_command(commandv[1], commandc);
            } else if (strcmp(commandv[0], "run") == 0) {
                if (testRunning != 1) {
                    run_command(commandv, commandc);
                } else { printf("Command not accepted while test is running."); }
            } else if (strcmp(commandv[0], "quit") == 0 || strcmp(commandv[0], "exit") == 0) {
                quit_command();
                testRunning = 0;
            } else if (strcmp(commandv[0], "sjf") == 0) {
                if (testRunning != 1) {
                    sjf_command(&command, &tempHead);
                } else { printf("Command not accepted while test is running."); }
            } else if (strcmp(commandv[0], "pri") == 0) {
                if (testRunning != 1) {
                    pri_command(&command, &tempHead);
                } else { printf("Command not accepted while test is running."); }
            } else if (strcmp(commandv[0], "list") == 0) {
                list_command(commandv[1], commandc);
            } else if (strcmp(commandv[0], "fcfs") == 0) {
                if (testRunning != 1) {
                    fcfs_command(&command, &tempHead);
                } else { printf("Command not accepted while test is running."); }
            } else if (strcmp(commandv[0], "test") == 0) {
                test_command(commandv, bufsize, commandc);
            } else if (strcmp(commandv[0], "job") == 0 || strcmp(commandv[0], "job") == 0) {
                printf("Error: No Command Job has been input.\n");
                printf("run <job> <time> <pri>: submit a job named <job>,\n");
            } else {
                printf("Error: Please Check your Input Parameters.\n");
            }
        }
        if (count == 0 && testDone == 1) {
            testRunning = 0;
            runJobInfo();
            printFileStats(fp);
            command = test;
            if (fp) {
                fclose(fp);
            }
        }
        printf("\n>");
    } while (command != end);
    return;
}

//HELP COMMAND TO PULL UP MENU OF OPTIONS 
void help_command(char *commandv, int commandc) {
    if (commandc == 1) {
        helpInfo();
    } else if (commandc == 2) {
        if (strcmp(commandv, "-list") == 0 || strcmp(commandv, "list") == 0 || strcmp(commandv, "-l") == 0) {
            printf("list: display the job status.\n");
        } else if (strcmp(commandv, "run") == 0 || strcmp(commandv, "-run") == 0 || strcmp(commandv, "-r") == 0) {
            printf("run <job> <time> <pri>: submit a job named <job>,\n");
        } else if (strcmp(commandv, "pri") == 0 || strcmp(commandv, "-p") == 0 || strcmp(commandv, "-pri") == 0) {
            printf("Scheduling policy:\n");
            printf("\tpriority: change the scheduling policy to priority.\n");
        } else if (strcmp(commandv, "fcfs") == 0 || strcmp(commandv, "-f") == 0 || strcmp(commandv, "-fcfs") == 0) {
            printf("Scheduling policy:\n");
            printf("\tfcfs: change the scheduling policy to FCFS.\n");
        } else if (strcmp(commandv, "sjf") == 0 || strcmp(commandv, "-s") == 0 || strcmp(commandv, "-sjf") == 0) {
            printf("Scheduling policy:\n");
            printf("\tsjf: change the scheduling policy to SJF.\n");
        } else if (strcmp(commandv, "test") == 0 || strcmp(commandv, "-test") == 0 || strcmp(commandv, "-t") == 0) {
            printf("Test Command:\n");
            printf("test <benchmark> <policy> <num_of_jobs> <priority_levels>\n\t<min_CPU_time> <max_CPU_time>");
        } else if (strcmp(commandv, "quit") == 0 || strcmp(commandv, "-quit") == 0 || strcmp(commandv, "-q") == 0) {
            printf("Quit Command:Exit the Program.\n");
        }
    } else {
        printf("Error: Help Command.\n");
        printf("help list, || run , sjf , pri , fcfs");
        printf("help -l,   || run , sjf , pri , fcfs");
    }
}


/**************PRINTING OF THE JOBS, POLICY, AND STATUS***********/
//using temporary storage of the count of jobs, policy, etc. (e.g. tempCount)
//
void printNewQueue() {
    int tempCount;
    printf("Jobs still in the queue: %d\n", count);
    char tempStr[5];
    switch (scheduleType) {
        case FCFS:
            strcpy(tempStr, "FCFS");
            break;
        case SJF:
            strcpy(tempStr, "SJF");
            break;
        case PRI:
            strcpy(tempStr, "PRI");
            break;
    }
    //The info needing to be provided for the actual test
    printf("Policy being used: %s.\n", tempStr);
    if (RunningJob != NULL) {
      if (strcmp((*RunningJob).status, "DONE!") != 0) {
            printf("Job that is in Progress:\n");
            printf("Name \t\tCPU_Time \tPri \tProgress\n");
            printf("%s %10d %14d %12s\n", (*RunningJob).name, (*RunningJob).cputime,
                   (*RunningJob).priority,
                   (*RunningJob).status);
            printf("\n");
        } else {

        }
    }
    //status information for ongoing test
    for (int i = 0; i < MAX_JOB_NUMBER; i++) {
        if (strcmp(new_queue[i].status, "") == 0 || strcmp(new_queue[i].status, "complete") == 0 ||
            strcmp(new_queue[i].status, "init") == 0) {
            continue;
        } else {
            printf("Ready Queue:\n");
            printf("Name \t\t\tCPU_Time \tPri \tArrival_time \tProgress\n");
            printf("%s %20d %10d %10d:%d:%d %18s\n", new_queue[i].name, new_queue[i].cputime,
                   new_queue[i].priority,
                   new_queue[i].time.tm_hour, new_queue[i].time.tm_min, new_queue[i].time.tm_sec,
                   new_queue[i].status);
        }
    }
    tempCount++;
}
/****************END OF JOB PRINTING METHODS *************/


/****************STATS METHODS DEFINITIONS *************/
//These are printing to different locations, one to the output of tests,
//the other is output directly to the file.
void printFileStats(FILE *fp) {
    fprintf(fp, "Jobs Tested: %d\n", totalcount);
    fprintf(fp, "Turnaround time Avg.: %f seconds\n", avgTurnaround);
    fprintf(fp, "CPU Time Avg.:%f seconds\n", avgCpuTime);
    fprintf(fp, "Wait Time Avg.:%f seconds\n", avgWaitTime);
    fprintf(fp, "Throughput: %f per second\n", avgThroughput);
}
void printStats() {
    printf("\n Jobs tested: %d\n", totalcount);
    printf("Jobs completed: %d\n", totalCompletedJobs);
    printf("Turnaround time Avg.: %f seconds\n", avgTurnaround);
    printf("CPU Time Avg.:%f seconds\n", avgCpuTime);
    printf("Wait Time Avg.:%f seconds\n", avgWaitTime);
    printf("Throughput: %f per second\n", avgThroughput);
    return;
}
//'list' command table, showcases completed/ongoing work, and the information attached
void printFilledOutQueue() {
    int tempCount;
    struct filledOutQueue *temp = chead;
    if (temp == NULL) {}
    else {
        printf("Completed Jobs:\n");
        printf("Name \t\t\tCPU_Time \tPri \tArrival_time \tProgress\n");
        while (temp != NULL) {
	  printf("%s %20d %10d %10d:%d:%d %12s\n", (*temp).job.name, (*temp).job.cputime,
		 (*temp).job.priority,
		 (*temp).job.time.tm_hour, (*temp).job.time.tm_min, (*temp).job.time.tm_sec,
		 (*temp).job.status);
	  temp = (*temp).next_job;
        }
    }
}
/****************END OF STAT  METHODS *************/


//CREATION/INSERTION OF NEW, FILLED OUT QUEUE, TO BE PROCESSED:
void insertFilledOutQueue(struct Job *job) {
  struct filledOutQueue *job_node = (struct filledOutQueue *) malloc(sizeof(struct filledOutQueue));
    struct filledOutQueue *last = chead;
    (*job_node).job = *job;
    (*job_node).next_job = NULL;
    if (chead == NULL) {
        chead = job_node;
        return;
    }
    while ((*last).next_job != NULL) {
      last = (*last).next_job;
    }
    (*last).next_job = job_node;
    return;
}

/******Methods that clean all of our information out from previous jobs and tests******/
//CLEANS OUT PREVIOUS JOBS AS WELL AS STATS, THREADS, AND QUEUE
//ALLOWS TO RUN ANOTHER JOB AS THE USER PLEASES.RESETS POINTERS, TOO
void clearJob(struct Job *job) {
  strcpy((*job).name, "");
  //set maxes
  (*job).cputime = 250;
  (*job).priority = 250;
  (*job).arrival_time = 0;
  (*job).turnarount_time = 0;
  (*job).wait_time = 0;
  strcpy((*job).status, "init");
  (*job).number = 0;
}

void clearFilledOutQueue() {
    struct filledOutQueue *next;
    struct filledOutQueue *current = chead;
    struct filledOutQueue *temp = NULL;
    while (current != NULL) {
      next = (*current).next_job;
        free(current);
        current = next;
    }
    chead = temp;
};
//Setting all datapoints back to zero
void clearStats() {
    avgTurnaround = 0;
    avgWaitTime = 0;
    avgCpuTime = 0;
    totalCompletedJobs = 0;
    avgThroughput = 0;
    totalcount = 0;
}
//Setting tail pointer value back to zero too
void inc_reset_tail() {
    tail++;
    if (tail == MAX_JOB_NUMBER) {
        tail = 0;
    }
}
/*******************************RESET/CLEAN**********************************/

//MENU INFORMATION FOR WHEN THIS IS LOADED UP FOR THE FIRST TIME.
//INCLUDES INFORMATION ON COMMANDS THAT A USER CAN USE
void helpInfo() {
    command = help;
    printf("Help Menu and Commands to Run:\n");
    printf("Single Job: 'run batch_job <CPUtime> <priority>'\n");
    printf("list: Show the list of current job statuses.\n");
    printf("Change Scheduling policy:\n");
    printf("\tfcfs: change to First Come First Serve.\n");
    printf("\tsjf: change to Shortest Job First.\n");
    printf("\tpri: change to Priority.\n");
    printf("Run batch test: <benchmark> <policy> <num_of_jobs> <priority_levels>\n\t<min_CPU_time> <max_CPU_time>.\n E.g. test mybenchmark fcfs 4 8 15 30\n");
    printf("Quit Program: 'quit'\n");
    return;
}

/*POLICY OUTLINES (SCHEDULING TYPES FOR FCFS, SJF, PRIORITY*/
//ALSO INCLUDES LIST OF COMMANDS TO USE LIKE QUIT, RUN, LIST...
void fcfs_command(enum Commands *command, int *tempHead) {
    (*command) = fcfs;
    scheduleType = FCFS;
    insertSortFCFS(tempHead);
    tail = 0;
    head = (*tempHead);
}
void pri_command(enum Commands *command, int *tempHead){
    (*command) = pri;
    scheduleType = PRI;
    insertSortPri(tempHead);
    tail = 0;
    head = (*tempHead);
}
void sjf_command(enum Commands *command, int *tempHead) {
    (*command) = sjf;
    scheduleType = SJF;
    insertSortSJF(tempHead);
    tail = 0;
    head = (*tempHead);
}
void quit_command() {
  //When a user quits out, it will display all of the tests that have been run in a session
    runJobInfo();
    printf("\n Jobs tested: %d\n", totalcount);
    printf("Jobs completed: %d\n", totalCompletedJobs);
    printf("Turnaround time Avg.: %f seconds\n", avgTurnaround);
    printf("CPU Time Avg.:%f seconds\n", avgCpuTime);
    printf("Wait Time Avg.:%f seconds\n", avgWaitTime);
    printf("Throughput: %f per second\n", avgThroughput);
    command = end;
    pthread_cond_signal(&cmd_buf_not_full);
    pthread_exit(NULL);
}
void run_command(char *const *commandv, int commandc) {
    command = run;
    if (commandc < 4 || commandc > 4) {
        printf("Arguments Error: Too many or too little number of arguments included.  %d of 4 parameters entered.\n", commandc);
        printf("Reminder: To run a single job: 'run batch_job <CPUtime> <priority>'\n");
    } else {
        send_job(commandv);
    }
}
void send_job(char *const *commandv) {
    JobPtr job = createJob(commandv);
    strcpy((*job).status, "wait");
    schedule(job);
}
//SUPPORT FOR SINGLE JOB CREATION, including error messaging.
JobPtr createJob(char *const *commandv) {
    JobPtr job = (JobPtr) malloc(sizeof(struct Job));
    char my_job[10];
    strcpy((*job).name, commandv[1]);
    (*job).cputime = atoi(commandv[2]);
    (*job).priority = atoi(commandv[3]);
    if (strcmp((*job).name, "batch_job") == 0) {
      sprintf(my_job, "%s", (*job).name);
    } else {
        if (fp) {
            fprintf(fp, "For single job input, this only supports the ./batch_job program\n");
            fprintf(fp, "%s will be replaced with ./batch_job program\n in your list", (*job).name);
        }
	//Seems counterintuitive that "single" job can only be called "batch" job, but
	//its just what I ended up calling my separate .c program.
        printf("For single job input, this only supports the ./batch_job program\n");
        printf("%s will be replaced with ./batch_job program\n in your list", (*job).name);
        sprintf(my_job, "%s", "batch_job");
    }
    (*job).wait_time = 0;
    (*job).arrival_time = 0;
    (*job).turnarount_time = 0;
    (*job).number = totalcount++;
    return job;
}
void list_command(char *commandv, int commandc) {
    command = list;
    if (commandc == 2) {
        if (strcmp(commandv, "-r") == 0) {
            while (count > 0) {
                printNewQueue();
                printFilledOutQueue();
                sleep(2);
            }
        } else {
            printNewQueue();
            printFilledOutQueue();
        }
    } else {
        printNewQueue();
        printFilledOutQueue();
    }
}
//***********************END OF LIST OF COMMANDS **************************


void test_command(char *const *commandv, size_t bufsize, int commandc) {
    command = test;
    //Requires that there's 7 paramenters - test, benchmark, scheduler, and the 4 CPU needs
    if (commandc < 7 || commandc > 7) {
        printf("Error: Test Command.\n");
        printf("test <benchmark> <policy> <num_of_jobs> <priority_levels>\n");
        printf("\t<min_CPU_time> <max_CPU_time>");
    } else {
        printf("Warning: Test Command.\n");
        printf("Previous Statistics and Queue will be cleared.\n");
        printf("Only list and Help command will be allowed to be entered.\n");
        testRunning = 1;
        clearFilledOutQueue();
        initNewQueue();
        clearStats();
        char filename[25];
        struct Benchmark *benchmark;
        JobPtr job = (JobPtr) malloc(sizeof(struct Job));
        benchmark = (struct Benchmark *) malloc(bufsize * sizeof(struct Benchmark));
        strcpy((*benchmark).name, commandv[1]);
        strcpy((*benchmark).sType, commandv[2]);
        sprintf(filename, "%s_%s.txt", (*benchmark).name, (*benchmark).sType);
        fp = fopen(filename, "w");
        if (strcmp((*benchmark).sType, "fcfs") == 0) {
            scheduleType = FCFS;
        } else if (strcmp((*benchmark).sType, "pri") == 0) {
            scheduleType = PRI;
        } else if (strcmp((*benchmark).sType, "sjf") == 0) {
            scheduleType = SJF;
        }
        (*benchmark).num_of_jobs = atoi(commandv[3]);
        (*benchmark).priority_levels = atoi(commandv[4]);
        (*benchmark).min_CPU_time = atoi(commandv[5]);
        (*benchmark).max_CPU_time = atoi(commandv[6]);
        int pri_level = (*benchmark).priority_levels;
        int max_cpu = (*benchmark).max_CPU_time;
        int min_cpu = (*benchmark).min_CPU_time;
        int randBurst;
        srand(time(NULL));
        fprintf(fp, "Submitted Test Jobs: %s\n",(*benchmark).sType);
        fprintf(fp, "Number of Jobs:%d,Priority Levels:%d,Min CPU:%d Max CPU:%d\n",(*benchmark).num_of_jobs,(*benchmark).priority_levels,(*benchmark).min_CPU_time,(*benchmark).max_CPU_time);
        fprintf(fp, "Name \t\tCPU_Time \tPri \tProgress\n");
        for (int i = 0; i < (*benchmark).num_of_jobs; i++) {
            if ((max_cpu - min_cpu) == 0) {
                randBurst = rand() % max_cpu + 1;
            } else {
                randBurst = min_cpu + rand() % (max_cpu - min_cpu);
            }
            int randPri = rand() % pri_level + 1;
            strcpy((*job).name, "batch_job");
            (*job).cputime = randBurst;
            (*job).priority = randPri;
            (*job).wait_time = 0;
            (*job).arrival_time = 0;
            (*job).turnarount_time = 0;
            (*job).number = totalcount++;
            strcpy((*job).status, "wait");
            schedule(job);
            fprintf(fp, "%s %10d %14d %12s\n", (*job).name, (*job).cputime, (*job).priority, (*job).status);
        }
        free(benchmark);
    }
}
//COMMAND PARSER OF SEVERAL ARGUMENTS PASSED IN 
void commandParser(char *argument, char *param[], int *paramSize) {
    char *arg;
    int i = 0;
    argument[strcspn(argument, "\r\n")] = 0;
    arg = strtok(argument, " ");
    param[i] = arg;
    while (arg != NULL) {
        i++;
        arg = strtok(NULL, " ");
        param[i] = arg;
    }
    *paramSize = i;
    return;

}
//ACTUAL SORT COMMANDS FOR INCOMING BATCH JOB FOR EACH POLICY TYPE
void insertSortSJF(int *currentHead) {
    int j;
    JobPtr tempJob = (JobPtr) malloc(sizeof(struct Job));
    for (int i = 0; i < MAX_JOB_NUMBER; i++) {
        j = i;
        while (j > 0 && new_queue[j].cputime < new_queue[j - 1].cputime) {
            *tempJob = new_queue[j];
            new_queue[j] = new_queue[j - 1];
            new_queue[j - 1] = *tempJob;
            j--;
        }
    }
    j = 0;
    for (int i = 0; i < MAX_JOB_NUMBER; i++) {
        if (new_queue[i].cputime != 250) {
            j++;
        }
    }
    *currentHead = j;

    free(tempJob);
}
void insertSortPri(int *currentHead) {
    int j;
    JobPtr tempJob = (JobPtr) malloc(sizeof(struct Job));
    for (int i = 0; i < MAX_JOB_NUMBER; i++) {
        j = i;
        while (j > 0 && new_queue[j].priority < new_queue[j - 1].priority) {
            *tempJob = new_queue[j];
            new_queue[j] = new_queue[j - 1];
            new_queue[j - 1] = *tempJob;
            j--;
        }
    }
    j = 0;
    for (int i = 0; i < MAX_JOB_NUMBER; i++) {
        if (new_queue[i].priority != 250) {
            j++;
        }
    }
    *currentHead = j;

    free(tempJob);
}
void insertSortFCFS(int *currentHead) {
    int j;
    JobPtr tempJob = (JobPtr) malloc(sizeof(struct Job));
    for (int i = 0; i < MAX_JOB_NUMBER; i++) {
        j = i;
        while (j > 0 && new_queue[j].number < new_queue[j - 1].number) {
            *tempJob = new_queue[j];
            new_queue[j] = new_queue[j - 1];
            new_queue[j - 1] = *tempJob;
            j--;
        }
    }
    j = 0;
    for (int i = 0; i < MAX_JOB_NUMBER; i++) {
        if (new_queue[i].number != 250) {
            j++;
        }
    }
    *currentHead = j;
    free(tempJob);
}
//****************END OF POLICY DESCRIPTIONS **************





