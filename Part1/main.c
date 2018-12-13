#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/time.h>

#include "constants.h"
#include "connection.h"

connection* currentFp = NULL;


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

int attemptConnect(connection* conn, char* message)
{
	int sockfd, i;
	struct sockaddr_in server_addr;
	
	/*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(conn->IPAddress);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(conn->port);		/*server TCP port must be network byte ordered */
	
	for (i = 0; i < MAXATTEMPTS; i++)
	{
		sleep(1);

		/*open an TCP socket*/
		if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
		{
			// fprintf(stderr, "socket() failed!\n");
			continue;
		}
		/*connect to the server*/
		if (connect(sockfd, 
					  (struct sockaddr *)&server_addr, 
				sizeof(server_addr)) < 0)
		{
			// fprintf(stderr, "connect() failed!\n");
			continue;
		}

		break;
	}
	
	if (i == MAXATTEMPTS)
	{
		fprintf(stderr, "Connection timed out!\n");
		return 1;
	}

	conn->fp = fdopen(sockfd, "r+");

	return 0;
}

int receiveMessage(connection* conn, char* message)
{	
	size_t* bufferSize = malloc(sizeof(size_t));
	*bufferSize = 1024;
	int bytes, i, flag = 1;
	char* buffer = malloc(*bufferSize);
	char code[4];
	message[0] = 0;
	buffer[0] = 0;

	for (i = 0; flag; i++)
	{
		alarm(1);
		bytes = getline(&buffer, bufferSize, conn->fp);
		alarm(0); // Cancel alarm


		if (bytes < 0)
		{
			fprintf(stderr, "Error on reading from server!\n");
			free(bufferSize);
			free(buffer);
			return -1;
		}

		buffer[bytes] = 0;

		if (i == 0)
		{
			memcpy(code, buffer, 3);
			code[3] = 0;
		}
		else if (memcmp(code, buffer, 3) != 0)
		{
			fprintf(stderr, "Error on checking code from server!\n");
			free(bufferSize);
			free(buffer);
			return -2;
		}

		if (buffer[3] == ' ')
			flag = 0;

		strcat(message, buffer);
	}

	free(bufferSize);
	free(buffer);

	return (code[0] == '4' || code[0] == '5');
}

int sendCommand(connection* conn, char* command)
{
	int bytes = fwrite(command, 1, strlen(command), conn->fp);
	// printf("Bytes = %i\n", bytes);
	
	if (bytes != strlen(command))
	{
		fprintf(stderr, "Error seding command!\n");
		return 1;
	}

	return 0;
}

int login(connection* conn, char* message, char* username, char* password)
{
	char buffer[100+6+1];
	buffer[0] = 0;

	strcat(buffer, "user ");
	strcat(buffer, username);
	strcat(buffer, "\n");

	sendCommand(conn, buffer);
	
	if (receiveMessage(conn, message) != 0)
		return 1;

	printf("%s\n", message);

	buffer[0] = 0;

	strcat(buffer, "pass ");
	strcat(buffer, password);
	strcat(buffer, "\n");

	sendCommand(conn, buffer);

	if (receiveMessage(conn, message) != 0)
		return 1;

	printf("%s\n", message);

	return 0;
}




