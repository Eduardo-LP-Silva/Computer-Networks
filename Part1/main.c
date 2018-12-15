#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "constants.h"
#include "connection.h"
#include "utilities.h"

connection* currentFp = NULL;

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
		alarm(TIMEOUT);
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
		
		if (isNumber(buffer[0]) && isNumber(buffer[1]) && isNumber(buffer[2]))
		{
			if (memcmp(code, buffer, 3) == 0)
			{
				if (buffer[3] == ' ')
					flag = 0;
			}
			
		}

		strcat(message, buffer);
	}

	free(bufferSize);
	free(buffer);

	return (code[0] == '4' || code[0] == '5');
}

int sendCommand(connection* conn, char* command)
{
	int length = strlen(command);
	char* buffer = malloc(length+2+1);
	buffer[0] = 0;

	strcpy(buffer, command);
	strcat(buffer, "\r\n");

	int bytes = fwrite(buffer, 1, length+2, conn->fp);
	
	if (bytes != length+2)
	{
		fprintf(stderr, "Error seding command!\n");
		return 1;
	}

	return 0;
}

int closeConnection(connection* conn, char* message)
{
	message[0] = 0;

	strcpy(message, "QUIT");

	if (sendCommand(conn, message) != 0)
		return 1;

	if (receiveMessage(conn, message) != 0)
		return 1;

	printf("%s\n", message);

	return 0;
}

int login(connection* conn, char* message, char* username, char* password)
{
	char buffer[100+6+1];
	buffer[0] = 0;

	strcat(buffer, "user ");
	strcat(buffer, username);

	sendCommand(conn, buffer);
	
	if (receiveMessage(conn, message) != 0)
		return 1;

	// printf("%s\n", message);

	buffer[0] = 0;

	strcat(buffer, "pass ");
	strcat(buffer, password);

	sendCommand(conn, buffer);

	if (receiveMessage(conn, message) != 0)
		return 1;

	// printf("%s\n", message);

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

		sendCommand(connections[0], "PASV");

		receiveMessage(connections[0], message);
		messageLength = strlen(message);

		printf("%s\n", message);

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
					portString[i-start] = 0;

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

		sendCommand(connections[0], message);

		receiveMessage(connections[0], message);
		printf("%s\n", message);
	
	}
	
	if (enterPassiveMode(connections, message) != 0)
		return 1;

	// Sets transfer type to binary
	message[0] = 0;

	strcat(message, "TYPE I");

	sendCommand(connections[0], message);
	if (receiveMessage(connections[0], message) != 0)
	{
		printf("%s\n", message);
		return 1;
	}


	// Gets size of file
	message[0] = 0;

	strcat(message, "SIZE ");
	strcat(message, filename);

	sendCommand(connections[0], message);
	if (receiveMessage(connections[0], message) != 0)
	{
		printf("%s\n", message);
		return 1;
	}

	long long size = strtoll(&message[4], NULL, 10);
	printf("size = %lli\n", size);


	message[0] = 0;

	strcat(message, "RETR ");
	strcat(message, filename);

	sendCommand(connections[0], message);

	receiveMessage(connections[0], message);
	printf("%s\n", message);

	size_t bufferSize = 512;
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
		usleep(1000 * 1); // Sleeps for 10 millisecond
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
	
		if (gettimeofday(&startTime, NULL) != 0)
			printf("Error getting time!\n");

		
		if (sumTimeAverage > repeatTime)
		{
			clearScreen();

			rate = (double)sumBytesAverage / sumTimeAverage;

			sumBytesAverage = 0;
			sumTimeAverage = 0;

			printPercentage((double)sumBytes / size);
			printTransferRate(rate);
		}

		sumBytes += bytes;
		sumTime += deltaTime;

		sumBytesAverage += bytes;
		sumTimeAverage += deltaTime;
	}

	clearScreen();
	printPercentage((double)sumBytes / size);
	printTransferRate(rate);

	free(buffer);
	close(fd);

	fclose(connections[1]->fp);

	receiveMessage(connections[0], message);
	printf("%s\n", message);

	printf("Downloaded with average %.1f KB/s\n", (double)sumBytes/sumTime/1024);


	return 0;
}




