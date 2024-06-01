#include "common.h"


//VERY USED FUNCTIONS-----------------------------------------------------------------------------
char *read_until(int fd, char end)
{
    char *string = NULL;
    char c;
    int i = 0, size;

    while (1)
    {
        size = read(fd, &c, sizeof(char));
        if (string == NULL)
        {
            string = (char *)malloc(sizeof(char));
        }
        if (c != end && size > 0)
        {
            string = (char *)realloc(string, sizeof(char) * (i + 2));
            string[i++] = c;
        }
        else
        {
            break;
        }
    }
    string[i] = '\0';
    return string;
}



//INPUT PHASE-----------------------------------------------------------------------------
void checkArgc(int argc) {
    if (argc != 2) {
        printStringWithHeader("ERROR:", "Expecting one parameter.\n");
        exit(EXIT_FAILURE);
    }
}

void openFile( char *filename, int *fd) {
    *fd = open(filename, O_RDONLY);
    if (*fd == -1) {
        printStringWithHeader("ERROR:", "opening file");
        exit(EXIT_FAILURE);
    }
}

//PRINT ON CONSOLE-----------------------------------------------------------------------------

void printStringWithHeader( char* text_,  char* string_) {
    char *buffer;
    int size = asprintf(&buffer, "%s %s\n", text_ ,string_);
    if (size == -1) {
        
        perror("asprintf");
        return;
    }
    write(1, buffer, strlen(buffer));
    free(buffer);
}

void printString( char* text_) {
    char *buffer;
    int size = asprintf(&buffer, "%s", text_ );
    if (size == -1) {
        
        perror("asprintf");
        return;
    }
    write(1, buffer, strlen(buffer));
    free(buffer);
}

void printInt( char* text_,  int int_) {
    char *buffer;
    int size = asprintf(&buffer, "%s %d\n", text_ ,int_);
    if (size == -1) {
        
        perror("asprintf");
        return;
    }
    write(1, buffer, strlen(buffer));
    free(buffer);
}

void printFrame(Frame * frame){
    printInt("TYPE:", frame->type);
    printInt("HEADERLEnght:", frame->headerLength);
    printStringWithHeader("HEADER:", frame->header);
    printStringWithHeader("DATA:", frame->data);
}

//READ FROM FILES-----------------------------------------------------------------------------
void readStringFromFile(int fd, char delimiter, char **destination) {
    char *buffer = read_until(fd, delimiter);
    if (buffer == NULL) {
        perror("read_until");
        exit(EXIT_FAILURE);
    }

    *destination = malloc(sizeof(char) * (strlen(buffer) + 1));
    if (*destination == NULL) {
        perror("malloc");
        free(buffer);
        exit(EXIT_FAILURE);
    }

    strcpy(*destination, buffer);
    (*destination)[strlen(buffer)] = '\0';
    free(buffer);
}

void readIntFromFile(int fd, char delimiter, int *destination) {
    char *buffer = read_until(fd, delimiter);
    if (buffer == NULL) {
        perror("read_until");
        exit(EXIT_FAILURE);
    }

    *destination = atoi(buffer);
    free(buffer);
}


//READ FROM CONSOLE-----------------------------------------------------------------------------
void readFromConsole(int * lenght, char (* buffer)[200]){
	*lenght = read(STDIN_FILENO, *buffer, 200);
    (*buffer)[*lenght - 1] = '\0';
}

//SOCKETS-----------------------------------------------------------------------------


