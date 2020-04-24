// Kacper Kamieniarz (293065)
// EOPSY Lab 2

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <wait.h>
#include <stdbool.h>
#define NUM_CHILD 10
#define WITH_SIGNALS

void myInterrupt(int); // my keyboard interrupt handler
void childHandler(int);// child termination handler

bool keyboardInterrupt = 0;

int main(int argc, char* argv[]) {
    pid_t children[NUM_CHILD]; // array of children processes ID's
    pid_t childID;
    int i, j, temp, count = 0;

    #ifdef WITH_SIGNALS
            // inside parent
            for(j = 0; j < NSIG; j++) {
				// ignoring all the signals
                signal(j, SIG_IGN);
            }
            signal(SIGCHLD, SIG_DFL); // setting SIGCHLD back to default
            signal(SIGINT, myInterrupt);    // setting myInterrupt handler for SIGINT signals
    #endif

    for(i = 0; i < NUM_CHILD; i++) {
        
        if((childID = fork()) == 0) {
            // inside a child
            children[i] = getpid();
            #ifdef WITH_SIGNALS
            signal(SIGINT, SIG_IGN); // ignoring SIGINT 
            signal(SIGTERM, childHandler); // setting childHaldner for SIGTERM 
            #endif
            printf("child[%i]: created\n", getpid());
            sleep(10);
            printf("child[%i]: execution finished\n", getpid());
            exit(0); // exit after execution is finished
        } else if(childID < 0) {
            fprintf(stderr, "PARENT %i: UNABLE TO CREATE CHILD\n", getpid()); // printing message to stderr about failed try to create child
            for(j = 0; j < i; j++) {
                kill(j, SIGTERM);	// terminating all the children processes made so far
            }

            exit(1);
        } 
            
        sleep(1);
        #ifdef WITH_SIGNALS
        if(keyboardInterrupt) {
            printf("parent[%i]: the creation process interrupted.\n", getpid());
            printf("parent[%i]: sending SIGTERM signal.\n", getpid());
            for(j = 0; j < i; j++) {
                kill(j, SIGTERM);	// terminating child processes
            }
            
            
            break;
        }
        #endif
    }
    while(1) {
		temp = wait(NULL);
		if(temp == -1) {
			// inside parent, no more processes to synchronize
			break;
        } else {
			// exit code received
			count++; 
		}
	}
    printf("There are no more processes to be synchronized with parent.\nNumber of exit codes received: %d\n", count);

    #ifdef WITH_SIGNALS
		for(j = 0; j < NSIG; j++) {
			// setting all the signal handlers back to default
			signal(j, SIG_DFL); 
        }
	#endif
  
}


void childHandler(int signal) {
    printf("child[%i]: received SIGTERM signal, terminating\n", getpid());
}

void myInterrupt(int signal) {
    printf("parent[%i]: keyboard interrupt received\n", getpid());
    keyboardInterrupt = true;
}
