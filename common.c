#include "common.h"


//VERY USED FUNCTIONS-----------------------------------------------------------------------------
char * read_until(int fd, char end) {
	char *string = NULL;
	char c;
	int i = 0, size;

	while (1) {
		size = read(fd, &c, sizeof(char));
		if(string == NULL){
			string = (char *) malloc(sizeof(char));
		}
		if(c != end && size > 0){
			string = (char *) realloc(string, sizeof(char)*(i + 2));
			string[i++] = c;
		}else{
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

void openFile(const char *filename, int *fd) {
    *fd = open(filename, O_RDONLY);
    if (*fd == -1) {
        printStringWithHeader("ERROR:", "opening file");
        exit(EXIT_FAILURE);
    }
}

//PRINT ON CONSOLE-----------------------------------------------------------------------------

void printStringWithHeader(const char* text_, const char* string_) {
    char *buffer;
    int size = asprintf(&buffer, "%s %s\n", text_ ,string_);
    if (size == -1) {
        
        perror("asprintf");
        return;
    }
    write(1, buffer, strlen(buffer));
    free(buffer);
}

void printString(const char* text_) {
    char *buffer;
    int size = asprintf(&buffer, "%s", text_ );
    if (size == -1) {
        
        perror("asprintf");
        return;
    }
    write(1, buffer, strlen(buffer));
    free(buffer);
}

void printInt(const char* text_, const int int_) {
    char *buffer;
    int size = asprintf(&buffer, "%s %d\n", text_ ,int_);
    if (size == -1) {
        
        perror("asprintf");
        return;
    }
    write(1, buffer, strlen(buffer));
    free(buffer);
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
#define OPT_CHECK_DOWNLOADS2 "DOWNLOADS"

char * createFrame(uint8_t type, const char *header, const char *data) {
    char *frame = (char *)malloc(256);
	int headerCounter = 0;
	int dataCounter = 0;

    size_t lengthHeader = strlen(header);
    size_t lengthData = strlen(data);

    if (lengthHeader > 255) {
        printString("\nERROR: HEADER TOO LONG\n");
        return 0;
    }

    frame[0] = type;

    frame[1] = lengthHeader & 0xFF;
    frame[2] = (lengthHeader >> 8) & 0xFF;

   
    for (size_t i = 0; i < lengthHeader; ++i) {
        frame[3 + i] = header[i];
		headerCounter++;
    }


    for (size_t i = 0; i < lengthData; ++i) {
        frame[4 + lengthHeader + i] = data[i];
		dataCounter++;

    }
  
    size_t totalLength = 3 + lengthHeader + lengthData + 1;
    size_t pad = 256 - totalLength;

    frame[(int)totalLength] = '\0';
    memset(&frame[((int)totalLength ) + 1], 0, pad);

    printInt("\ntotalLength: ", totalLength );
    printInt("\npad: ", pad );
    printInt("\nheaderCounter: ", headerCounter );
    printInt("\ndataCounter: ", dataCounter );

   return frame;
}

typedef struct{
    uint8_t type;
    uint16_t headerLength;
    char *header;
    char *data;
}Frame; 

void readFrame(int socketFd){
    char frame[256];
     read(socketFd, frame, 256);
	printString(frame);

    
}


