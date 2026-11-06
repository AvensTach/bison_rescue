#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_EXPEDITIONS 10
#define MSG_KEY 0x1234
#define TEXT_SIZE 256

typedef enum { YOUNG, ADULT, OLD } BisonAge;

typedef struct {
    char observing_personname[50];
    char territory_name[50];
    int bison_number;
    BisonAge bison_age;
    time_t observation_time;
} Observation;

typedef struct {
    long mtype;
    Observation data;
} MsgBuf;

#endif /* COMMON_H */