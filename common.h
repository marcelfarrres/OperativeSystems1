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

#include "structures.h"


#define OPT_CONNECT "CONNECT"
#define OPT_CHECK_DOWNLOADS1 "CHECK"
#define OPT_CHECK_DOWNLOADS2 "DOWNLOADS"

//VERY USED FUNCTIONS-----------------------------------------------------------------------------
char *read_until(int fd, char end);


//INPUT PHASE-----------------------------------------------------------------------------
void checkArgc(int argc);
void openFile( char *filename, int *fd);

//PRINT ON CONSOLE-----------------------------------------------------------------------------
void printStringWithHeader(  char* text_,  char* string_);
void printString( char* text_);
void printInt( char* text_,  int int_);
void printFrame(Frame * frame);
void printPooleServer(PooleServer *server);

//READ FROM FILES-----------------------------------------------------------------------------
void readStringFromFile(int fd, char delimiter, char ** destination);
void readIntFromFile(int fd, char delimiter, int *destination);

//READ FROM CONSOLE-----------------------------------------------------------------------------
void readFromConsole(int * lenght, char (* buffer)[200]);

//SOCKETS-----------------------------------------------------------------------------
int createServer(int inputPort, char * inputIp);
int connectToServer(char *ip, int port);

//FRAMES-----------------------------------------------------------------------------
void initFrame(Frame * frame);
void freeFrame(Frame * frame);
char * createFrame(uint8_t type,  char *header,  char *data);
int readFrame(int socketFd, Frame * frame);
void freeSeparatedData(char *** data, int * num);
int separateData(char *data, char ***destination, int * num);




//DOCUMENTATION SENDING FRAMES:
void sendNewConnectionPooleDiscovery(int socketFd,  char * data);
void sendOkConnectionDiscoveryPoole(int socketFd);
void sendKoConnectionDiscoveryPoole(int socketFd);

void sendNewConnectionBowmanDiscovery(int socketFd,  char * data);
void sendOkConnectionDiscoveryBowman(int socketFd, char * data);
void sendKoConnectionDiscoveryBowman(int socketFd);

void sendNewConnectionBowmanPoole(int socketFd,  char * data);
void sendOkConnectionPooleBowman(int socketFd);
void sendKoConnectionPooleBowman(int socketFd);

void listSongs(int socketFd);
void sendSongsResponse(int socketFd,  char * songs);

void listPlaylists(int socketFd);
void sendPlaylistsResponse(int socketFd,  char * playlists);

void downloadSong(int socketFd,  char * songName);
void downloadPlaylist(int socketFd,  char * playlistName);

void sendFileInfo(int socketFd,  char * fileInfo);
void sendFileData(int socketFd,  char * fileData);

void sendCheckResult(int socketFd,  char * result);

void sendLogout(int socketFd,  char * userName);
void sendLogoutResponse(int socketFd,  char * result);

void sendUnknownFrame(int socketFd);






#endif