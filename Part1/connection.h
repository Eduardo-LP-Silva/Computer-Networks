#ifndef CONNECTION_H
#define CONNECTION_H

#include <stdio.h>


typedef struct
{
	char* IPAddress;
	int port;
	FILE* fp;
}
connection;

int initializeConnection(connection** conn);
int freeConnection(connection** conn);

#endif