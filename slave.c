//Author  :  Amal Presingu
//Date    :  10/14/2022
#include "config.h"

enum state { idle, want_in, in_cs };

int turnId, flagId;
int *turn;
enum state *flag;

//opening and writing to cstest
void fileWrite(int id)
{
	FILE *fptr;
	fptr = fopen(CSTEST, "a");

	if (fptr == NULL)
	{
		printf("Cannot open file");
		return;
	}

	time_t t = time(NULL);
	struct tm *tm = localtime(&t);

	//HH:MM:SS File modified by process number xx
	fprintf(fptr, "%d:%d:%d File modified by process number %d\n", tm -> tm_hour, tm -> tm_min, tm -> tm_sec, id);
	fclose(fptr);
}

//opening and writing to logfile
void logEntry(int id, char *str)
{
	char filename[50];
	snprintf(filename, 50, "logfile.%d", id);

	FILE *fptr;
	fptr = fopen(filename, "a");
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	fprintf(fptr, "%s Critical Section at %s", str, asctime(tm));
	fclose(fptr);
}

//SIGTERM signal handle
void signalHandle()
{
	//once again using shared memory system calls; turn will go into the algorithm below
	shmdt(turn);
	shmctl(turnId,IPC_RMID,NULL);

    	shmdt(flag);
 	shmctl(flagId,IPC_RMID,NULL);
    	exit(0);
}
	
//main code for slave (working with semaphores)
int main(int argc , char ** argv){
    	signal(SIGTERM,signalHandle);

    	int childIds = atoi(argv[1]);
    	int n = atoi(argv[2]);

	struct sembuf semLock, semRelease;

	//taken from man page
	//number, operation, and flags for LOCK & RELEASE
	semLock.sem_num = 0;
	semLock.sem_op = -1;
	semLock.sem_flg = 0;

	semRelease.sem_num = 0;
	semRelease.sem_op = 1;
	semRelease.sem_flg = 0;

	key_t key = ftok(SEM_NAME, 'P');
	if (key == -1) 
	{
		perror("File not found");
		exit(1);
	}

	int semId = semget(key, 1, 0);
	if (semId == -1)
	{
		perror("semget");
		exit(1);
	}

	srand(time(0));
	for (int itr = 0; itr < 5; itr++)
	{
                int randomSleepTimeBefore  = rand()%3 +1;
                int randomSleepTimeAfter  = rand()%3 +1;
		
		semop(semId, &semLock, 1);

		//critical section
		logEntry(childIds, ENTER);
		sleep(randomSleepTimeBefore);
		fileWrite(childIds);
		sleep(randomSleepTimeAfter);
		logEntry(childIds, EXIT);
		//exit section 
		semop(semId, &semRelease, 1);
	}
	return 0;
}






