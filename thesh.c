#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

typedef struct cmd{
    int redirect_in;        /*Any stdin redirection?        */
    int redirect_out;       /*Any stdout redirection?       */
    int redirect_append;    /*Append stdout redirection?    */
    int background;         /*Put process in background?    */
    int piping;             /*Pipe prog1 into prog2?        */
    char *infile;           /*Name of stdin redirect file   */
    char *outfile;          /*Name of stdout redirect file  */
    char *argv1[10];        /*First program to exicute      */
    char *argv2[10];        /*Second program in pipe        */
}TODO;

int cmdscan(char *cmdbuf, struct cmd *com);

int main(void){
    char buffer[1024];
    int fdin[2], fdout[2], fdpipe[2], inputfile, outputfile;
    TODO info;
    pid_t child;
    pipe(fdin);
    pipe(fdout);

    while((gets(buffer) != NULL)){
        if (cmdscan(buffer,&info)){
            printf("Illegal Format: \n");
            continue;
        }
        if(strcmp(info.argv1[0], "exit") == 0){
            exit(0);
        }
        switch(child = fork()){
            case -1:
                perror("fork error\n");
                exit(0);
            case 0:     //child
                switch(child = fork()){
                    case -1:
                        perror("fork error\n");
                        exit(-1);
                    case 0:             //grandchild
                        if(info.redirect_in){
                            inputfile = open(info.infile, O_RDONLY);
                            dup2(inputfile, STDIN_FILENO);
                            close(inputfile);
                        }
                        if(info.redirect_out == 1 && info.redirect_append == 0){
                            outputfile = open(info.outfile, (O_CREAT | O_WRONLY | O_TRUNC), 0664);
                            dup2(outputfile, STDOUT_FILENO);
                            close(outputfile);
                        }
                        if(info.redirect_out == 1 && info.redirect_append == 1){
                            outputfile = open(info.outfile, (O_CREAT | O_WRONLY | O_APPEND), 0664);
                            dup2(outputfile, STDOUT_FILENO);
                            close(outputfile);
                        }
                        if(info.piping){
                            pipe(fdpipe);
                            switch(child = fork()){
                                case -1:
                                    perror("grandchild fork error\n");
                                    exit(-1);
                                case 0:     //great grandchild
                                    close(fdpipe[1]);
                                    dup2(fdpipe[0],STDIN_FILENO);
                                    //close(fdpipe[0]);
                                    execvp(info.argv2[0], info.argv2);
                                    perror("grandchild exec failed\n");
                                    exit(-1);
                            }

                            close(fdpipe[0]);
                            dup2(fdpipe[1],STDOUT_FILENO);
                            close(fdpipe[1]);
                        }

                        execvp(info.argv1[0],info.argv1);
                        perror("exec failed\n");
                        exit(-1);
                }
                if(!info.background){
                    waitpid(child,NULL,WCONTINUED);
                }
                else{
                    waitpid(child,NULL,WNOHANG);
                }
                exit(0);
        }
        waitpid(child,NULL,WCONTINUED);
        }//exits the while loop
    }
