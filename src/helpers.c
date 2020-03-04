#include "helpers.h"
#include "shell_util.h"

int flag_helper = 0;
int flag_sigUSR = 0;

int comparatorTime(void *a, void *b){
    return  ((((ProcessEntry_t*)a)->seconds > ((ProcessEntry_t*)b)->seconds)?1:-1);
}

void sigchld_handler(int sig){
    flag_helper = 1;
}

void sigusr_handler(int sig){
    flag_sigUSR = 1;
}

char *cmdFoundByPid(List_t* list, pid_t pid){
    node_t* node = (list->head);
    while(node!=NULL){
        ProcessEntry_t*  p = (ProcessEntry_t*)(node->value);

        if(pid == p->pid){
            char *cmd = p->cmd;
            return cmd;
        }
        node = node->next;
    }
    return NULL;
}
