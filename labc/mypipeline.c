#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <unistd.h>

int main (int argc, char** argv) {
    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    printf("(parent_process>forking…)\n");
    // Fork first child
    int child1 = fork();
    if (child1 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }


    printf("(parent_process>created process with id:%d )\n",child1);
    if (child1 == 0) {
        // Child 1
        printf("(child1>redirecting stdout to the write end of the pipe…)\n");
        close(STDOUT_FILENO); // Close standard output
        dup(fd[1]); // Duplicate write-end of pipe
        close(fd[1]); // Close file descriptor that was duplicated
        close(fd[0]); // Close read-end of pipe

     // Execute "ls -l"
        printf("(child1>going to execute cmd:ls -l)\n");
        char *args[] = {"./bin/ls", "-l", NULL};
        execvp("ls", args);
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    else{
        // Parent process
        printf("(parent_process>closing the write end of the pipe…)\n");
        close(fd[1]); // Close write-end of pipe
    }
    printf("(parent_process>forking…)\n");
    int child2 = fork();
    if (child2 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    

    printf("(parent_process>created process with id:%d )\n",child2);
    if (child2 == 0) {
        // Child 2
        printf( "(child2>redirecting stdin to the read end of the pipe…)\n");
        close(STDIN_FILENO); // Close standard input
        dup(fd[0]); // Duplicate read-end of pipe
        close(fd[0]); // Close file descriptor that was duplicated
        close(fd[1]); // Close write-end of pipe
        printf("(child2>going to execute cmd: tail -n 2)");
    // Execute "tail -n 2"
        char *args2[] = {"/usr/bin/tail", "-n" ,"2",NULL};
        execvp("tail", args2);
        perror("execvp");
        exit(EXIT_FAILURE);
    }
      else{
        // Parent process
        printf("(parent_process>closing the read end of the pipe…)\n");
        close(fd[0]); // Close read-end of pipe
      }
    // Wait for child processes to terminate
    printf( "(parent_process>waiting for child processes to terminate…)\n");
    waitpid(child1, NULL, 0);
    waitpid(child2, NULL, 0);
    printf("(parent_process>exiting…)\n");
    return 0;
      
}
