#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

void signal_handler(int signum){
	printf("Received signal %d (%s)\n",signum, strsignal(signum));
	signal(signum, SIG_DFL);
	raise(signum);
	if(signum==SIGTSTP){
		signal(SIGCONT, signal_handler);
		}
	if(signum == SIGCONT){
		signal(SIGTSTP, signal_handler);
		}
}

int main(int argc, char**argv){
	signal(SIGINT, signal_handler);
	signal(SIGTSTP, signal_handler);
	signal(SIGCONT, signal_handler);
	while(1){
		sleep(2);
	}
	return 0;
}
