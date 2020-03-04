#include <stdio.h>
#include <stdlib.h>
#include "linkedList.h"

extern int flag_helper;
extern int flag_sigUSR;

int comparatorTime(void *a, void *b);

void sigchld_handler(int sig);

void sigusr_handler(int sig);

char *cmdFoundByPid(List_t* list, pid_t pid);

