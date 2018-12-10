#include "connection.h"

#include <stdlib.h>

int initializeConnection(connection** conn)
{
	*conn = malloc(sizeof(connection*));
	(*conn)->IPAddress = malloc(16);

	return 0;
}

int freeConnection(connection** conn)
{
	free((*conn)->IPAddress);
	free(*conn);

	return 0;
}