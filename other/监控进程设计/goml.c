/*********************************************************************
file:         main.c
description:  the source file of gmobi otamaster loader implemention
date:         2018/09/29
author        chenyin
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/file.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include "file.h"
#include "com_app_def.h"

static const char *mon_process[] =
{
    "/usrapp/current/data/image/cn.rockycore.dkeyservice",
    "/usrapp/current/data/image/ioTSecuSrv.bin",
    "/usrapp/current/data/image/eezi-ota",
    "/usrapp/current/data/image/devicemonitor",
    NULL,
};

static void *mon_service(void *argv)
{
    char name[strlen(argv) + 1];
    char head[strlen(argv) + 16];

    strcpy(name, argv);
    strcat(strcat(strcpy(head, "MON<"), name), ">");
    prctl(PR_SET_NAME, head);
    printf("Monitor of '%s' is running.\n", name);


    while (1)
    {
        pid_t pid = fork();

        if (pid < 0)
        {
            printf("Failed to create process for '%s': %s\n", name, strerror(errno));
        }
        else if (pid == 0) //child process
        {
            if (execl(name, name, (void *)0) < 0)
                printf("Failed to excute '%s': %s\n", name, strerror(errno));
            return 0;
        }
        else
        {
            int stat;

            log_o(LOG_APPL, "'%s' is running, pid = %d\n", name, pid);
            waitpid(pid, &stat, 0);

            if (WIFEXITED(stat))
            {
                log_e(LOG_APPL, "'%s' exit with %d\n", name, WEXITSTATUS(stat));
            }
            else if (WIFSIGNALED(stat))
            {
                log_e(LOG_APPL, "'%s' is signaled by %d\n", name, WTERMSIG(stat));
            }
            else if (WIFSTOPPED(stat))
            {
                log_e(LOG_APPL, "'%s' is stoped by %d\n", name, WSTOPSIG(stat));
            }
            else
            {
                log_e(LOG_APPL, "'%s' is crashed\n", name);
            }
            sleep(5);
        }
    }

    return 0;
}

static void start_monitor(void)
{
    int index = 0;

    while (mon_process[index] != NULL)
    {
        pthread_t tid;
        int res = 0;

        if (!file_exists(mon_process[index]))
        {
            log_e(LOG_APPL, "%s is not exist", mon_process[index]);
            index++;
            continue;
        }

        printf("<start_monitor_v1> Create monitor for '%s'\n", mon_process[index]);
        res = pthread_create(&tid, 0, (void *)mon_service, (char *)mon_process[index]);
        if (res < 0)
        {
            printf("Failed to create monitor for '%s': %s\n", mon_process[index], strerror(errno));
        }
        pthread_detach(tid);
        index++;
        sleep(1); //waitting for thread start up!
    }

    printf("monitor finish!\n");
}

int main(int argc, char **argv)
{
    //wait for tboxapp startup
    sleep(2);

    start_monitor();

    while (1)
    {
        sleep(100000);
    }

    return 0;
}


