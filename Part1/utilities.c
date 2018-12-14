#include "utilities.h"

#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "constants.h"

int getFreeFilePointer(FILE** fpArray)
{
	int i;
	for (i = 0; i < MAXCONNECTIONS; i++)
	{
		if (fpArray[i] == NULL)
			return i;
	}

	return -1;
}

int closeSockets(FILE** fileArray)
{
	int i;
	for (i = 0; fileArray[i] != NULL; ++i)
	{
		fclose(fileArray[i]);
	}

	free(fileArray);

	return 0;
}

void sigalrm_handler(int signal)
{
	printf("Server response timed out!\n");

}

int setHandler()
{
	struct sigaction sigalrm_action;
	sigalrm_action.sa_handler = sigalrm_handler;
	sigemptyset(&sigalrm_action.sa_mask);
	sigalrm_action.sa_flags = 0;

	if (sigaction(SIGALRM, &sigalrm_action, NULL) < 0)
	{
		fprintf(stderr,"Unable to install SIGINT handler\n");
		return 1;
	}

	return 0;
}

int isNumber(char input)
{
	return (input >= '0' && input <= '9');
}

void printUsage()
{
	printf("Usage: download [OPTIONS] [HOSTNAME] [FILE]\n");
}

int findFirst(char* str, char target)
{
	int i;
	for (i = 0; str[i] != 0; i++)
	{
		if (str[i] == target)
			return i;
	}

	return -1;
}

int findLast(char* str, char target)
{
	int i, ret = -1;
	for (i = 0; str[i] != 0; i++)
	{
		if (str[i] == target)
			ret = i;
	}

	return ret;
}

int extractFromArgument(char* input, char* username, char* password, char* hostname, char* filename)
{
	char ftp[7];
	memcpy(ftp, input, 6);
	ftp[6] = 0;

	username[0] = 0;
	password[0] = 0;
	hostname[0] = 0;
	filename[0] = 0;

	if (strcmp(ftp, "ftp://") == 0)
	{
		input += 6;
		int index = findLast(input, '@');

		if (index != -1) // Login present
		{
			int index2 = findFirst(input, ':');

			if (index2 != -1)
			{
				memcpy(username, input, index2+1);
				username[index2] = 0;

				memcpy(password, input+index2+1, index-index2-1);
				password[index-index2+1] = 0;

				input += index+1;
			}
			else	
				return 1;
		}
		else // No login present
		{
			strcpy(username, ANONYMOUS);
			strcpy(password, "none");
		}

		int index2 = findFirst(input, '/');

		if (index2 != -1)
		{
			memcpy(hostname, input, index2);
			hostname[index2+1] = 0;

			strcpy(filename, input+index2+1);
		}
		else
			return 1;

		return 0;
	}

	return 1;
}

void printPercentage(double percentage)
{
	if (percentage >= 0 && percentage <= 1)
	{
		printf("<");

		int i, length = 15 /* length of the percentage bar */;
		for (i = 0; i < length; i++)
		{
			if ((double)i/length < percentage)
				printf("|");
			else
				printf(" ");
		}

		printf(">%.1f%%\n", percentage*100);
	}
}

void printTransferRate(double rate)
{
	if (rate > 0)
	{
		printf("Transfer rate : %.1f KB/s\n", rate/1024);
	}
}


void clearScreen()
{
	printf("\033[2J\033[1;1H");
	printf("\033[2J\033[1;1H");
}

int splitFilename(char* fullPath, char* path, char* filename)
{
	int i, index = -1;
	for (i = 0; fullPath[i] != 0; i++)
	{
		if (fullPath[i] == '/')
			index = i;
	}

	if (index != -1)
	{
		memcpy(path, fullPath, index+1);
		path[index+1] = 0;

		strcpy(filename, fullPath+index+1);

		return 0;
	}
	else
	{
		path[0] = 0;

		strcpy(filename, fullPath);

		return -1;
	}
}