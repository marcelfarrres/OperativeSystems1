#include "common.h"


#define MAX_FRAME_SIZE 256
#define MAX_FRAME_SIZE_SENDING 100







//COMMON FUNCTIONS-----------------------------------------------------------------------------
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

char *concatenateWords(char **words, int wordCount) {
    int totalLength = 0;
    for (int i = 1; i < wordCount; ++i) {
        totalLength += strlen(words[i]);
        if (i < wordCount - 1) totalLength++; 
    }

    char *result = (char *)malloc((totalLength + 1) * sizeof(char)); 
    if (!result) {
        printString("\nMemory allocation failed\n");
        return NULL;
    }

    result[0] = '\0'; 
    for (int i = 1; i < wordCount; ++i) {
        strcat(result, words[i]);
        if (i < wordCount - 1) strcat(result, " ");
    }

    return result;
}

char *concatenateWords2(char **words, int wordCount) {
    int totalLength = 0;
    for (int i = 2; i < wordCount; ++i) {
        totalLength += strlen(words[i]);
        if (i < wordCount - 1) totalLength++; 
    }

    char *result = (char *)malloc((totalLength + 1) * sizeof(char)); 
    if (!result) {
        printString("\nMemory allocation failed\n");
        return NULL;
    }

    result[0] = '\0'; 
    for (int i = 2; i < wordCount; ++i) {
        strcat(result, words[i]);
        if (i < wordCount - 1) strcat(result, " ");
    }

    return result;
}

int calculateMD5Checksum(const char *filePath, char *md5Checksum) {
    int pipeFileDescriptors[2];
    pid_t childPID;
    char buffer[33];

    // Create a pipe
    if (pipe(pipeFileDescriptors) == -1) {
        perror("Failed to create pipe");
        return -1;
    }

    // Fork a process
    childPID = fork();
    if (childPID == -1) {
        perror("Failed to fork process");
        return -1;
    }

    if (childPID == 0) { // Child process
        close(pipeFileDescriptors[0]); // Close the read end of the pipe
        if (dup2(pipeFileDescriptors[1], STDOUT_FILENO) == -1) {
            perror("Failed to redirect STDOUT");
            exit(EXIT_FAILURE);
        }
        close(pipeFileDescriptors[1]); // Close the write end of the pipe

        execlp("md5sum", "md5sum", filePath, NULL);
        perror("Failed to execute md5sum");
        exit(EXIT_FAILURE);
    } else { // Parent process
        close(pipeFileDescriptors[1]); // Close the write end of the pipe

        int bytesRead = read(pipeFileDescriptors[0], buffer, 32);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0'; // Ensure the buffer is null-terminated
            strncpy(md5Checksum, buffer, 32);
            md5Checksum[32] = '\0'; // Null-terminate the MD5 checksum
        } else {
            perror("Failed to read data from pipe");
            close(pipeFileDescriptors[0]);
            waitpid(childPID, NULL, 0);
            return -1;
        }
        close(pipeFileDescriptors[0]); // Close the read end of the pipe
        waitpid(childPID, NULL, 0); // Wait for the child process to exit
    }

    return 0;
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

void printOnlyInt(int int_) {
    char *buffer;
    int size = asprintf(&buffer, "%d" ,int_);
    if (size == -1) {
        
        perror("asprintf");
        return;
    }
    write(1, buffer, strlen(buffer));
    free(buffer);
}

void printChar(char ch) {
    char *buffer;
    int size = asprintf(&buffer, "%c" ,ch);
    if (size == -1) {
        
        perror("asprintf");
        return;
    }
    write(1, buffer, strlen(buffer));
    free(buffer);
}

