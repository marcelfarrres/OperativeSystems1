#ifndef _COMMON_H_
#define _COMMON_H_

#define _GNU_SOURCE
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


#define OPT_CONNECT "CONNECT"
#define OPT_CHECK_DOWNLOADS1 "CHECK"
#define OPT_CHECK_DOWNLOADS2 "DOWNLOADS"

//VERY USED FUNCTIONS-----------------------------------------------------------------------------
char *read_until(int fd, char end);

//INPUT PHASE-----------------------------------------------------------------------------
void checkArgc(int argc);
void openFile(const char *filename, int *fd);

//PRINT ON CONSOLE-----------------------------------------------------------------------------
void printStringWithHeader( const char* text_, const char* string_);
void printString(const char* text_);
void printInt(const char* text_, const int int_);

//READ FROM FILES-----------------------------------------------------------------------------
void readStringFromFile(int fd, char delimiter, char ** destination);
void readIntFromFile(int fd, char delimiter, int *destination);

//READ FROM CONSOLE-----------------------------------------------------------------------------
void readFromConsole(int * lenght, char (* buffer)[200]);

//SOCKETS-----------------------------------------------------------------------------
int createServer(int inputPort, char * inputIp);
int connectToServer(char *ip, int port);

//FRAMES-----------------------------------------------------------------------------
char * createFrame(uint8_t type, const char *header, const char *data);
void readFrame(int socketFd);





#endif