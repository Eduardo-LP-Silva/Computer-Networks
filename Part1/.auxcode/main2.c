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

int main()
{
	int i, fd = open("lab2.pdf", O_RDONLY), fd2 = open("newlab2.pdf", O_RDONLY);
	unsigned char a, b;

	for (i = 0; ; i++)
	{
		read(fd, &a, 1);
		read(fd2, &b, 1);

		if (i > 620470)
		{
			printf("i = %i |%u|%u|\n", i, a, b);
		}
	}

	return 0;
}