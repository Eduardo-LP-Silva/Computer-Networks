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
#include <sys/time.h>

#define SERVER_PORT 21

// #define SERVER_HOSTNAME "speedtest.tele2.net"
#define SERVER_HOSTNAME "ftp.dlptest.com"

#define MAXCONNECTIONS 10

typedef struct
{
	char* IPAddress;
	int port;
	FILE* fp;
}
connection;


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
	int	sockfd, i, maxAttempts = 3;
	struct sockaddr_in server_addr;
	
	/*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(conn->IPAddress);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(conn->port);		/*server TCP port must be network byte ordered */
	
	for (i = 0; i < maxAttempts; i++)
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
	
	if (i == maxAttempts)
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
			return 1;
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
			return 2;
		}

		if (buffer[3] == ' ')
			flag = 0;

		strcat(message, buffer);
	}

	free(bufferSize);
	free(buffer);

	return 0;
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
}

int login(connection* conn, char* message, char* username, char* password)
{
	sendCommand(conn, username);
	
	receiveMessage(conn, message);
	printf("%s\n", message);

	sendCommand(conn, password);

	receiveMessage(conn, message);
	printf("%s\n", message);

	return 0;
}




int enterPassiveMode(connection** connections, char* message)
{
	int	sockfd, i, j, k, start, IPAddressCounter, messageLength;
	for (k = 0; k < 10; k++)
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
	
	return 0;
}

void swap(char* a, char*b)
{
	char temp = *a;
	*a = *b;
	*b = temp;
}

void shiftLeft(char* buffer, int size, int position, int shift)
{
	int i, j;

	for (j = 0; j < shift; j++)
	{

		for (i = position-1; i < size; i++)
		{
			swap(&buffer[i], &buffer[i+1]);
		}

		size--;
		position--;
	}
}

int destuff(char* buffer, int* size)
{
	int i;
	for (i = 0; i < *size-1; i++) // Destuffs the data package
	{
		if (buffer[i] == '\r' && buffer[i+1] == '\n')
		{
			shiftLeft(buffer, *size, i+1, 1);
			(*size) -= 1;

			// buffer[i] = FLAG;
		}
	}

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
	printf("\033[2J\033[1;1H");
	printf("\033[2J\033[1;1H");
}


int receiveFile(connection** connections, char* message, char* serverFilename, char* clientFilename)
{
	enterPassiveMode(connections, message);

	// receiveMessage(fpArray[0], message);
	// printf("%s\n", message);

	// message[0] = 0;

	// strcat(message, "LIST\n");

	// sendCommand(fpArray[0], message);
	// receiveMessage(fpArray[0], message);

	// if (memcmp(message, "15", 2) != 0)
	// {
	// 	fprintf(stderr, "Error reading list of files!\n");
	// 	return 1;
	// }

	// receiveMessage(fpArray[1], message);
	// printf("%s\n", message);

	// receiveMessage(fpArray[0], message);
	// printf("%s\n", message);

	// if (memcmp(message, "226", 3) != 0)
	// {
	// 	fprintf(stderr, "Error reading list of files!\n");
	// 	return 1;
	// }
	
	// enterPassiveMode(fpArray, message);

	message[0] = 0;

	strcat(message, "SIZE ");
	strcat(message, serverFilename);
	strcat(message, "\n");

	sendCommand(connections[0], message);

	receiveMessage(connections[0], message);
	// printf("%s\n", message);

	long int size = strtol(&message[4], NULL, 10);
	printf("size = %li\n", size);

	message[0] = 0;

	strcat(message, "TYPE I\n");

	sendCommand(connections[0], message);

	receiveMessage(connections[0], message);
	printf("%s\n", message);

	message[0] = 0;

	strcat(message, "RETR ");
	strcat(message, serverFilename);
	strcat(message, "\n");

	sendCommand(connections[0], message);

	receiveMessage(connections[0], message);
	printf("%s\n", message);

	size_t bufferSize = 4096;
	int i, sumBytes = 0, bytes = bufferSize, fd = open(clientFilename, O_WRONLY | O_TRUNC | O_CREAT, 0777);
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
	
		printf("deltaTime = %f\n", deltaTime);

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


int main(int argc, char** argv)
{
	char message[4096];

	setHandler();

	struct hostent * ent = gethostbyname(SERVER_HOSTNAME);
	char IPAddress[16];		
	inet_ntop(AF_INET, ent->h_addr_list[0], IPAddress, INET_ADDRSTRLEN);

	connection* connections[2];
	connections[0] = malloc(sizeof(connection*));
	connections[1] = malloc(sizeof(connection*));

	connections[1]->IPAddress = malloc(16);

	connections[0]->IPAddress = IPAddress;
	connections[0]->port = SERVER_PORT;

	attemptConnect(connections[0], message);

	receiveMessage(connections[0], message);
	printf("%s\n", message);
	
	// login(fpArray[0], message, "user anonymous\n", "pass anonymous\n");
	login(connections[0], message, "user dlpuser@dlptest.com\n", "pass e73jzTRTNqCN9PYAAjjn\n");

	receiveFile(connections, message, "curl.txt", "curl.txt");
	receiveFile(connections, message, "curl.txt", "curl.txt");
	receiveFile(connections, message, "curl.txt", "curl.txt");
	receiveFile(connections, message, "curl.txt", "curl.txt");

	// receiveFile(connections, message, "sandHeightmap.xcf", "sandHeightmap.xcf");

	free(connections[0]);
	free(connections[1]);

	return 0;
}