int connectToServer(char *ip, int port) {
   

    struct in_addr newIp;
	int serverFd = 0;
	
    if (inet_aton(ip, &newIp) == 0) {
        printString("\nERROR: inet_aton");
        exit(EXIT_FAILURE);
    }

	//create universal server structure
	struct sockaddr_in serverSock;
    bzero(&serverSock, sizeof(serverSock));
    serverSock.sin_family = AF_INET;
    serverSock.sin_port = htons(port);
    serverSock.sin_addr = newIp;

    // Create socket
    if ((serverFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        printString("\nERROR CRETING A SOCKET");
        return -1;
    }

    // Connect to the server
    if (connect(serverFd, (void *) &serverSock, sizeof(serverSock)) < 0) {
        printString("\nERROR: CONECTION");

        exit(EXIT_FAILURE);
    }else{
		printString("\n STABLISHING CONNECTION... ");
	}


    return serverFd;
}



int createServer(int inputPort, char * inputIp) {
    
	struct sockaddr_in server;
	int listenfd = 0;
    // Create socket
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(inputIp); 
    server.sin_port = htons(inputPort);

	//Create a universal SOcket
	if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        printString("\nERROR CRETING A SOCKET\n");
        return -1;
    }

	//Bind the universal Socket created to our server previously created
	if (bind(listenfd, (struct sockaddr*)&server, sizeof(server)) < 0) {
        printString("\nERROR BINDING SOCKET");

        close(listenfd);
        return -1;
    }

    
	if(listen(listenfd, 5) < 0) {
        printString("\nERROR: LISTEN");
    }
    return listenfd;


}



//FRAMES-----------------------------------------------------------------------------


void initFrame(Frame * frame){
	frame->type = 0;
    frame->headerLength = (uint16_t) 0;
    frame->header = NULL;
    frame->data = NULL;
}

void freeFrame(Frame * frame){
	
    free(frame->header);
    free(frame->data);
}

char * createFrame(uint8_t type,  char *header,  char *data) {
    char *frame = (char *)malloc(256);
    int headerCounter = 0;
    int dataCounter = 0;

    int lengthHeader = strlen(header);
    int lengthData = strlen(data);

    if (lengthHeader > 255) {
        printString("\nERROR: HEADER TOO LONG\n");
        return 0;
    }

    frame[0] = type;
    frame[1] = lengthHeader & 0xFF;
    frame[2] = (lengthHeader >> 8) & 0xFF;

    for (int i = 0; i < lengthHeader; ++i) {
        frame[3 + i] = header[i];
        headerCounter++;
    }

    for (int i = 0; i < lengthData; ++i) {
        frame[3 + lengthHeader + i] = data[i];
        dataCounter++;
    }

    int totalLength = 3 + lengthHeader + lengthData; 
    int pad = 256 - totalLength;

    frame[totalLength] = '\0'; 
    memset(&frame[totalLength + 1], 0, pad); 

    
    return frame;
}

int readFrame(int socketFd, Frame * frame) {
    char buffer[256];
    int numBytes = read(socketFd, buffer, 256);

    if (numBytes <= 0) {
        return numBytes;
    }

    freeFrame(frame);

    frame->type = buffer[0];
    frame->headerLength = (buffer[2] << 8) | buffer[1];

    frame->header = (char *)malloc(frame->headerLength * sizeof(char));
    memcpy(frame->header, buffer + 3, frame->headerLength);

    int dataLength = numBytes - (3 + frame->headerLength); 
   
    frame->data = (char *)malloc(dataLength * sizeof(char));
    memcpy(frame->data, buffer + 3 + frame->headerLength, dataLength);
    frame->data[dataLength] = '\0'; 

    return 1;
}


int separateData(char *data, char ***destination) {
    int i = 0;
    int count = 0;
    char *dataCopy;
    char *token;

    dataCopy = strdup(data);
    if (dataCopy == NULL) {
        printString("Memory allocation failed\n");
        return -1;
    }

    token = strtok(dataCopy, "&");
    while (token != NULL) {
        count++;
        token = strtok(NULL, "&");
    }

    *destination = (char **)malloc(count * sizeof(char *));
    if (*destination == NULL) {
        printString("Memory allocation failed\n");
        free(dataCopy); 
        return -1;
    }

    strcpy(dataCopy, data);

    token = strtok(dataCopy, "&");
    while (token != NULL) {
        (*destination)[i] = (char *)malloc((strlen(token) + 1) * sizeof(char));
        if ((*destination)[i] == NULL) {
            printString("Memory allocation failed\n");
            
            for (int j = 0; j < i; j++) {
                free((*destination)[j]);
            }
            free(*destination);
            free(dataCopy);
            return -1;
        }
        strcpy((*destination)[i], token);
        i++;
        token = strtok(NULL, "&");
    }
    free(dataCopy);
	return count;
}

