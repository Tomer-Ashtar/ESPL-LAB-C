#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "LineParser.h"
#include <linux/limits.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h> 
#include <fcntl.h>
#include <signal.h>

 #define TERMINATED  -1
 #define RUNNING 1
 #define SUSPENDED 0
 #define HISTLEN 20
   
typedef struct process{
        cmdLine* cmd;                         /* the parsed command line*/
        pid_t pid; 		                  /* the process id that is running the command*/
        int status;                           /* status of the process: RUNNING/SUSPENDED/TERMINATED */
        struct process *next;	                  /* next process in chain */
    } process;
    
process* Gprocess_list;
char* history[HISTLEN];
int size_of_history = 0;

 process* makeProcess(cmdLine* cmd, pid_t pid){
 	process* newProcess = malloc(sizeof(struct process));
 	newProcess->cmd = cmd;
 	newProcess-> pid = pid;
 	newProcess->status = RUNNING;
 	newProcess->next = NULL;
 	return newProcess;
 }
 
 void free_history(){
	int i;
	for(i=0; i<size_of_history ;i++){
		free(history[i]);
	}
}
 
void freeProcessList(process* process_list){
 	process* Cprocess_list = process_list;
 	if(Cprocess_list != NULL){
 		freeProcessList(Cprocess_list->next);
 		freeCmdLines(Cprocess_list->cmd);

 		free(Cprocess_list);
 	}
 }

void freeProcess(process* p){
	freeCmdLines(p->cmd);
	free(p);
}

void updateProcessStatus(process* process_list, int pid, int status){
	if(process_list != NULL && process_list->pid == pid){
		if(status == -1){
			process_list->status = TERMINATED;
		}else if(status == 0){
			process_list->status = SUSPENDED;
		}else if(status == 1){
			process_list->status = RUNNING;
		}
	}
} 

void updateProcessStatusID(process* process_list ,int pid, int status){
	while(process_list != NULL && process_list->pid != pid){
		process_list = process_list->next;
	}
	if(process_list != NULL){
		if(status == -1){
			process_list->status = TERMINATED;
		}else if(status == 0){
			process_list->status = SUSPENDED;
		}else if(status == 1){
			process_list->status = RUNNING;
		}
	}
}

 
void updateProcessList(process **process_list){
	process* curr_process = (*process_list);
	while(curr_process != NULL){
		int status;
		int ret = waitpid(curr_process->pid,&status,WNOHANG);
		if(ret > 0){
			updateProcessStatus(curr_process,curr_process->pid,-1);
		}
		curr_process = curr_process->next;
	}
}
 
 void addProcess(process** process_list, cmdLine* cmd, pid_t pid){
 	process* newProcess = makeProcess(cmd,pid);
 	newProcess->next = *process_list;
 	*process_list = newProcess;
 } 
 

int deleteDead(process** process_list){
	process* curr = (*process_list);
	if(curr == NULL){return 0;}
	while(curr->next!=NULL){
		process* next = curr->next;
		if(next->status == TERMINATED){
			curr->next = next->next;
			freeProcess(next);

		}else{
			curr = curr->next;
		}
	}
	curr = (*process_list);
	if(curr->status == TERMINATED){
		*process_list = curr->next;
		freeProcess(curr);

	}
	return 1;
}

void printProcessList(process** process_list){
	updateProcessList(process_list);
	printf("%-15s %-15s %-15s\n","PID","Command","STATUS");
	process* curr = *process_list;
	while(curr != NULL){
		char* status;
		char command[256]="";
		if(curr->status == TERMINATED){
			status = "TERMINATED";
		}else if(curr->status == SUSPENDED){
			status = "SUSPENDED";
		}else{
			status = "RUNNING";
		}
		int i;
		for(i=0; i<curr->cmd->argCount;i++){
			const char* arg= curr->cmd->arguments[i];
			strcat(command,arg);
			strcat(command, " ");
		}
		printf("%-15d %-15s %-15s\n",curr->pid,command,status);
		curr = curr->next;
	}
	deleteDead(process_list);	
}

