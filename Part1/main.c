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

#define SERVER_PORT 21
#define SERVER_HOSTNAME "speedtest.tele2.net"


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
			return 1;
		}

		buffer[bytes] = 0;

		printf("buffer = %s\n", buffer);

		if (i == 0)
		{
			memcpy(code, buffer, 3);
			code[3] = 0;
		}
		else if (memcmp(code, buffer, 3) != 0)
		{
			fprintf(stderr, "Error on checking code from server!\n");
			return 2;
		}

		if (buffer[3] == ' ')
			flag = 0;

		strcat(message, buffer);

		// printf("Got response!\n");
	}

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

int login(FILE* fp, char* username, char* password)
{
	if (fwrite(username, 1, strlen(username), fp) != strlen(username))
	{
		fprintf(stderr, "Error on login!\n");
		return 1;
	}

	if (fwrite(password, 1, strlen(password), fp) != strlen(password))
	{
		fprintf(stderr, "Error on login!\n");
		return 1;
	}

	return 0;
}




int enterPassiveMode(FILE** fp, char* message)
{
	sendCommand(*fp, "PASV\n");

	receiveMessage(*fp, message);
	printf("%s\n", message);
	
	printf("Changing server for passive mode\n");

	// fclose(*fp);

	char IPAddress[16];
	IPAddress[0] = 0;
	int i, j = 0, start = -1, IPAddressCounter = 0, port = 0, messageLength = strlen(message);

	for (int i = 0; i < messageLength; ++i)
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
					printf("Got both IPAddress and port\n");
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

	int	sockfd;
	struct	sockaddr_in server_addr;

	/*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(IPAddress);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(port);		/*server TCP port must be network byte ordered */
	 
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

	*fp = fdopen(sockfd, "r+");

	
	return 0;
}

int receiveFile(FILE* fp, char* message, char* serverFilename, char* clientFilename)
{
	message[0] = 0;

	strcpy(message, "RETR ");

	strcpy(message, serverFilename);
	// strcpy(message, " ");
	// strcpy(message, clientFilename);

	strcpy(message, "\n");

	sendCommand(fp, message);

	// int fd = open()

	receiveMessage(fp, message);
	printf("%s\n", message);

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

	FILE* fp = fdopen(sockfd, "r+");


	receiveMessage(fp, message);
	printf("%s\n", message);
	
	char username[] = "user anonymous\n", password[] = "PASS ola\n";
	sendCommand(fp, username);
	
	receiveMessage(fp, message);
	printf("%s\n", message);

	sendCommand(fp, password);

	receiveMessage(fp, message);
	printf("%s\n", message);

	enterPassiveMode(&fp, message);

	char serverFilename[] = "1KB.zip", clientFilename[] = "1KB.zip";

	receiveFile(fp, message, serverFilename, clientFilename);

	fclose(fp);

	return 0;
}


