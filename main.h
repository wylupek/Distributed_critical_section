#ifndef MAINH
#define MAINH
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <vector>
#include <algorithm>
#include "util.h"

extern int size;
extern int rank;
extern int lamport;
extern int lamportREQQUEUE;
extern pthread_t comThread;


extern int ackQueueCounter;
extern int ackResCounter;

/* Print with colors based on rank */
#ifdef DEBUG
#define debugln(FORMAT,...) printf("%c[%d;%dm [%d %d]: " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, rank, lamport, ##__VA_ARGS__, 27,0,37);
#define debug(FORMAT,...) printf("%c[%d;%dm [%d %d]: " FORMAT "%c[%d;%dm",  27, (1+(rank/7))%2, 31+(6+rank)%7, rank, lamport, ##__VA_ARGS__, 27,0,37);
#define debugNoTag(FORMAT,...) printf("%c[%d;%dm" FORMAT "%c[%d;%dm",  27, (1+(rank/7))%2, 31+(6+rank)%7, ##__VA_ARGS__, 27,0,37);
#else
#define debugln(...);
#define debug(...);
#define debugNoTag(...);
#endif

#define println(FORMAT,...) printf("%c[%d;%dm [%d %d]: " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, rank, lamport, ##__VA_ARGS__, 27,0,37);
#define print(FORMAT,...) printf("%c[%d;%dm [%d %d]: " FORMAT "%c[%d;%dm",  27, (1+(rank/7))%2, 31+(6+rank)%7, rank, lamport, ##__VA_ARGS__, 27,0,37);
#define printNoTag(FORMAT,...) printf("%c[%d;%dm" FORMAT "%c[%d;%dm",  27, (1+(rank/7))%2, 31+(6+rank)%7, ##__VA_ARGS__, 27,0,37);
#endif