bool to_free(cmdLine* cmd){
	if(strcmp(cmd->arguments[0],"wake") == 0 || strcmp(cmd->arguments[0],"kill") == 0 
	|| strcmp(cmd->arguments[0],"suspend") == 0 || strcmp(cmd->arguments[0],"ps") == 0 || strcmp(cmd->arguments[0],"procs") == 0 ||
	strcmp(cmd->arguments[0],"history") == 0 || strncmp(cmd->arguments[0],"!",1) == 0){
		return true;
	}else{
		return false;
	}
}

void add_copy_to_history(char* copy){
	if(strncmp(copy,"!",1) != 0){
		if(size_of_history<HISTLEN){
			history[size_of_history] = copy;	
			size_of_history++;
		}else{
			int i;
			free(history[0]);
			for(i=0 ; i<HISTLEN-1; i++){
				history[i] = history[i+1];
			}
			history[HISTLEN] = copy;
		}
	}else{
		free(copy);
	}
} 

int searchN(char* search){
	int i;
	if(sscanf(search, "!%d", &i) == 1){
		return i;
	}
	return -1;
}

void execute(cmdLine *pCmdLine , bool debug){
	if(strcmp(pCmdLine->arguments[0],"quit")==0){
		freeProcessList(Gprocess_list);
		free_history();
		exit(EXIT_SUCCESS);
	}else if(strcmp(pCmdLine->arguments[0],"cd")==0){
		if(chdir(pCmdLine->arguments[1])<0){
			freeProcessList(Gprocess_list);
			freeCmdLines(pCmdLine);
			free_history();
			perror("cd command fail");
		}
	}else if(strcmp(pCmdLine->arguments[0],"suspend") == 0){		
			int suspend_id = atoi(pCmdLine->arguments[1]);
			if(suspend_id<=0){
				fprintf(stderr, "Invalid PID: %s\n",pCmdLine->arguments[1]);
				freeProcessList(Gprocess_list);
				free_history();
				_exit(EXIT_FAILURE);
			}
			int res = kill(suspend_id,SIGTSTP);
			if(res == -1){
				perror("suspend failed\n");
				freeProcessList(Gprocess_list);
				freeCmdLines(pCmdLine);
				free_history();
				_exit(EXIT_FAILURE);
			}
			updateProcessStatusID(Gprocess_list, suspend_id, SUSPENDED);		
	}else if(strcmp(pCmdLine->arguments[0],"wake") == 0){		
			int suspend_id = atoi(pCmdLine->arguments[1]);
			if(suspend_id<=0){
				fprintf(stderr, "Invalid PID: %s\n",pCmdLine->arguments[1]);
				freeProcessList(Gprocess_list);
				freeCmdLines(pCmdLine);
				free_history();
				_exit(EXIT_FAILURE);
			}
			int res = kill(suspend_id,SIGCONT);
			if(res == -1){
				perror("wake failed\n");
				freeProcessList(Gprocess_list);
				freeCmdLines(pCmdLine);
				free_history();
				_exit(EXIT_FAILURE);
			}
			updateProcessStatusID(Gprocess_list, suspend_id, RUNNING);				
	}else if(strcmp(pCmdLine->arguments[0],"kill") == 0){		
			int suspend_id = atoi(pCmdLine->arguments[1]);
			if(suspend_id<=0){
				fprintf(stderr, "Invalid PID: %s\n",pCmdLine->arguments[1]);
				freeProcessList(Gprocess_list);
				freeCmdLines(pCmdLine);
				free_history();
				_exit(EXIT_FAILURE);
			}
			int res = kill(suspend_id,SIGINT);
			if(res == -1){
				perror("kill failed\n");
				freeProcessList(Gprocess_list);
				freeCmdLines(pCmdLine);
				free_history();
				_exit(EXIT_FAILURE);
			}
			updateProcessStatusID(Gprocess_list, suspend_id, TERMINATED);						
	}else if(strcmp(pCmdLine->arguments[0],"procs") == 0){
		printProcessList(&Gprocess_list);	
	}else if(strcmp(pCmdLine->arguments[0],"history") == 0){
		int i;
		for(i=0; i<size_of_history; i++){
			printf("%d %s",i,history[i]);
		}
	}else if(strcmp(pCmdLine->arguments[0],"!!") == 0){
		printf("%s",history[size_of_history-1]);
		char* copy = (char*) malloc(sizeof(char)*strlen(history[size_of_history-1])+1);
		strcpy(copy,history[size_of_history-1]);
		cmdLine* line = parseCmdLines(history[size_of_history-1]);
		add_copy_to_history(copy);	
		execute(line, debug);		
	}else if(searchN(pCmdLine->arguments[0])>=0){
		int number = searchN(pCmdLine->arguments[0]);
		if(number<HISTLEN){
			printf("%s",history[number]);
			char* copy = (char*) malloc(sizeof(char)*strlen(history[number])+1);
			strcpy(copy,history[number]);
			cmdLine* line = parseCmdLines(history[number]);
			add_copy_to_history(copy);	
			execute(line, debug);
		}else{
			fprintf(stdout,"n is out of bound\n");
		}	
	}else if(pCmdLine->next == NULL){
		int pid;
		pid = fork();
		if(pid == 0){
			if(pCmdLine->inputRedirect != NULL){
				int fd = open(pCmdLine->inputRedirect, O_RDONLY);
				if(fd == -1){
					perror("open");
					freeProcessList(Gprocess_list);
					freeCmdLines(pCmdLine);
					free_history();
					_exit(EXIT_FAILURE);
				}
				if(dup2(fd, STDIN_FILENO) == -1){
				perror("error dup2");
				freeProcessList(Gprocess_list);
				freeCmdLines(pCmdLine);
				free_history();
				_exit(EXIT_FAILURE);
				}
				close(fd);
			}
			
			if(pCmdLine->outputRedirect != NULL){
				int fd = open(pCmdLine->outputRedirect, O_RDWR | O_CREAT | O_TRUNC | O_APPEND,0664);
				if(fd == -1){
					perror("open");
					freeProcessList(Gprocess_list);
					freeCmdLines(pCmdLine);
					free_history();
					_exit(EXIT_FAILURE);
				}
				if(dup2(fd, STDOUT_FILENO) == -1){
					perror("error dup2");
					freeProcessList(Gprocess_list);
					freeCmdLines(pCmdLine);
					free_history();
					_exit(EXIT_FAILURE);
				}
				close(fd);
			}
			
			int success = execvp(pCmdLine->arguments[0],pCmdLine->arguments);
			if(success<0){
				perror("exeution faild");
				freeProcessList(Gprocess_list);
				freeCmdLines(pCmdLine);
				free_history();
				_exit(EXIT_FAILURE);
			}
			
			
		}
		
		addProcess(&Gprocess_list,pCmdLine,pid);
		if(debug){
			fprintf(stderr,"PID: %d\nexecuting command: %s\n",pid,pCmdLine->arguments[0]);
		}
		if(pid != 0 && pCmdLine->blocking){
			waitpid(pid,NULL,0);
		}
	}else{
		int fd[2];
	    if (pipe(fd) == -1) {
		perror("pipe");
		freeProcessList(Gprocess_list);
		freeCmdLines(pCmdLine);
		free_history();
		exit(EXIT_FAILURE);
	    }
	    // Fork first child
	    int child1 = fork();
	    if (child1 == -1) {
		perror("fork");
		freeProcessList(Gprocess_list);
		freeCmdLines(pCmdLine);
		free_history();
		exit(EXIT_FAILURE);
	    }


	    if (child1 == 0) {	
	    	if(pCmdLine->inputRedirect != NULL){
				int fd = open(pCmdLine->inputRedirect, O_RDONLY);
				if(fd == -1){
					perror("open");
					freeProcessList(Gprocess_list);
					freeCmdLines(pCmdLine);
					free_history();
					_exit(EXIT_FAILURE);
				}
				if(dup2(fd, STDIN_FILENO) == -1){
				perror("error dup2");
				freeProcessList(Gprocess_list);
				freeCmdLines(pCmdLine);
				free_history();
				_exit(EXIT_FAILURE);
				}
				close(fd);
			}
			
			if(pCmdLine->outputRedirect != NULL){
				fprintf(stderr, "Error of Rediretion");
				freeProcessList(Gprocess_list);
				freeCmdLines(pCmdLine);
				free_history();
				_exit(EXIT_FAILURE);
			}	    		
		close(STDOUT_FILENO); // Close standard output
		dup(fd[1]); // Duplicate write-end of pipe
		close(fd[1]); // Close file descriptor that was duplicated
		close(fd[0]); // Close read-end of pipe
		execvp(pCmdLine->arguments[0], pCmdLine->arguments);
		perror("execvp");
		exit(EXIT_FAILURE);
				
	    }
	    	if(debug){
			fprintf(stderr,"PID: %d\nexecuting command: %s\n",child1,pCmdLine->arguments[0]);
		}
		if(child1 != 0 && pCmdLine->blocking){
			waitpid(child1,NULL,0);
		}

	    close(fd[1]); // Close write-end of pipe
	    int child2 = fork();
	    if (child2 == -1) {
		perror("fork");
		freeProcessList(Gprocess_list);
		freeCmdLines(pCmdLine);
		free_history();
		exit(EXIT_FAILURE);
	    }
	   
	    if (child2 == 0){
	    		if(pCmdLine->inputRedirect != NULL){
				fprintf(stderr, "Error of Rediretion");
				freeProcessList(Gprocess_list);
				freeCmdLines(pCmdLine);
				free_history();
				_exit(EXIT_FAILURE);
			}
			
			if(pCmdLine->outputRedirect != NULL){
				int fd = open(pCmdLine->outputRedirect, O_RDWR | O_CREAT | O_TRUNC | O_APPEND);
				if(fd == -1){
					perror("open");
					freeProcessList(Gprocess_list);
					freeCmdLines(pCmdLine);
					free_history();
					_exit(EXIT_FAILURE);
				}
				if(dup2(fd, STDOUT_FILENO) == -1){
					perror("error dup2");
					freeProcessList(Gprocess_list);
					freeCmdLines(pCmdLine);
					free_history();
					_exit(EXIT_FAILURE);
				}
				close(fd);
			}
		close(STDIN_FILENO); // Close standard input
		dup(fd[0]); // Duplicate read-end of pipe
		close(fd[0]); // Close file descriptor that was duplicated
		close(fd[1]); // Close write-end of pipe
		execvp(pCmdLine->next->arguments[0], pCmdLine->next->arguments);
		perror("execvp");
		freeProcessList(Gprocess_list);
		freeCmdLines(pCmdLine);
		free_history();
		exit(EXIT_FAILURE);		
	    }
	    addProcess(&Gprocess_list,pCmdLine,child1);
	      if(debug){
			fprintf(stderr,"PID: %d\nexecuting command: %s\n",child2,pCmdLine->next->arguments[0]);
		}
		if(child2 != 0 && pCmdLine->next->blocking){
			waitpid(child2,NULL,0);
		}
		
	    close(fd[0]); // Close read-end of pipe
	    waitpid(child1, NULL, 0);
	    waitpid(child2, NULL, 0);
		
	}
	if(to_free(pCmdLine)){freeCmdLines(pCmdLine);}
}





int main (int argc, char* argv[]){
	Gprocess_list = NULL;
	char buffer[2048];
	bool debugMode=false;
	

	int i;
	for(i=0; i<argc;i++){
		if((strcmp("-D",argv[i])==0)){
			debugMode=true;
		}
	}
	
	
	while(1){
	char path_name [PATH_MAX];
	if(getcwd(path_name,PATH_MAX)!=NULL){printf("%s$ ",path_name);}
	if(fgets(buffer, 2048,stdin) == NULL){
		free_history();
		freeProcessList(Gprocess_list);
		exit(0);
	}
	if(strncmp(buffer,"quit",4) == 0){
		freeProcessList(Gprocess_list);
		free_history();
		exit(0);
	}
	char* copy = (char*) malloc(sizeof(char)*strlen(buffer)+1);
	strcpy(copy,buffer);
	cmdLine* line = parseCmdLines(buffer);
	add_copy_to_history(copy);	
	execute(line, debugMode);	
	}
	return 0;
}



