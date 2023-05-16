#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MSG_SIZE 256

int main(){
	int fd[2];
	char msg[MSG_SIZE];
	if(pipe(fd) == -1){
		perror("error pipe");
		_exit(EXIT_FAILURE);
	}
	int pid = fork();
	if(pid == -1){
		perror("error fork");
		_exit(EXIT_FAILURE);
	}else if(pid == 0){
		char* msg = "hello!";
		close(fd[0]);
		if(write(fd[1], msg, strlen(msg)) == -1){
			perror("error write");
			_exit(EXIT_FAILURE);
		}
		close(fd[1]);
		_exit(EXIT_SUCCESS);
	}else{
		close(fd[1]);
		if(read(fd[0], msg, MSG_SIZE) == -1){
			perror("error write");
			_exit(EXIT_FAILURE);
		}
		printf("Message received from child process: %s\n", msg);
		close(fd[0]);
		_exit(EXIT_SUCCESS);
	}

}
