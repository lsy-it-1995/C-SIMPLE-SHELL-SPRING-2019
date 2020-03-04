#include "shell_util.h"
#include "linkedList.h"
#include "helpers.h"

// Library Includes
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>


int main(int argc, char *argv[])
{
    int i; //loop counter
    char *args[MAX_TOKENS + 1];
    int exec_result;
    int exit_status;
    pid_t pid;
    pid_t wait_result;
    List_t* bg_list;

    //Initialize the linked list
    bg_list = malloc(sizeof(List_t));
    bg_list->head = NULL;
    bg_list->length = 0;
    bg_list->comparator = NULL;  // Don't forget to initialize this to your comparator!!!

    // Setup segmentation fault handler
    if(signal(SIGSEGV, sigsegv_handler) == SIG_ERR){
        perror("Failed to set signal handler");
        exit(-1);
    }
    if(signal(SIGCHLD, sigchld_handler) == SIG_ERR){
        perror("Failed to set signal handler");
        exit(-1);
    }
    if(signal(SIGUSR1, sigusr_handler) == SIG_ERR){
        perror("Failed to set signal handler");
        exit(-1);
    }

    while(1) {
        // DO NOT MODIFY buffer
        // The buffer is dynamically allocated, we need to free it at the end of the loop
        char * const buffer = NULL;
        size_t buf_size = 0;

        // Print the shell prompt
        display_shell_prompt();

        // Read line from STDIN
        ssize_t nbytes = getline((char **)&buffer, &buf_size, stdin);

        // No more input from STDIN, free buffer and terminate
        if(nbytes == -1) {
            free(buffer);
            break;
        }

        // Remove newline character from buffer, if it's there
        if(buffer[nbytes - 1] == '\n')
            buffer[nbytes- 1] = '\0';

        // Handling empty strings
        if(strcmp(buffer, "") == 0) {
            free(buffer);
            continue;
        }

        char *line = strdup(buffer); //copy the entire buffer

        // Parsing input string into a sequence of tokens
        size_t numTokens;
        *args = NULL;
        numTokens = tokenizer(buffer, args);

        if(strcmp(args[0],"exit") == 0) {
            // Terminating the shell

            node_t* node = (bg_list->head);
            while(node!=NULL){
                ProcessEntry_t*  p = (ProcessEntry_t*)(node->value);
                kill(p->pid, SIGKILL);
                node = node->next;
            }

            free(buffer);
            return 0;
        }
        char s[100];

        //change dir
        if(strcmp(args[0],"cd") == 0) {
            if (args[1] == NULL) {
                chdir(getenv("HOME"));
                printf("%s\n", getcwd(s, 100));
            } else if (chdir(args[1]) == 0) { //if dir is valid and found
                printf("%s\n", getcwd(s, 100));
            } else { //if dir is not found and valid
                fprintf(stderr, DIR_ERR);
            }
            free(buffer);
            continue;
        }

        if(strcmp(args[0],"estatus") == 0){
            printf("%d\n", WEXITSTATUS(exit_status));
            free(buffer);
            continue;
        }

        int background_proc = 0;
        int p[2];
        int pipeIndex = 0;
        int pipeFlag = 0;
        pid_t ppid;

        int i = 0;
        int value1 = 0;
        int value2 = 0;
        int value3 = 0;
        int value4 = 0;
        int flag1 = 0, flag3 = 0;//write and append can't happen
        for(i = 0; i < numTokens; i++){
            int check1 = strcmp(args[i],">"); //write
            int check2 = strcmp(args[i],"<"); //read
            int check3 = strcmp(args[i],">>"); //append
            int check4 = strcmp(args[i],"2>"); //redirect

            //args[i] symbol
            //args[i+1] file name

            if(strcmp(args[i],"&") == 0){
                args[numTokens - 1] = NULL;
                background_proc = 1;
            }
            else if(strcmp(args[i], "|")==0){
                pipeIndex = i;
                pipeFlag = 1;
                args[i] = NULL;
            }
            else if(check1 == 0){
                flag1 = 1;
                value1 = open(args[i+1], O_RDWR|O_CREAT, 00644 );
                args[i] = NULL;
            }else if(check2 == 0) {
                value2 = open(args[i+1], O_RDONLY, 00644);
                args[i] = NULL;
            }else if(check3 == 0){
                flag3 = 1;
                value3 = open(args[i+1], O_RDWR|O_CREAT|O_APPEND, 00644);
                args[i] = NULL;
            }else if(check4 == 0) {
                value4 = open(args[i+1], O_RDWR|O_CREAT, 00644);
                args[i] = NULL;
            }
        }

        if(value1 < 0 || value2< 0 || value3 < 0 || value4 < 0 || (flag1 == 1 && flag3 == 1)){
            fprintf(stderr, RD_ERR);
            free(buffer);
            continue;
        }

        if(pipeFlag == 1)
            if(pipeIndex == 0 || pipeIndex == numTokens-1 || pipe(p)< 0 ||
               (background_proc == 1 && pipeIndex == numTokens -2)){
                fprintf(stderr, PIPE_ERR);
                free(buffer);
                continue;
            }
        pid = fork();   //In need of error handling......

        if(pid == 0){ //If zero, then it's the child process

            if(value1){dup2(value1, 1);}
            if(value2){dup2(value2, 0);}
            if(value3){dup2(value3, 1);}
            if(value4){dup2(value4, 2);}

            close(p[0]);
            dup2(p[1], STDOUT_FILENO);
            exec_result = execvp(args[0], &args[0]); //take everything b4 | which is NULL

            if(value1){close(value1);}
            if(value2){close(value2);}
            if(value3){close(value3);}
            if(value4){close(value4);}

            if(exec_result == -1){ //Error checking
                printf(EXEC_ERR, args[0]);
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        }else{ // Parent Processif

            if(pipeFlag == 1){
                ppid = fork();
                if(ppid ==0){ //child
                    close(p[1]);
                    dup2(p[0], STDIN_FILENO);
                    exec_result = execvp(args[pipeIndex + 1], &args[pipeIndex + 1]); //take everything from | + 1 (index) to the end
                    if(exec_result == -1){ //Error checking
                        printf(EXEC_ERR, args[0]);
                        exit(EXIT_FAILURE);
                    }
                    exit(EXIT_SUCCESS);
                }else{ //parent
                    close(p[1]);
                    close(p[0]);
                    wait_result = waitpid(ppid, &exit_status, 0);
                    if(wait_result == -1){
                        printf(WAIT_ERR);
                        exit(EXIT_FAILURE);
                    }
                }
            }

            if(flag_helper == 1){
                flag_helper = 0;
                while((wait_result = waitpid(-1, NULL, WNOHANG))>0){
                    char *cmd = cmdFoundByPid(bg_list, wait_result);
                    if(cmd!=NULL){
                        printf(BG_TERM, wait_result, cmd);
                        removeByPid(bg_list, wait_result);
                    }
                }
            }

            if(flag_sigUSR == 1){
                flag_sigUSR = 0;
                node_t* node = (bg_list->head);
                while(node!=NULL){
                    ProcessEntry_t*  p = (ProcessEntry_t*)(node->value);
                    printBGPEntry(p);
                    node = node->next;
                }
            }

            if(background_proc == 1){
                int i = 0;
//                background_proc = 0;
                ProcessEntry_t *proc = malloc(sizeof(ProcessEntry_t));
                proc->cmd =  line;
                proc->pid = pid;
                proc->seconds = time(NULL);
                bg_list->comparator = &comparatorTime;
                insertInOrder(bg_list, proc);
            }else{
                if(kill(pid, 0) == 0) {
                    wait_result = waitpid(pid, &exit_status, 0);
                    if (wait_result == -1) {
                        printf(WAIT_ERR);
                        exit(EXIT_FAILURE);
                    }
                }
            }

        }
        free(buffer);
    }
    return 0;
}