void printFrame(Frame * frame){
	printString("\n");
    printString("┏━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
    printString("┃          FRAME            ┃\n");
    printString("┣━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\n");
    printInt("┃ TYPE:", frame->type);
    printInt("┃ DATALength:", (int) strlen(frame->data));

    printInt("┃ HEADERLenght:", frame->headerLength);
    printStringWithHeader("┃ HEADER:", frame->header);
    printStringWithHeader("┃ DATA:", frame->data);
    printInt("┃ ID:", frame->id);

    printString("┗━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n");

    
}

void printPooleServer(PooleServer *server) {
    printString("\n");
    printString("┏━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
    printString("┃       POOLE SERVER        ┃\n");
    printString("┣━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\n");

    printStringWithHeader("┃ NAME:", server->name);
    printInt("┃ PORT:", server->port);
    printStringWithHeader("┃ IP:", server->ip);
    printInt("┃ NUM CONNECTIONS:", server->numConnections);
	for (int i = 0; i < server->numConnections; i++) {
            printString("┃  ");
            printOnlyInt(i + 1);
            printStringWithHeader("):", (server->bowmans)[i]);
        }
    printString("┗━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n");  

}

void printAllPooleServers(PooleServer **servers, int numServers) {
    if(numServers < 1){
        printString("\n");
        printString("╔═══════════════════════════════╗\n");
        printString("║       ALL POOLE SERVERS       ║\n");
        printString("╠═══════════════════════════════╣\n");
        printString("║             EMPTY             ║\n");
        printString("╠═══════════════════════════════╝\n");
        printString("║\n");
        printString("╨\n\n"); 
    }else{
        for (int e = 0; e < numServers; e++) {
            if( e + 1 == numServers && e == 0 ){
                printString("\n");
                printString("╔═══════════════════════════════╗\n");
                printString("║       ALL POOLE SERVERS       ║\n");
                printString("╠═══════════════════════════════╣\n");
                printString("║ ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━┓ ║\n");
                printStringWithHeader("║ ┃ NAME:", servers[e]->name);
                printInt("║ ┃ PORT:", servers[e]->port);
                printStringWithHeader("║ ┃ IP:", servers[e]->ip);
                printInt("║ ┃ NUM CONNECTIONS:", servers[e]->numConnections);
	            for (int i = 0; i < servers[e]->numConnections; i++) {
                        printString("║ ┃  ");
                        printOnlyInt(i + 1);
                        printStringWithHeader("):", (servers[e]->bowmans)[i]);
                    }
                printString("║ ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━┛ ║\n");
                printString("╠═══════════════════════════════╝\n");
                printString("║\n");
                printString("╨\n\n");   


            }else if(e == 0){
                printString("\n");
                printString("╔═══════════════════════════════╗\n");
                printString("║       ALL POOLE SERVERS       ║\n");
                printString("╠═══════════════════════════════╣\n");
                printString("║ ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━┓ ║\n");
                printStringWithHeader("║ ┃ NAME:", servers[e]->name);
                printInt("║ ┃ PORT:", servers[e]->port);
                printStringWithHeader("║ ┃ IP:", servers[e]->ip);
                printInt("║ ┃ NUM CONNECTIONS:", servers[e]->numConnections);
	            for (int i = 0; i < servers[e]->numConnections; i++) {
                        printString("║ ┃  ");
                        printOnlyInt(i + 1);
                        printStringWithHeader("):", (servers[e]->bowmans)[i]);
                    }
                printString("║ ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━┛ ║ \n");  


            }else if( e + 1 == numServers){
                printString("║                               ║\n");
                printString("║ ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━┓ ║\n");

                printStringWithHeader("║ ┃ NAME:", servers[e]->name);
                printInt("║ ┃ PORT:", servers[e]->port);
                printStringWithHeader("║ ┃ IP:", servers[e]->ip);
                printInt("║ ┃ NUM CONNECTIONS:", servers[e]->numConnections);
	            for (int i = 0; i < servers[e]->numConnections; i++) {
                        printString("║ ┃  ");
                        printOnlyInt(i + 1);
                        printStringWithHeader("):", (servers[e]->bowmans)[i]);
                    }
                printString("║ ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━┛ ║\n");  
                printString("╠═══════════════════════════════╝\n");
                printString("║\n");
                printString("╨\n\n");


            }else{
                printString("║                               ║\n");
                printString("║ ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━┓ ║\n");

                printStringWithHeader("║ ┃ NAME:", servers[e]->name);
                printInt("║ ┃ PORT:", servers[e]->port);
                printStringWithHeader("║ ┃ IP:", servers[e]->ip);
                printInt("║ ┃ NUM CONNECTIONS:", servers[e]->numConnections);
	            for (int i = 0; i < servers[e]->numConnections; i++) {
                        printString("║ ┃  ");
                        printOnlyInt(i + 1);
                        printStringWithHeader("):", (servers[e]->bowmans)[i]);
                    }
                printString("║ ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━┛ ║\n"); 
            } 

        }
    }
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
    (*destination)[strlen(buffer) - 1] = '\0';
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



int createServer(int inputPort) {
    
	struct sockaddr_in server;
	int listenfd = 0;
    // Create socket
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY; 
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

    
	if(listen(listenfd, 10) < 0) {
        printString("\nERROR: LISTEN");
    }
    return listenfd;


}



//FRAMES-----------------------------------------------------------------------------


void initFrame(Frame *frame) {
    if (frame == NULL) return;

    frame->type = 0;
    frame->headerLength = 0;
    frame->dataLength = 0;

    frame->id = 0;  // Set the integer ID to 0

    frame->header = NULL;
    frame->data = NULL;
}

void freeFrame(Frame *frame) {
    if (frame == NULL) return;

    free(frame->header);  // Free the memory for the header
    frame->header = NULL;

    free(frame->data);    // Free the memory for the data
    frame->data = NULL;
    

    // No need to free 'id' since it's an integer and not dynamically allocated
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
    memset(&frame[totalLength], 0, pad); 

    
    return frame;
}

int readFrame(int socketFd, Frame * frame) {
	freeFrame(frame);
    initFrame(frame);

    char buffer[256];
    int numBytes = read(socketFd, buffer, 256);
    if (numBytes <= 0) {
        return numBytes;
    }

    frame->type = buffer[0];
    frame->headerLength = (buffer[2] << 8) | buffer[1];
    frame->header = (char *)malloc(((frame->headerLength) + 1) * sizeof(char));
    memcpy(frame->header, buffer + 3, frame->headerLength);
	(frame->header)[((frame->headerLength))] = '\0';

    int dataLength = numBytes - (3 + frame->headerLength); 
   
    frame->data = (char *)malloc(dataLength * sizeof(char));
    memcpy(frame->data, buffer + 3 + frame->headerLength, dataLength);
    (frame->data)[dataLength - 1] = '\0'; 

    return dataLength;
}

void freeSeparatedData(char *** data, int *num){
    if(*num > 0){

        for (int j = 0; j < *num; j++) {
                free((*data)[j]);
            }
        free(*data);
    }
	*num = 0;
}

int separateData(char *data, char ***destination, int *num) {
    int i = 0;
    int count = 0;
    char *dataCopy;
    char *token;

	freeSeparatedData(destination, num);

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


//FILES-----------------------------------------------------------------------------


char **splitString(char *longString, int *numStrings) {
    int totalLength = strlen(longString);
    int numFrames = (totalLength + MAX_FRAME_SIZE_SENDING - 1) / MAX_FRAME_SIZE_SENDING; // Calculate number of frames
    *numStrings = numFrames;

    char **splitStrings = malloc(numFrames * sizeof(char *));
    if (splitStrings == NULL) {
        perror("malloc");
        return NULL;
    }

    int remainingLength = totalLength;
    char *currentData = longString;
    for (int i = 0; i < numFrames; i++) {
        int dataLength = (remainingLength > MAX_FRAME_SIZE_SENDING) ? MAX_FRAME_SIZE_SENDING : remainingLength;

        char *splitString = malloc(dataLength + 1); // Allocate memory for split string
        if (splitString == NULL) {
            perror("malloc");
            for (int j = 0; j < i; j++) {
                free(splitStrings[j]);
            }
            free(splitStrings);
            return NULL;
        }
        memcpy(splitString, currentData, dataLength); // Copy data to split string
        splitString[dataLength] = '\0'; // Null-terminate the split string

        splitStrings[i] = splitString;

        remainingLength -= dataLength;
        currentData += dataLength;
    }

    return splitStrings;
}

char *joinStrings(char **splitStrings, int numStrings) {
    int totalLength = 0;
    for (int i = 0; i < numStrings; i++) {
        totalLength += strlen(splitStrings[i]);
    }

    char *longString = malloc(totalLength + 1); // +1 for null terminator
    if (longString == NULL) {
        perror("malloc");
        return NULL;
    }

    longString[0] = '\0'; // Ensure the string is initially empty
    for (int i = 0; i < numStrings; i++) {
        strcat(longString, splitStrings[i]); // Concatenate each split string
    }

    return longString;
}

// Function to read song names from a directory and divide them into frames
int readSongsFromFolder( char *folderPath, char ****songs, int *numSongs, int *numFrames, int** numberOfSongsPerFrame) {
    printStringWithHeader("folderPath:", folderPath);
    DIR *dir = opendir(folderPath);
    if (dir == NULL) {
        perror("opendir");
        return -1;
    }

    struct dirent *ent;
    char **songList = NULL;
    int songCount = 0;

    // Read the directory entries
    while ((ent = readdir(dir)) != NULL) {
        // Skip the current and parent directory entries
        if (ent->d_type == DT_REG && strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
            char **temp = realloc(songList, sizeof(char *) * (songCount + 1));
            if (temp == NULL) {
                perror("realloc");
                for (int i = 0; i < songCount; i++) {
                    free(songList[i]);
                }
                free(songList);
                closedir(dir);
                return -1;
            }
            songList = temp;
            songList[songCount] = strdup(ent->d_name);
            if (songList[songCount] == NULL) {
                perror("strdup");
                for (int i = 0; i < songCount; i++) {
                    free(songList[i]);
                }
                free(songList);
                closedir(dir);
                return 0;
            }
            songCount++;
        }
    }
    closedir(dir);

    // Calculate the total length of all song names including separators (e.g., newlines or null terminators)
    int totalLength = 0;
    for (int i = 0; i < songCount; i++) {
        totalLength += ((int) strlen(songList[i])) + 1; // Adding 1 for the separator
    }

    // Calculate the number of 256-byte frames needed
    // Header length should be taken into account if there is a fixed header size
    int framesNeeded = (totalLength) / 50;
    int finalNumberOfFrames = ((int) framesNeeded) + 1;

    // Allocate the outer list of lists
    char ***songFrames = malloc(finalNumberOfFrames * sizeof(char **));
    int *songCountPerFrame =  malloc(finalNumberOfFrames * sizeof(int));
    
    if (songFrames == NULL) {
        perror("malloc");
        for (int i = 0; i < songCount; i++) {
            free(songList[i]);
        }
        free(songList);
        return -1;
    }

    // Distribute the songs equally among the frames
    int songsPerFrame = songCount / finalNumberOfFrames;
    int remainder = songCount % finalNumberOfFrames;
    int songIndex = 0;

    for (int i = 0; i < finalNumberOfFrames; i++) {
        int currentFrameSize = songsPerFrame + (remainder > 0 ? 1 : 0);
        remainder--;

        songFrames[i] = malloc(currentFrameSize * sizeof(char *));
        if (songFrames[i] == NULL) {
            perror("malloc");
            for (int j = 0; j < i; j++) {
                for (int k = 0; k < (songsPerFrame + (remainder > j ? 1 : 0)); k++) {
                    free(songFrames[j][k]);
                }
                free(songFrames[j]);
            }
            for (int k = songIndex; k < songCount; k++) {
                free(songList[k]);
            }
            free(songList);
            free(songFrames);
            return -1;
        }

        for (int j = 0; j < currentFrameSize; j++) {
            songFrames[i][j] = songList[songIndex++];
        }
        songCountPerFrame[i] = currentFrameSize;
    }

    free(songList);

    *songs = songFrames;
    *numSongs = songCount;
    *numFrames = finalNumberOfFrames;
    *numberOfSongsPerFrame = songCountPerFrame;

    return 1;
}



int readPlaylistsFromFolder(char *folderPath, Playlist **finalPlaylists, int *finalNumPlaylists) {
    char *miniBuffer;
    asprintf(&miniBuffer, "%s/playlists", folderPath);

    printStringWithHeader("folderPath:", miniBuffer);
    DIR *dir = opendir(miniBuffer);
    if (dir == NULL) {
        perror("opendir");
        free(miniBuffer);
        return -1;
    }
    free(miniBuffer);

    struct dirent *ent;
    Playlist *playlists = NULL;
    int playlistCount = 0;

    // Read the directory entries
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_type == DT_DIR && strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
            playlists = (Playlist *)realloc(playlists, sizeof(Playlist) * (playlistCount + 1));
            asprintf(&playlists[playlistCount].name, "%s", ent->d_name);
            playlists[playlistCount].songs = NULL; // Initialize the songs pointer to NULL
            playlistCount++;
        }
    }
    closedir(dir); // Close the directory after reading its contents

    for (int i = 0; i < playlistCount; i++) {
        asprintf(&miniBuffer, "%s/playlists/%s", folderPath, playlists[i].name);

        dir = opendir(miniBuffer);
        if (dir == NULL) {
            perror("opendir");
            free(miniBuffer);
            // Free previously allocated playlists and their songs
            for (int j = 0; j < i; j++) {
                free(playlists[j].name);
                for (int k = 0; k < playlists[j].numSongs; k++) {
                    free(playlists[j].songs[k]);
                }
                free(playlists[j].songs);
            }
            free(playlists);
            return -1;
        }

        int songsCount = 0;
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_REG && strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
                playlists[i].songs = realloc(playlists[i].songs, sizeof(char *) * (songsCount + 1));
                asprintf(&playlists[i].songs[songsCount], "%s", ent->d_name);
                songsCount++;
            }
        }
        playlists[i].numSongs = songsCount;

        free(miniBuffer);
        closedir(dir); // Close the directory after reading its contents
    }

    *finalNumPlaylists = playlistCount;
    *finalPlaylists = playlists;

    return 1;
}


