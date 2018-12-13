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
#include <time.h>

int main(int argc, char** argv)
{
	int i, fd = open(argv[1], O_WRONLY | O_TRUNC | O_CREAT, 0777);

	int seed = time(NULL);
    srand(seed);
    unsigned char foobar;
    int size = strtol(argv[2], NULL, 10);

	for (i = 0; i < size; i++)
	{
		foobar = rand()%255;

		write(fd, &foobar, 1);
	}

	return 0;
}