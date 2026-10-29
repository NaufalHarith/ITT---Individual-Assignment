#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

#define NUM_CHILDREN 3
#define BUFFER_SIZE 128

pid_t child_pids[NUM_CHILDREN];
int pipes[NUM_CHILDREN][2];

//xInterrupt handler
void handle_interrupt(int sig){
        write(STDOUT_FILENO, "\nCtrl+C initiated, program stopped.\n", 36);
        write(STDOUT_FILENO, "(Parent) All children have finished.\n", 37);

        for (int i=0; i<NUM_CHILDREN; i++){
          if(child_pids[i]>0){
                kill(child_pids[i],SIGKILL);
          }
        }
        exit(0);
}

int main(){
        char message[BUFFER_SIZE];

        signal(SIGINT, handle_interrupt);

        // Pre-allcating pipes for communicaation
        for(int i=0; i<NUM_CHILDREN; i++){
                if (pipe(pipes[i]) == -1){
                        perror("Pipe allocation failed");
                        exit(EXIT_FAILURE);
                }
        }
        // Forking into multiple child processes
        for(int i=0; i<NUM_CHILDREN; i++){
                child_pids[i] = fork();

                if (child_pids[i]<0){
                  perror("Process fork failed");
                  exit(EXIT_FAILURE);
                }
                if (child_pids[i]==0){
                  char read_buffer[BUFFER_SIZE]={0};

                  signal(SIGINT, SIG_IGN);

                  close(pipes[i][1]);

                  for(int j=0; j<i; j++){
                        close(pipes[j][1]);
                  }

                  //Read the message from the parent pipe
                  ssize_t bytes = read(pipes[i][0],  read_buffer, sizeof(read_buffer) - 1);
                  if(bytes>0){
                        read_buffer[bytes] = '\0';
                        printf("[Child %d] (PID: %d) received: %s\n", i + 1, getpid(), read_buffer);
                        fflush(stdout);
                  }

                  close(pipes[i][0]);
                  exit(EXIT_SUCCESS);
                }
        }

        for(int  i=0; i<NUM_CHILDREN; i++){
                close(pipes[i][0]);
        }
        //Get input from keybard
        printf("Parent: Enter a message for the children: ");
        fflush(stdout);

        if(fgets(message, sizeof(message), stdin) != NULL){
          message[strcspn(message, "\n")] =  '\0';

          for(int i=0; i<NUM_CHILDREN; i++){
                write(pipes[i][1], message, strlen(message));
                close(pipes[i][1]);
          }

          for(int i=0; i<NUM_CHILDREN; i++){
                waitpid(child_pids[i], NULL, 0);
          }

        return 0;
}
}