void freeSongsList(char ****songs, int numFrames, int * numSongsPerFrame) {
    if (*songs == NULL) {
        return;
    }

    for (int i = 0; i < numFrames; i++) {
        if ((*songs)[i] != NULL) {
            for (int j = 0; j < numSongsPerFrame[i]; j++) {
                free((*songs)[i][j]);
            }
            free((*songs)[i]);
        }
    }
    free(*songs);
    free(numSongsPerFrame);
}

void freePlaylists(Playlist *finalPlaylists, int numPlaylists) {
    for (int i = 0; i < numPlaylists; i++) {
        for (int j = 0; j < finalPlaylists[i].numSongs; j++) {
            free(finalPlaylists[i].songs[j]);
        }
        free(finalPlaylists[i].songs);
        
        free(finalPlaylists[i].name);
        
       
    }
    
    free(finalPlaylists);
}
//DOCUMENTATION SENDING FRAMES------------------------------------------------------------

//---Poole → Discovery NEW CONNECTION
void sendNewConnectionPooleDiscovery(int socketFd,  char * data) {
    char * frameToSend = createFrame(0x01, "NEW_POOLE", data);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}


void sendOkConnectionDiscoveryPoole(int socketFd) {
    char * frameToSend = createFrame(0x01, "CON_OK", "EMPTY");
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}