int sendFile(connection** connections, char* message, char* filepath)
{
	char path[200];
	char filename[100];
	splitFilename(filepath, path, filename);
	
	if (enterPassiveMode(connections, message) != 0)
		return 1;


	// Sets transfer type to binary
	message[0] = 0;

	strcat(message, "TYPE I");

	sendCommand(connections[0], message);

	receiveMessage(connections[0], message);
	printf("%s\n", message);

	message[0] = 0;

	strcat(message, "STOR ");
	strcat(message, filename);

	sendCommand(connections[0], message);

	receiveMessage(connections[0], message);
	printf("%s\n", message);

	size_t bufferSize = 512;
	int i, sumBytes = 0, bytes = bufferSize, fd = open(filepath, O_RDONLY);
	char* buffer = malloc(bufferSize);

	struct stat st;

	if (stat(filename, &st) != 0)
	{
		printf("Failed to get file size!\n");
		return 1;
	}

	int size = st.st_size;

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
		bytes = read(fd, buffer, bytes);

		if (bytes < 0)
		{
			printf("Error reading from file!\n");
			return 1;
		}

		bytes = fwrite(buffer, 1, bufferSize, connections[1]->fp);

		if (bytes < 0)
		{
			printf("Error sending file to server!\n");
			return 1;
		}


		if (gettimeofday(&finishTime, NULL) != 0)
			printf("Error getting time!\n");

		double deltaTime = (double)(finishTime.tv_sec - startTime.tv_sec) + (double)(finishTime.tv_usec - startTime.tv_usec)/1000/1000; // In seconds 
	
		if (gettimeofday(&startTime, NULL) != 0)
			printf("Error getting time!\n");

		if (sumTimeAverage > repeatTime)
		{
			clearScreen();

			rate = (double)sumBytesAverage / sumTimeAverage;

			sumBytesAverage = 0;
			sumTimeAverage = 0;

			printPercentage((double)sumBytes / size);
			printTransferRate(rate);
		}

		sumBytes += bytes;
		sumTime += deltaTime;

		sumBytesAverage += bytes;
		sumTimeAverage += deltaTime;
	}

	clearScreen();
	printPercentage((double)sumBytes / size);
	printTransferRate(rate);

	free(buffer);
	close(fd);

	fclose(connections[1]->fp);

	receiveMessage(connections[0], message);
	printf("%s\n", message);

	printf("Uploaded with average %.1f KB/s\n", (double)sumBytes/sumTime/1024);

	return 0;
}


int main(int argc, char** argv)
{
	if (argc < 2 || argc > 4)
	{
		printUsage();
		return 1;
	}

	char *username = malloc(100), *password = malloc(100), *hostname = malloc(100), *filename = malloc(100);

	if (extractFromArgument(argv[argc-1], username, password, hostname, filename) != 0)
	{
		printUsage();
		return 1;
	}

	char* options = "hu";
	int i, opterr = 0, uflag = 0;
	char c;

	while ((c = getopt (argc-1, argv, options)) != -1)
	{
		if (c == 'h')
		{
			printUsage();
			return 0;
		}
		if (c == 'u')
		{
			uflag = 1;
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
		}	
		else
			return 1;
	}
	
	for (i = optind; i < argc-1; i++)
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

	if (uflag)
	{
		if (sendFile(connections, message, filename))
			return 1;
	}
	else
	{
		if (receiveFile(connections, message, filename))
			return 1;
	}

	if (closeConnection(connections[0], message) != 0)
		return 1;

	freeConnection(&connections[0]);
	freeConnection(&connections[1]);

	return 0;
}