//DOCUMENTATION SENDING FRAMES------------------------------------------------------------
#define MAX_FRAME_SIZE 256

//---Poole → Discovery NEW CONNECTION
void sendNewConnectionPooleDiscovery(int socketFd,  char * data) {
    char * frameToSend = createFrame(0x01, "NEW_POOLE", data);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}


void sendOkConnectionDiscoveryPoole(int socketFd) {
    char * frameToSend = createFrame(0x01, "CON_OK", NULL);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}


void sendKoConnectionDiscoveryPoole(int socketFd) {
    char * frameToSend = createFrame(0x01, "CON_KO", NULL);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}

//---Bowman → Discovery NEW CONNECTION
void sendNewConnectionBowmanDiscovery(int socketFd,  char * data) {
    char * frameToSend = createFrame(0x01, "NEW_BOWMAN", data);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}

void sendOkConnectionDiscoveryBowman(int socketFd, char * data) {
    char * frameToSend = createFrame(0x01, "CON_OK", data);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}


void sendKoConnectionDiscoveryBowman(int socketFd) {
    char * frameToSend = createFrame(0x01, "CON_KO", NULL);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}

//---Bowman → Poole NEW CONNECTION
void sendNewConnectionBowmanPoole(int socketFd,  char * data) {
    char * frameToSend = createFrame(0x01, "NEW_BOWMAN", data);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}

void sendOkConnectionPooleBowman(int socketFd) {
    char * frameToSend = createFrame(0x01, "CON_OK", NULL);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}


void sendKoConnectionPooleBowman(int socketFd) {
    char * frameToSend = createFrame(0x01, "CON_KO", NULL);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}

//----LIST SONGS 
void listSongs(int socketFd) {
    char * frameToSend = createFrame(0x02, "LIST_SONGS", NULL);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}


void sendSongsResponse(int socketFd,  char * songs) {
    char * frameToSend = createFrame(0x02, "SONGS_RESPONSE", songs);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}

//---LIST PLAYLISTS 
void listPlaylists(int socketFd) {
    char * frameToSend = createFrame(0x02, "LIST_PLAYLISTS", NULL);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}


void sendPlaylistsResponse(int socketFd,  char * playlists) {
    char * frameToSend = createFrame(0x02, "SONGS_RESPONSE", playlists);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}


void downloadSong(int socketFd,  char * songName) {
    char * frameToSend = createFrame(0x03, "DOWNLOAD_SONG", songName);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}


void downloadPlaylist(int socketFd,  char * playlistName) {
    char * frameToSend = createFrame(0x03, "DOWNLOAD_LIST", playlistName);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}


void sendFileInfo(int socketFd,  char * fileInfo) {
    char * frameToSend = createFrame(0x04, "NEW_FILE", fileInfo);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}


void sendFileData(int socketFd,  char * fileData) {
    char * frameToSend = createFrame(0x04, "FILE_DATA", fileData);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}


void sendCheckResult(int socketFd,  char * result) {
    char * frameToSend = createFrame(0x05, result, NULL);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}


void sendLogout(int socketFd,  char * userName) {
    char * frameToSend = createFrame(0x06, "EXIT", userName);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}


void sendLogoutResponse(int socketFd,  char * result) {
    char * frameToSend = createFrame(0x06, result, NULL);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}


void sendUnknownFrame(int socketFd) {
    char * frameToSend = createFrame(0x07, "UNKNOWN", NULL);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}




