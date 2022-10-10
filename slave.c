//Author  :  Amal Presingu
//Date    :  10/3/2022
#include "config.h"

//implemented turn-based algorithm from notes
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
	
//main code for slave (working with multiple process algorithm)
int main(int argc , char ** argv){
    	signal(SIGTERM,signalHandle);

    	int childIds = atoi(argv[1]);
    	int n = atoi(argv[2]);

    	for (int itr = 0; itr<5; itr++){

        	turnId  = shmget(SHMTURN, sizeof(int), 0666 | IPC_CREAT);
        	flagId = shmget(SHMFLAG,sizeof(enum state) *n, 0666 | IPC_CREAT);

        	turn = (int *) shmat(turnId, 0, 0);
        	flag = (enum state *) shmat(flagId ,0 ,0);

        	int j; // Local to each process
        	int i = childIds - 1;
        	do
        	{
            		flag[i] = want_in; // Raise my flag
            		j = *turn; // Set local variable
            		while ( j != i )
                		j = (flag[j] != idle) ? *turn : (j + 1) % n;
                	// Declare intention to enter critical section
                	flag[i] = in_cs;
                	// Check that no one else is in critical section
                	for (j = 0; j < n; j++)
                    		if ( ( j != i ) && ( flag[j] == in_cs ) )
                        break;
        	} 
        	while( ( j < n ) || ( *turn != i && flag[*turn] != idle ));

        	// Assign turn to self and enter critical section
        
        	*turn = i;
        	srand(time(0));

		//sleeping between 1 and 3 seconds
        	int randomSleepTimeBefore  = rand()%3 +1;
        	int randomSleepTimeAfter  = rand()%3 +1;

        	logEntry(childIds, ENTER);    

        	sleep(randomSleepTimeBefore);                   
        	fileWrite(childIds);
        	sleep(randomSleepTimeAfter);

        	logEntry(childIds, EXIT);    
        	// Exit section

        	j = (*turn + 1) % n;
        	while (flag[j] == idle)
        	j = (j + 1) % n;
        	// Assign turn to next waiting process; change own flag to idle
        	*turn = j;
        	flag[i] = idle;

    	}
	return 0;
}































