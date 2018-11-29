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
#define SERVER_HOSTNAME "ftp.fe.up.pt"
int flag = 1;


void sigalrm_handler(int signal)
{
	// printf("Message timed out!\n");
	flag = 0;
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


int receiveMessage(int sockfd, char* message)
{
	int bytes;
	char buffer[256];
	message[0] = 0;
	buffer[0] = 0;

	while (flag)
	{
		alarm(1);

		bytes = read(sockfd, buffer, 256);
		if (bytes < 1)
			return 1;

		buffer[bytes] = 0;

		alarm(0); // Cancel alarm

		if (flag)
			strcat(message, buffer);
	}

	flag = 1;
	return 0;
}

int login(int sockfd, char* username, char* password)
{
	if (write(sockfd, username, strlen(username)) != strlen(username))
		return 1;

	if (write(sockfd, password, strlen(password)) != strlen(password))
		return 1;

	return 0;
}

int main(int argc, char** argv)
{
	
	
	int	sockfd;
	struct	sockaddr_in server_addr;
	char	message[256] = "Mensagem de teste na travessia da pilha TCP/IP\n";  
	int	bytes;

	struct hostent * ent = gethostbyname(SERVER_HOSTNAME);
	char IPAdress[16];
		
	inet_ntop(AF_INET, ent->h_addr_list[0], IPAdress, INET_ADDRSTRLEN);
	
	/*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(IPAdress);	/*32 bit Internet address network byte ordered*/
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

	struct sigaction sigalrm_action;

	sigalrm_action.sa_handler = sigalrm_handler;
	sigemptyset(&sigalrm_action.sa_mask);
	sigalrm_action.sa_flags = 0;

	if (sigaction(SIGALRM, &sigalrm_action, NULL) < 0)
	{
		fprintf(stderr,"Unable to install SIGINT handler\n");
		return 1;
	}

		/*send a string to the server*/
	// bytes = write(sockfd, buf, strlen(buf));
	// printf("Bytes escritos %d\n", bytes);

	receiveMessage(sockfd, message);
	printf("%s\n", message);
	
	char username[] = "anonymous", password[] = "up201604828";
	
	printf("%i\n", write(sockfd, username, strlen(username))); 
	
	receiveMessage(sockfd, message);
	printf("%s\n", message);

	printf("%i\n", write(sockfd, password, strlen(password))); 

	receiveMessage(sockfd, message);
	printf("%s\n", message);

	sleep(2);

	receiveMessage(sockfd, message);
	printf("%s\n", message);

	close(sockfd);
	return 0;
}


