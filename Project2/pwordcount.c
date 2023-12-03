/* Matthew Voggel COMP 7500 - Project 2 Wordcount. This program's intention is to
 * create two processes that are cooperating via Unix Pipes. Utilizing the
 * two ends of the pipe, we are having a user-read input of a text file, 
 * wherein our parent process reads and loads, and the child process 
 * eventually counts and prints the total words.
 */
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE  250
#define READ_END  0
#define WRITE_END   1

//function created for creating and utilizing two separate pipes, mimicking
//their actions in relation to the processes we eventually initiate 
int pipe_creation (int argc, char *argv[])
{
  int fd[2];
  int val = 0;
  pipe(fd);
  pid_t pid = fork();
  
  //Descriptors of the pipes, where when fork() returns 0, it is child, and
  //when it is the parent, it is equal to the child-pid value
  if (fork() !=0)
    {
      close(fd[0]);
      val = 100;
      write(fd[1], &val, sizeof(val));
      printf("Parent(%d) send value: %d\n", getpid(), val);
      //close our descriptor for write operations
      close(fd[1]); 
    }
  else
    {
      close(fd[1]);
      //demonstrates when the data is being actually read
      read(fd[0], &val, sizeof(val));
      printf("child(%d) received value: %d\n", getpid(), val);

      //close read-descriptor
      //close our descriptor for read operations
      close(fd[0]);
    }
  return 0;
}

//Main function and program
int main (int argc, char *argv[])
{
  printf("program starts\n");
  pid_t pid = fork();
  if (pid > 0)
    //parent denomination to prompt user input for filename to be read
    {
      FILE *fd;
      char *file_name;
      file_name = argv[1];
      printf("Please enter a file to be checked:");
      scanf("%s", file_name);
      //if the system cannot find the file for a number of reasons, this will be returned
      if (fd == NULL)
       {
	 printf("cant open file");
       }
          //Error-checking, unable to open
    if (fd == NULL)
    {
        printf("\nUnable to open file.\n");
        printf("Please check if file exists and you have read privilege.\n");

        exit(EXIT_FAILURE);
    }
    //no file selected
    if (argc !=2) {
      printf("Sample usage, pwordcount <filename>");
      return 0;
    }
    //File cannot be opened 
    if (fd == 0) {
      printf("file %S cannot be opened.", argv[1]);
      return 0;
    }
      else
  	  {
 	    printf("File loaded: '%s'", file_name);
  	  }
      return 0;

    }
  else if (pid == 0)//child
    {
      /* This section is meant for the child process code within the overall set of functions to run.
       * Beyond error-checking, pipe and fork creation, and file loading, I could not get the code to
       * work that would read the information and eventually print it. I am continuing to work on it.
       */
    } 
  else
    {
      //this accounts for the other values indicating the fork failed
      printf("fork() failed!\n");
      return 1; 
    }
  //this is the end of the program
  printf("This is the end of the program\n");
  return 0;

}
