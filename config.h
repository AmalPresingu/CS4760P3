#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <signal.h>
#include <sys/types.h>

#define MAXCHILD 20
#define SHMKEY 123425
#define SHMSIZE 1024
#define SHMTURN 54321
#define SHMFLAG 54322

#define ENTER "Entered"
#define EXIT "EXIT"

#define CSTEST "cstest"
#define DEFAULT_SS 100