void sendKoConnectionDiscoveryPoole(int socketFd) {
    char * frameToSend = createFrame(0x01, "CON_KO", "EMPTY");
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
    char * frameToSend = createFrame(0x01, "CON_KO", "EMPTY");
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
    char * frameToSend = createFrame(0x01, "CON_OK", "EMPTY");
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}


void sendKoConnectionPooleBowman(int socketFd) {
    char * frameToSend = createFrame(0x01, "CON_KO", "EMPTY");
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}

//----LIST SONGS 
void sendListSongs(int socketFd) {
    char * frameToSend = createFrame(0x02, "LIST_SONGS", "EMPTY");
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}


void sendSongsResponse(int socketFd,  char * songs) {
    char * frameToSend = createFrame(0x02, "SONGS_RESPONSE", songs);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}

//---LIST PLAYLISTS 
void sendListPlaylists(int socketFd) {
    char * frameToSend = createFrame(0x02, "LIST_PLAYLISTS", "EMPTY");
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}


void sendPlaylistsResponse(int socketFd,  char * playlists) {
    char * frameToSend = createFrame(0x02, "SONGS_RESPONSE", playlists);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}

//SEND FILES
void sendDownloadSong(int socketFd,  char * songName) {
    char * frameToSend = createFrame(0x03, "DOWNLOAD_SONG", songName);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}

