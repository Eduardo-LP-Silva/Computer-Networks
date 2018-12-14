#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdio.h>
#include <fcntl.h>

int getFreeFilePointer(FILE** fpArray);
int closeSockets(FILE** fileArray);
void sigalrm_handler(int signal);
int setHandler();
int isNumber(char input);
void printUsage();
int findFirst(char* str, char target);
int findLast(char* str, char target);
int extractFromArgument(char* input, char* username, char* password, char* hostname, char* filename);
void printPercentage(double percentage);
void printTransferRate(double rate);
void clearScreen();
int splitFilename(char* fullPath, char* path, char* filename);



#endif