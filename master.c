//Author :  Amal Presingu
//Date   :  10/14/2022

#include "config.h"

//children globals
int shmidChild;
char *shmStrChild;
int childIds = 1;
int slaveId;
int semId;

//master globals
int shmidParent;
char *shmStrParent;
int *cIds;
int n;
char at[20];
char end[20];

//logfile for master
void logMaster(char *str)
{
	FILE *fptr;
	fptr = fopen("logfile.master", "a");
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	fprintf(fptr, "%s : %s", asctime(tm), str);
	fclose(fptr);
}

//SIGINT signal handled by child
void signalHandleSIGINTChild()
{}

//SIGTERM signal handled by child
void signalHandleSIGTERMChild()
{
	//master - child interprocess communication
	kill(slaveId, SIGTERM); 

	//using suggested shared memory system calls
	shmdt(shmStrChild);
	shmctl(shmidChild, IPC_RMID, NULL);
	exit(0);
}

//forking child
void childProcess()
{
	signal(SIGINT, signalHandleSIGINTChild);
	signal(SIGTERM, signalHandleSIGTERMChild);

	shmidChild = shmget(SHMKEY, SHMSIZE, 0666 | IPC_CREAT);
	shmStrChild = shmat(shmidChild, (void*)0, 0);

	int status;
	
	//might have been an easier way to do this, but this is the way I terminated the process
	while(1)
	{
		if (shmStrChild[childIds - 1] == '*')
		{
			shmStrChild[childIds - 1] = '@';

			slaveId = fork();
			char childIdsStr[10];
			char nStr[10];
			snprintf(childIdsStr, 10, "%d", childIds);
			snprintf(nStr, 10, "%d", n);

			if (slaveId == 0)
			{
				//child process
				execl("./slave", "./slave", childIdsStr, nStr, NULL);
			}
			else 
			{
				waitpid(slaveId, &status, 0);
				shmStrChild[childIds - 1] = '!';
				break;
			}
		}
	}

	//calling on parent to terminate if all child processes are finished
	int diff = strncmp(shmStrChild, end, n);
	if (diff == 0)
	{
		//processes are finished
		int parentId = getppid();
		kill(parentId, SIGTERM);
	}
	//detach and remove shared memory
	shmdt(shmStrChild);
	shmctl(shmidChild, IPC_RMID, NULL);

	exit(0);
}

//cleaning up master before termination
void exitMaster()
{
	shmdt(shmStrParent);
	shmctl(shmidParent, IPC_RMID, NULL);

	semctl(semId, 0, IPC_RMID);

	int status;
	for (int i = 0; i < n; i++)
	{
		kill(cIds[i], SIGTERM);
	}
}

//SIGINT handling master
void signalHandle()
{
	logMaster("Handled the SIGINT signal\n ");
	printf("Master: Handled the SIGINT signal\n");
	exitMaster();
	exit(0);
}

//SIGTERM handling master
void signalHandleAlarm()
{
	logMaster("Handled the SIGALARM signal. Exiting...\n");
	printf("Master: Handled the SIGALARM signal. Exiting...\n");
	exitMaster();
	exit(0);
}

//SIGTERM handling master
void signalHandleTerminate()
{
	logMaster("Handled the SIGTERM signal. Exiting... \n");
    	shmdt(shmStrParent);
    	shmctl(shmidParent,IPC_RMID,NULL);
	semctl(semId, 0, IPC_RMID);
   	exit(0);
}

int main(int argc, char **argv)
{

	//signal calls
	signal(SIGINT, signalHandle);
	signal(SIGALRM, signalHandleAlarm);
	signal(SIGTERM, signalHandleTerminate);

	int ss;
	//not sure if we needed to use getopt again, but found this method easier
	if(argc < 2)
	{
		perror("Master: Not enough arguments\n");
        	exit(0);
    	}
	else if (argc == 4 && (strcmp(argv[1], "-t") == 0))
	{
		ss = atoi(argv[2]);
		n = atoi(argv[3]);
	}
	else if (argc == 2)
	{
		ss = DEFAULT_SS;
		n = atoi(argv[1]);
	}
	else 
	{
		perror("Master: Invalid arguments, try again\n");
		exit(0);
	}

	alarm(ss);
	char error [100];

	if (n > MAXCHILD)
	{
		snprintf(error, 100, "Master: Max number of slave processes is %d", MAXCHILD);
		perror(error);
		logMaster(error);

		n = MAXCHILD;
	}

	cIds = (int *)malloc(sizeof(int) *n);

	//shared memory from above
	shmidParent = shmget(SHMKEY, SHMSIZE, 0666 | IPC_CREAT);
	shmStrParent = shmat(shmidParent, (void*)0, 0);
	//referencing the hotfix from above
	for (int i = 0; i < MAXCHILD; i++)
	{
		shmStrParent[i] = at[i] = '@';
		end[i] = '!';
	}

	//n forks
	int child;
	for (int i = 0; i < n; i++)
	{
		child = fork();
		if (child == 0)
		{
			//calling child
			childProcess();
		}
		else 
		{
			childIds++;
			cIds[i] = child;
		}
	}

	char input[100];
	char logStr[200];

	//initializing sempahores
	key_t key = ftok(SEM_NAME, 'P');
	if (key == -1)
	{
		perror("File was not found.");
		exit(1);
	}

	semId = semget(key, 1, 0666 | IPC_CREAT);
	if (semId == -1) 
	{
		perror("semget");
		exit(1);
	}

	int status = semctl(semId, 0, SETVAL, 1);
	if (status == - 1)
	{
		perror ("semctl");
		exit(1);
	}

	while (1)//not sure if this is the intended procedure, but it seems to work
	{
		if (strncmp(shmStrParent, at, n) == 0)
		{
			scanf("%[^\n]s", input);
			//check if slave number is less than n and print
			if (strlen(input) > 1) 
			{
				snprintf(logStr, 200, "Input:%s\n", input);
				logMaster(logStr);
				char *t1 = strtok(input, " ");
				//user command slave xx
				if (t1 != NULL && (strcmp(t1, "slave") == 0))
				{
					t1 = strtok(0, " ");
					int sn = atoi(t1);
					if (sn <= n)
					{
						shmStrParent[sn - 1] = '*';
					}
					else
					{
						snprintf(error, 100, "Master: Enter number between 1 and %d\n", n);
						perror(error);
						logMaster(error);
					}
				}
				else 
				{
					printf("Master: Invalid user input\n");
					logMaster("Master: Invalid user input\n");
				}
			}
			getchar();
		}
	}
	return 0;
}

