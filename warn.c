#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <ctype.h>
#include <stdbool.h>

static int delay = 5;
static bool childFlag = false;

void parentHandler(int);
void childHandler(int);
void alarmHandler(int);
void parentHandlerTwo(int);
void userHandler(int);
bool tooManySpaces(char *);

int main(int argc, char **argv) {
    //pipe file descriptors for reading and writing:
    char line[512];
    int fd[2];
    char msg[512];  // used for writing.
    char *buf; // used as a temp for string manipulation
    bool tryAgain;
    if (pipe(fd) == -1) {
        fprintf(stdout, "Error: %s\n", strerror(errno));
        exit(1);
    }
    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stdout, "Error: %s\n", strerror(errno));
        exit(1);
    }
    //---------------------------------[ CHILD ]
    if (pid == 0) {
        close(fd[1]);
        // terminate the infinite loop:
        if (signal(SIGUSR1, userHandler) == SIG_ERR) {
            fprintf(stdout, "Error: %s\n", strerror(errno));
        }
        if (signal(SIGFPE, childHandler) == SIG_ERR) {
            fprintf(stdout, "Error: %s\n", strerror(errno));
        }
        //child ignores sigint:
        if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
            fprintf(stdout, "Error: %s\n", strerror(errno));
        }
        // recieve sigfpe from parent:
        if (signal(SIGALRM, alarmHandler) == SIG_ERR) {
            fprintf(stdout, "Error: %s\n", strerror(errno));
        }
        
        for (;;) {
            pause();
            if (childFlag) {
                close(0);
                dup(fd[0]);
                //read from pipe:
                if ((read(fd[0], line, 512)) < 0) {
                    fprintf(stderr, "Err:%s", strerror(errno));
                }

                if (line[0] == '\n') {
                    printf("Err: please enter a message.\n");
                    break;
                }
                
                // get rid of any leading space:
                if (line[0] == ' ') {
                    for (int i = 0; i < (strlen(msg) - 1);) {
                        while (line[i] == ' ') {
                            i++;
                        }
                        
                        //copy everything from that index:
                        strcpy(buf, &line[i]);
                        break;
                    }
                    // copy back to line.
                    strcpy(line, buf);
                }
                // return to global delay:
                if (line[0] > '0' && line[0] <= '9') {
                    sscanf(line, "%d", &delay);
                } else{
                    delay = 5;
                }
                //if the first character is an integer, grab the rest of the message:
                for (int i = 0; i < (strlen(line) - 1); i++) {
                    //copy user input to our temp string:
                    buf = strdup(line);
                    char *token = strtok(buf, " ");
                    //check to make sure there is a integer and a space
                    if ((atoi(token) > 0) && ((line[strlen(token)]) == ' ')) {
                        //get the index of the string that contains the integer and space:
                        i = (int) strlen(token) + 1;
                        //start from that index :
                        while (i < (strlen(line) - 1)) {  //----
                            //and get rid of any possible spaces:
                            while (line[i] == ' ') {
                                i++;
                            }
                            strcpy(buf, &line[i]);
                            // copy the 511 characters minus the pesky new line:
                            // msg will be used to be written:
                            sscanf(buf, "%511[^\n]", msg);
                            break;
                        }
                        break;
                    } else {
                        sscanf(line, "%511[^\n]", msg);
                        break;
                    }
                }
                childFlag = false;
            }//if childFlag == true
            if ((strcmp(msg, "exit")) == 0) {
                printf("exiting...\n");
                exit(0);
            }
            alarm(delay);
            printf("%s\n", msg);
        }
        close(fd[0]);
    } else {
        //---------------------------------[ PARENT ]
        //handle interupt:
        if (signal(SIGINT, parentHandler) == SIG_ERR) {
            printf("Error: %s\n", strerror(errno));
        }
        if (signal(SIGCHLD, parentHandlerTwo) == SIG_ERR) {
            printf("Error: %s\n", strerror(errno));
        }
        for (;;) {
            // user writes a message in handler to global line.
            pause();
            kill(pid, SIGUSR1);
            close(fd[0]); //close read end of pipe.
            printf("Please Enter A Message:\n");
            setbuf(stdin, NULL);
            fgets(line, sizeof(line), stdin);

            tryAgain = true;

            while (tryAgain) {
                // so we know if the input is invalid or not:
                tryAgain = tooManySpaces(line);
                if (tryAgain) {
                    break;
                }
                // get rid of any leading space:
                if (line[0] == '\n') {
                    printf("Err: please enter a message.\n");
                    break;
                }
                if (line[0] == ' ') {
                    for (int i = 0; i < (strlen(msg) - 1);) {
                        while (line[i] == ' ') {
                            i++;
                        }
                        //copy everything from that index:
                        strcpy(buf, &line[i]);
                        break;
                    }
                    // copy back to line.
                    strcpy(line, buf);
                }

                if (line[0] > '0' && line[0] <= '9') {
                    sscanf(line, "%d", &delay);
                } else{
                    delay = 5;
                }
                //if the first character is an integer, grab the rest of the message:
                for (int i = 0; i < (strlen(line) - 1); i++) {
                    //copy user input to our temp string:
                    buf = strdup(line);
                    char *token = strtok(buf, " ");
                    //check to make sure there is a integer and a space
                    if ((atoi(token) > 0) && ((line[strlen(token)]) == ' ')) {
                        //get the index of the string that contains the integer and space:
                        i = (int) strlen(token) + 1;
                        //start from that index :
                        while (i < (strlen(line) - 1)) {  //----
                            //and get rid of any possible spaces:
                            while (line[i] == ' ') {
                                i++;
                            }
                            strcpy(buf, &line[i]);
                            // copy the 511 characters minus the pesky new line:
                            // msg will be used to be written:
                            sscanf(buf, "%511[^\n]", msg);
                            break;
                        }
                        break;
                    } else {
                        sscanf(line, "%511[^\n]", msg);
                    }
                }
            }
            //now we write it to pipe:
            if (write(fd[1], line, 512) == -1) {
                printf("Error: %s\n", strerror(errno));
            }
            //send signal to child:
            kill(pid, SIGFPE);
        }
    }
    if (close(fd[1]) == -1) {
        printf("Error: %s\n", strerror(errno));
    }
}
//-------

//---------------------------
void parentHandler(int signal) {

}//parentHandler
//--------------

//---------------------------
void parentHandlerTwo(int signal) {
    // terminates upon recieving SIGCHLD
    exit(0);
}//parentHandler
//--------------

//---------------------------
void childHandler(int signal) {
    childFlag = true;
}//childHandler
//--------------

//---------------------------
void alarmHandler(int signal) {
    alarm(delay);
}//alarmHandler
//--------------

void userHandler(int sig) {
    //upon recieving SIGUSR1 from parent to child:
    //cancels alarm signal.
    alarm(0);
    pause();
}

//---------------------------
bool tooManySpaces(char *line) {
    int space = 0;
    int i = 0;
    //check for spaces:

    while (i <= (strlen(line) - 1)) {
        if (line[i] == ' ') {
            space++;
        }
        //if the string is nothing but spaces:
        i++;
    }
    if (space == (strlen(line) - 1)) {
        // return true to goAgain.
        return true;
    } else if (line[i] == '\n') {
        return true;
    } else {
        return false;
    }
}
//----------------
