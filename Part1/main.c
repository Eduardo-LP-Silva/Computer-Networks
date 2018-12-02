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


#define SERVER_PORT 21

// #define SERVER_HOSTNAME "speedtest.tele2.net"
#define SERVER_HOSTNAME "ftp.dlptest.com"


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


int receiveMessage(FILE* fp, char* message)
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
		bytes = getline(&buffer, bufferSize, fp);
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

int sendCommand(FILE* fp, char* command)
{
	int bytes = fwrite(command, 1, strlen(command), fp);
	// printf("Bytes = %i\n", bytes);
	
	if (bytes != strlen(command))
	{
		fprintf(stderr, "Error seding command!\n");
		return 1;
	}
}

int login(FILE* fp, char* message, char* username, char* password)
{
	sendCommand(fp, username);
	
	receiveMessage(fp, message);
	printf("%s\n", message);

	sendCommand(fp, password);

	receiveMessage(fp, message);
	printf("%s\n", message);

	return 0;
}




int enterPassiveMode(FILE** fpArray, char* message)
{

	int	sockfd;
	int k;
	// for (k = 0; k < 5; k++)
	// {
		struct	sockaddr_in server_addr;

		char IPAddress[16];
		IPAddress[0] = 0;
		int i, j = 0, start = -1, IPAddressCounter = 0, port = 0, messageLength = strlen(message);

		sendCommand(fpArray[0], "PASV\n");

		receiveMessage(fpArray[0], message);
		
		printf("Changing server for passive mode\n");

		j = 0, start = -1, IPAddressCounter = 0, port = 0, messageLength = strlen(message);
		for (i = 0; i < messageLength; ++i)
		{
			if (message[i] == '(')
				start = i+1;

			if (start != -1 && (message[i] == ',' || message[i] == ')'))
			{
				if (IPAddressCounter < 4)
				{
					int offset = strlen(IPAddress);
					
					memcpy(&IPAddress[offset], &message[start], i-start);

					if (IPAddressCounter < 3)
					{
						IPAddress[offset + i-start] = '.';
						IPAddress[offset + i-start + 1] = 0;
					}
					else
						IPAddress[offset + i-start] = 0;
				}
				else
				{
					char portString[4];
					memcpy(portString, &message[start], i-start);
					long int part = strtol(portString, NULL, 10);

					if (IPAddressCounter == 4)
					{
						port = 256*part;
					}
					else if (IPAddressCounter == 5)
					{
						port += part;
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

		printf("IPAddress = %s\n", IPAddress);
		printf("port = %i\n", port);

		sleep(1);


			/*server address handling*/
		bzero((char*)&server_addr,sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = inet_addr(IPAddress);	/*32 bit Internet address network byte ordered*/
		server_addr.sin_port = htons(port);		/*server TCP port must be network byte ordered */
	 
	
		/*open an TCP socket*/
		if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
				perror("socket()");
				return 1;
			}
		/*connect to the server*/
			if(connect(sockfd, 
					  (struct sockaddr *)&server_addr, 
				sizeof(server_addr)) < 0){
				perror("connect()");
			return 1;
		}

		// break;
	// }

	fpArray[1] = fdopen(sockfd, "r+");
	
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

int receiveFile(FILE** fpArray, char* message, char* serverFilename, char* clientFilename)
{
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

	sendCommand(fpArray[0], message);

	receiveMessage(fpArray[0], message);
	// printf("%s\n", message);

	long int size = strtol(&message[4], NULL, 10);
	printf("size = %li\n", size);

	message[0] = 0;

	strcat(message, "RETR ");
	strcat(message, serverFilename);
	strcat(message, "\n");

	sendCommand(fpArray[0], message);

	receiveMessage(fpArray[0], message);
	printf("%s\n", message);

	size_t bufferSize = 128;
	int i, sumBytes = 0, bytes = bufferSize, fd = open(clientFilename, O_WRONLY | O_TRUNC | O_CREAT, 0777);
	char* buffer = malloc(bufferSize);

	int fd2 = fileno(fpArray[1]);

	for (i = 0; sumBytes < size; i++)
	{
		bufferSize = size - sumBytes;

		if (bufferSize > 128)
			bufferSize = 128;

		bytes = fread(buffer, 1, bufferSize, fpArray[1]);
		// buffer[bytes-2] = 0;

		// bytes = read(fd2, buffer, bufferSize-2);

		destuff(buffer, &bytes);

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

		sumBytes += bytes;
	}

	printf("Read and wrote %i bytes\n", sumBytes);

	free(buffer);
	close(fd);

	printf("File transfered sucessfully!\n");

	return 0;
}


int main(int argc, char** argv)
{
	
	setHandler();
	
	int	sockfd;
	struct	sockaddr_in server_addr;
	char	message[4096];
	int	bytes;

	struct hostent * ent = gethostbyname(SERVER_HOSTNAME);
	char IPAddress[16];
		
	inet_ntop(AF_INET, ent->h_addr_list[0], IPAddress, INET_ADDRSTRLEN);
	
	/*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(IPAddress);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(SERVER_PORT);		/*server TCP port must be network byte ordered */
	 
	/*open an TCP socket*/
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
			perror("socket()");
			exit(0);
		}
	/*connect to the server*/
		if(connect(sockfd, 
				  (struct sockaddr *)&server_addr, 
			sizeof(server_addr)) < 0){
			perror("connect()");
		exit(0);
	}

	FILE** fpArray = calloc(5, sizeof(FILE*) * 5);
	fpArray[0] = fdopen(sockfd, "r+");


	receiveMessage(fpArray[0], message);
	printf("%s\n", message);
	
	// login(fpArray[0], message, "user anonymous\n", "pass anonymous\n");
	login(fpArray[0], message, "user dlpuser@dlptest.com\n", "pass e73jzTRTNqCN9PYAAjjn\n");

	enterPassiveMode(fpArray, message);

	char serverFilename[] = "coco.pdf", clientFilename[] = "1KB.zip";

	receiveFile(fpArray, message, serverFilename, serverFilename);

	closeSockets(fpArray);

	return 0;
}