int enterPassiveMode(connection** connections, char* message)
{
	int sockfd, i, j, k, start, IPAddressCounter, messageLength;
	for (k = 0; k < MAXCONNECTIONS; k++)
	{
		connections[1]->IPAddress[0] = 0;
		start = -1;
		IPAddressCounter = 0;
		connections[1]->port = 0;

		sendCommand(connections[0], "PASV\n");

		receiveMessage(connections[0], message);
		messageLength = strlen(message);

		printf("Changing server for passive mode\n");

		for (i = 0; i < messageLength; ++i)
		{
			if (message[i] == '(')
				start = i+1;

			if (start != -1 && (message[i] == ',' || message[i] == ')'))
			{
				if (IPAddressCounter < 4)
				{
					int offset = strlen(connections[1]->IPAddress);
					
					memcpy(&connections[1]->IPAddress[offset], &message[start], i-start);

					if (IPAddressCounter < 3)
					{
						connections[1]->IPAddress[offset + i-start] = '.';
						connections[1]->IPAddress[offset + i-start + 1] = 0;
					}
					else
						connections[1]->IPAddress[offset + i-start] = 0;
				}
				else
				{
					char portString[4];
					memcpy(portString, &message[start], i-start);
					long int part = strtol(portString, NULL, 10);

					if (IPAddressCounter == 4)
					{
						connections[1]->port = 256*part;
					}
					else if (IPAddressCounter == 5)
					{
						connections[1]->port += part;
						break;
					}
					else
					{

					}
				}

				start = i+1;
				IPAddressCounter++;

			}
		}

		printf("IPAddress = %s\n", connections[1]->IPAddress);
		printf("port = %i\n", connections[1]->port);

		if (attemptConnect(connections[1], message) == 0)
			break;
	}

	if (i == MAXCONNECTIONS)
		return 1;
	
	return 0;
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


int receiveFile(connection** connections, char* message, char* serverFilename)
{
	char path[200];
	char filename[100];
	splitFilename(serverFilename, path, filename);

	if (path[0] != 0) // Not empty string
	{
		message[0] = 0;

		strcat(message, "CWD ");
		strcat(message, path);
		strcat(message, "\n");

		sendCommand(connections[0], message);

		receiveMessage(connections[0], message);
		printf("%s\n", message);
	
	}
	
	if (enterPassiveMode(connections, message) != 0)
		return 1;


	// Gets size of file
	message[0] = 0;

	strcat(message, "SIZE ");
	strcat(message, filename);
	strcat(message, "\n");

	sendCommand(connections[0], message);
	if (receiveMessage(connections[0], message) != 0)
	{
		fprintf(stderr, "File doesn't exist!\n");
		return 1;
	}

	long int size = strtol(&message[4], NULL, 10);
	printf("size = %li\n", size);


	// Sets transfer type to binary
	message[0] = 0;

	strcat(message, "TYPE I\n");

	sendCommand(connections[0], message);
	receiveMessage(connections[0], message);


	message[0] = 0;

	strcat(message, "RETR ");
	strcat(message, filename);
	strcat(message, "\n");

	sendCommand(connections[0], message);

	receiveMessage(connections[0], message);
	printf("%s\n", message);

	size_t bufferSize = 4096;
	int i, sumBytes = 0, bytes = bufferSize, fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0777);
	char* buffer = malloc(bufferSize);

	struct timeval startTime, finishTime;
	double sumTime = 0;

	if (gettimeofday(&startTime, NULL) != 0)
			printf("Error getting time!\n");
	if (gettimeofday(&finishTime, NULL) != 0)
			printf("Error getting time!\n");

	int sumBytesAverage = 0;
	double sumTimeAverage = 0, repeatTime = 0.5, rate = 0;
	for (i = 0; sumBytes < size; i++)
	{
		bytes = fread(buffer, 1, bufferSize, connections[1]->fp);

		if (bytes < 0)
		{
			printf("Error getting file from!\n");
			return 1;
		}

		bytes = write(fd, buffer, bytes);

		if (bytes < 0)
		{
			printf("Error writing to file!\n");
			return 1;
		}


		if (gettimeofday(&finishTime, NULL) != 0)
			printf("Error getting time!\n");

		double deltaTime = (double)(finishTime.tv_sec - startTime.tv_sec) + (double)(finishTime.tv_usec - startTime.tv_usec)/1000/1000; // In seconds 
	
		// printf("deltaTime = %f\n", deltaTime);

		if (gettimeofday(&startTime, NULL) != 0)
			printf("Error getting time!\n");

		clearScreen();

		if (sumTimeAverage > repeatTime)
		{
			rate = (double)sumBytesAverage / sumTimeAverage;

			sumBytesAverage = 0;
			sumTimeAverage = 0;
		}

		printPercentage((double)sumBytes / size);
		printTransferRate(rate);


		sumBytes += bytes;
		sumTime += deltaTime;

		sumBytesAverage += bytes;
		sumTimeAverage += deltaTime;
	}

	free(buffer);
	close(fd);

	fclose(connections[1]->fp);

	receiveMessage(connections[0], message);
	printf("%s\n", message);

	return 0;
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

int extractFromLink(char* input, char* username, char* password, char* hostname, char* filename)
{
	char ftp[7];
	memcpy(ftp, input, 6);
	ftp[6] = 0;

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
			password[0] = 0;
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

int main(int argc, char** argv)
{
	if (argc < 2 || argc > 3)
	{
		printUsage();
		return 1;
	}

	char *username = malloc(100), *password = malloc(100), *hostname = malloc(100), *filename = malloc(100);

	if (extractFromLink(argv[1], username, password, hostname, filename) != 0)
	{
		printUsage();
		return 1;
	}

	char* options = "h";
	int i, opterr = 0, fflag = 0, pflag = 0;
	char c;

	while ((c = getopt (argc-2, argv, options)) != -1)
	{
		if (c == 'h')
		{
			printUsage();
			return 0;
		}
		else if (c == '?')
		{
			int flag = 1;
			for (i = 0; options[i] != 0; i++)
			{
				if (options[i] == ':' && optopt == options[i-1])
				{
					fprintf (stderr, "Option -%c requires an argument.\n", optopt);
					flag = 0;
				}
				
			}

			if (flag)
			{
				if (isprint (optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
			}
			
			return 1;

		}	
		else
			return 1;
	}
	
	for (i = optind; i < argc-2; i++)
		printf ("Non-option argument %s\n", argv[i]);


	char message[4096];

	setHandler();

	struct hostent * ent = gethostbyname(hostname);
	char IPAddress[16];		
	inet_ntop(AF_INET, ent->h_addr_list[0], IPAddress, INET_ADDRSTRLEN);

	connection* connections[2];
	initializeConnection(&connections[0]);
	initializeConnection(&connections[1]);

	strcpy(connections[0]->IPAddress, IPAddress);
	connections[0]->port = SERVER_PORT;


	if (attemptConnect(connections[0], message) != 0)
		return 1;

	receiveMessage(connections[0], message);
	printf("%s\n", message);
	
	if (login(connections[0], message, username, password) != 0)
	{
		fprintf(stderr, "Failed to login!\n");
		return 1;
	}

	if (receiveFile(connections, message, filename))
		return 1;

	freeConnection(&connections[0]);
	freeConnection(&connections[1]);

	return 0;
}
