#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#if 0
#define LOG(...) { printf(__VA_ARGS__); fflush(stdout); }
#else
#define LOG(...) ;
#endif

#define MSG_LENGTH 6
#define ALARM_TIME 5

int child(int number, int queueId);
void no_response_handler();

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		printf("usage: %s [Number of children]\n", argv[0]);
		return EXIT_SUCCESS;
	}

	char* end = NULL;
	int nChildren = strtol(argv[1], &end, 10);

	if (*end)
	{
		printf("usage: %s [Number of children]\n", argv[0]);
		return EXIT_SUCCESS;
	}

	int queueId = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
	if (queueId < 0)
	{
		perror("Cannot create a queue\n");
		return EXIT_FAILURE;
	}

	LOG("Parent: has created a queue with id = %d\n", queueId);	

	struct sigaction act = {};
	act.sa_handler = (sig_t) no_response_handler;
	sigaction(SIGALRM, &act, NULL);

	for (int i = 1; i <= nChildren; ++i)
	{
		pid_t pid = fork();

		if (pid == 0) //child
		{
			child(i, queueId);
			abort();
		}

		else if (pid == -1)
		{
			perror("Cannot fork\n");
			exit(EXIT_FAILURE);
		}
	}

	struct
	{
		long type;
		char buf[MSG_LENGTH];
	} msg = {1, "print"};

	int snd = msgsnd(queueId, &msg, MSG_LENGTH, 0);
	if (snd == -1)
	{
		perror("Parent: Cannot send the message\n");
		exit(EXIT_FAILURE);
	}

	LOG("Parent: has sent a message, msgsnd = %d\n", snd);

	alarm(ALARM_TIME);

	int rcv = msgrcv(queueId, &msg, MSG_LENGTH, nChildren + 1, 0);
	if (rcv == -1)
	{
		LOG("%s", "Parent: ");
		perror("Cannot read the message\n");
		exit(EXIT_FAILURE);
	}

	LOG("Parent: has received a message, msgrcv = %d\n", rcv);

	msgctl(queueId, IPC_RMID, NULL);
	return EXIT_SUCCESS;
}

void no_response_handler()
{
	printf("Out of time\n");
	exit(EXIT_FAILURE);
}

int child(int number, int queueId)
{
	struct
	{
		long type;
		char buf[MSG_LENGTH];
	} msg;

	alarm(ALARM_TIME);

	if ( msgrcv(queueId, &msg, MSG_LENGTH, number, 0) == -1 )
	{
		LOG("Child #%3d: ", number);
		perror("Cannot read the message");
		exit(EXIT_FAILURE);
	}

	printf("Child #%3d, pid = %d\n", number, getpid());
	fflush(stdout);

	msg.type++;

	msgsnd(queueId, &msg, MSG_LENGTH, 0);

	exit(EXIT_SUCCESS);
}
