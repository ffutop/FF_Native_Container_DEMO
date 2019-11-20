#define _GNU_SOURCE
#include <sys/wait.h>
#include <sched.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); } while (0)

static int childFunc(void *arg)
{
    execv("/bin/bash", NULL);
    return 0;
}

#define STACK_SIZE (1024 * 1024)

int main(int argc, char **argv)
{
    char *stack;
    char *stackTop;
    pid_t pid;

    /* 为子任务申请栈内存 */
    stack = malloc(STACK_SIZE);
    if (stack == NULL)
        errExit("malloc");
    stackTop = stack + STACK_SIZE;
  
    pid = clone(childFunc, stackTop, CLONE_NEWUTS | CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWIPC | CLONE_NEWNET | SIGCHLD, NULL);
    if (pid == -1)
        errExit("clone");
    printf("clone() returned %ld\n", (long) pid);

    sleep(1);

    if (waitpid(pid, NULL, 0) == -1)
        errExit("waitpid");
    printf("child has terminated\n");
    exit(EXIT_SUCCESS);
}