void sendDownloadPlaylist(int socketFd,  char * playlistName) {
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


void sendCheckResult(int socketFd, int i) {
    if(i != 0){
        char * frameToSend = createFrame(0x05, "CHECK_OK", "EMPTY");
        if(write(socketFd, frameToSend, MAX_FRAME_SIZE) < 1){
            printString("ERROR: Sending OK confirmation");
        }else{
            //printInt("We wrote CHECK_OK succesfully on socket:", socketFd);
        };
        free(frameToSend);
    }else{
        char * frameToSend = createFrame(0x05, "CHECK_KO", "EMPTY");
        if(write(socketFd, frameToSend, MAX_FRAME_SIZE) < 1){
            printString("ERROR: Sending KO confirmation");
        };
        free(frameToSend);
    }
    
}

//LOG OUT
void sendLogoutPoole(int socketFd,  char * userName) {
    char * frameToSend = createFrame(0x06, "EXIT_POOLE", userName);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}

void sendLogoutBowman(int socketFd,  char * userName) {
    char * frameToSend = createFrame(0x06, "EXIT_BOWMAN", userName);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}


void sendLogoutResponse(int socketFd) {
    char * frameToSend = createFrame(0x06, "CONOK", "EMPTY");
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}


void sendUnknownFrame(int socketFd) {
    char * frameToSend = createFrame(0x07, "UNKNOWN", "EMPTY");
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}


//EXTRA

//for diconnection bowman from discovery:
void sendRemoveConnectionBowman(int socketFd,  char * userName){
    char * frameToSend = createFrame(0x06, "LOGOUT_BOWMAN", userName);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}

//for telling a bowman the song doesnt exist
void sendSoungNotFound(int socketFd) {
    char * frameToSend = createFrame(0x03, "NOT_FOUND", "EMPTY");
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}

void sendPlayListNotFound(int socketFd) {
    char * frameToSend = createFrame(0x03, "PLAYLIST_NOT_FOUND", "EMPTY");
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}

void sendPlayListFound(int socketFd, char * fileData) {
    char * frameToSend = createFrame(0x03, "PLAYLIST_FOUND", fileData);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}



//Monolith--------------------------------------------------
void sendStatToMonolith(int socketFd, char * fileData) {
    char * frameToSend = createFrame(0x01, "STAT", fileData);
    write(socketFd, frameToSend, MAX_FRAME_SIZE);
    free(frameToSend);
}

