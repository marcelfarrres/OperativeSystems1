#include "common.h"
#include "structures.h"
#include "semaphore_v2.h"

Poole poole;
int pooleFd = -1; //POOLE FILE
int discoverySocketFd = -1;
fd_set setOfSockFd;
int *sockets;
int numberOfSockets;
Frame frame; 
int thisPooleFd;
char** separatedData;
int numberOfData = 0;

int pipefd[2];
pid_t pid;

struct sockaddr_in c_addr;
socklen_t c_len = sizeof (c_addr);


semaphore sem;



//SIGNALS PHASE-----------------------------------------------------------------
void ctrl_C_function_monolith(){
    printString("\nEXITED MONOLITITH\n");
    //SEM_destructor(&sem);
    freeSeparatedData(&separatedData, &numberOfData);
    close(pipefd[0]);
    exit(EXIT_SUCCESS);

}

void ctrl_C_function() {
    if(discoverySocketFd > 0){
        sendLogoutPoole(discoverySocketFd, poole.name);
        int result = readFrame(discoverySocketFd, &frame);
        if (result <= 0) {
            printString("\nERROR: OK not receieved\n");
            
            
        }else if(strcmp(frame.header, "CONKO") == 0){
            printString("\nERROR:Discovery KO CONNECTION.\n");
            printFrame(&frame);
            
    
        }else if(strcmp(frame.header, "CONOK") != 0){
            printString("\nERROR: not what we were expecting\n");
            printFrame(&frame);
            
        }else{
            printFrame(&frame);
            printString("\nConfirmation received from Discovery! Closing session..\n");
        }

    }
    
    printStringWithHeader("^\nFreeing memory...", " ");
    printStringWithHeader(".", " ");

    //MONOLITH
  
    close(pipefd[1]);

    free(poole.name);
    free(poole.ipDiscovery);
    free(poole.folder);
    free(poole.ipPoole);

    close(pooleFd);
    close(discoverySocketFd);

    FD_ZERO(&setOfSockFd);
    for (int i = 0; i < numberOfSockets; i++) {
        close(sockets[i]);
    }
    free(sockets);

    freeSeparatedData(&separatedData, &numberOfData);

    free(frame.data);
    free(frame.header);

    printStringWithHeader(" .", " ");
    printStringWithHeader("  .", " ");
    printStringWithHeader("   .", " ");
    printStringWithHeader("    .", " ");
    printStringWithHeader("     ...All memory freed...", "\n\nReady to EXIT this POOLE Process.");

    exit(EXIT_SUCCESS);
}

void socketDisconnectedPoole(int socket_){
    
        printInt("\nSocket closed: ", socket_);
  
        FD_CLR(socket_, &setOfSockFd);
        close(socket_);
       
        for (int i = 0; i < numberOfSockets; i++) {
            if (sockets[i] == socket_) {
                
                for (int j = i; j < numberOfSockets - 1; j++) {
                    sockets[j] = sockets[j + 1];
                }
                numberOfSockets--;
                sockets = realloc(sockets, sizeof(int) * numberOfSockets);
                break;
            }
        }
}

char *createFrameBinary(uint8_t type, char *header, char *data, int dataLength, uint32_t id) {
    int lengthHeader = strlen(header);
    int totalLength = 3 + lengthHeader + 4 + 4 + dataLength; 

    if (totalLength > BINARY_FRAME_SIZE) {
        printString("\nERROR: FRAME TOO LARGE\n");
        return NULL;
    }

    char *frame = (char *)malloc(256);
    if (!frame) {
        printString("\nERROR: MEMORY ALLOCATION FAILED\n");
        return NULL;
    }

    frame[0] = type;
    frame[1] = lengthHeader & 0xFF;
    frame[2] = (lengthHeader >> 8) & 0xFF;

    memcpy(frame + 3, header, lengthHeader);
    *(uint32_t *)(frame + 3 + lengthHeader) = htonl(id);
    *(uint32_t *)(frame + 7 + lengthHeader) = htonl(dataLength); 
    memcpy(frame + 11 + lengthHeader, data, dataLength);

    int pad = 256 - totalLength;
    memset(&frame[totalLength], 0, pad);  
    

    return frame;
}


void sendFileDataBinary(int socketFd, char *fileData, int fileDataLength, int id) {
    
    char *frameToSend = createFrameBinary(0x04, "FILE_DATA", fileData, fileDataLength, id);
    if (frameToSend != NULL) {
        usleep(100);
       
        write(socketFd, frameToSend, BINARY_FRAME_SIZE); 
        free(frameToSend);
    }
}

void sendEndFileDataBinary(int socketFd, int id) {
    char *frameToSend = createFrameBinary(0x04, "END", "EMPTY", 6, id);
    if (frameToSend != NULL) {
        write(socketFd, frameToSend, BINARY_FRAME_SIZE); 
        free(frameToSend);
    }
}

void* uploadThread(void* args){
    SendThread *sendThread = (SendThread *) args;
    Frame frame2; 
    initFrame(&frame2);

    int fd = -1;
    openFile(sendThread->filePath, &fd);
    int moreBytesToread = 1;

    while (moreBytesToread == 1) {
        
        char dataBuffer[BINARY_SENDING_SIZE];
        int bytesAlreadyRead = read(fd, dataBuffer, sizeof(dataBuffer)); 
        if (bytesAlreadyRead <= 0) {
            if (bytesAlreadyRead == -1) {
                printString("\nError reading file in thread\n");
            }
            moreBytesToread = 0; 
        } else {
            sendFileDataBinary(sendThread->fd, dataBuffer, bytesAlreadyRead, sendThread->id);
        }
    }

    printString("\nSending Finished\n");
    
    sendEndFileDataBinary(sendThread->fd, sendThread->id);

    close(fd); 

    free(sendThread->filePath); 
    free(sendThread); 
    freeFrame(&frame2); 
    
    return NULL;    
}

int totalDownloads = 0;
songDownloaded * songsDownloaded = NULL;



void sendSongToBowman(int socketToSendSong, char * songName, int idSending){

    //IF THE SONG HAS ALREADY BEEN SENT IN THE CONNECTION, THERE IS NO NEED TO SEND IT AGAIN
    int alreadyDownloaded = 0;
    for(int c = 0; c < totalDownloads; c++){
        if((strcmp(songsDownloaded[c].name, songName ) == 0) && (songsDownloaded[c].socket == socketToSendSong)){
            alreadyDownloaded = 1;
        }
    }

    if(alreadyDownloaded == 1){
        return;
    }else{
        
        songsDownloaded = realloc(songsDownloaded, (totalDownloads + 1) * sizeof(songDownloaded));
        songsDownloaded[totalDownloads].name = malloc(sizeof(char) * (strlen(songName) + 1));
        
        strcpy(songsDownloaded[totalDownloads].name, strdup(songName));

        songsDownloaded[totalDownloads].socket = socketToSendSong;

        totalDownloads++;
        
    }
    //---------------------------------------------------------------------
    
    char * songPath = NULL;
    asprintf(&songPath, "database%s/%s", poole.folder, songName);  

    int fd = -1;
    openFile(songPath, &fd);
    int songSize = (int) lseek(fd, 0, SEEK_END);

    char* md5sum = malloc(33 * sizeof(char));  
    printStringWithHeader("CHECKSUM FOR FILE:", songPath);

    calculateMD5Checksum(songPath, md5sum);
    printStringWithHeader("CHECKSUM CALCULATED:", md5sum);
   


    
    char *miniBuffer;
    char *entryStats;
    asprintf(&miniBuffer, "%s&%d&%s&%d", songName, songSize, md5sum, idSending); 
    asprintf(&entryStats, "%s&%s&%d", poole.name, songName, songSize); 

    sendFileInfo(socketToSendSong, miniBuffer);
    sendStatToMonolith(pipefd[1], entryStats);
    free(miniBuffer);  
    free(md5sum);
    free(entryStats);
    

    pthread_t thread;
    SendThread *args = malloc(sizeof(SendThread)); 

    args->fd = socketToSendSong;
    args->filePath = strdup(songPath); 
    args->id = idSending;
    args->songSize = songSize;

    if (pthread_create(&thread, NULL, uploadThread, args) != 0){
        printString("\nError creating download thread\n");
        free(args->filePath);  
        free(args); 
        ctrl_C_function();
    } else {
        pthread_detach(thread);  
    }

    free(songName);

    free(songPath);  
}

void mainPooleProcess( char *argv[]){

    

    openFile(argv[1], &pooleFd);

    
    readStringFromFile(pooleFd, '\n', &poole.name);
    readStringFromFile(pooleFd, '\n', &poole.folder);
    readStringFromFile(pooleFd, '\n', &poole.ipDiscovery);
    readIntFromFile(pooleFd, '\n', &poole.portDiscovery);
    readStringFromFile(pooleFd, '\n', &poole.ipPoole);
    readIntFromFile(pooleFd, '\n', &poole.portPoole);
    

    close(pooleFd);
    //printInt("sizeof(poole.name):",sizeof(poole.name));
    //printInt("sizeof(poole.ipPoole):",sizeof(poole.ipPoole));

    printStringWithHeader("Pooleee name:", poole.name);
    printStringWithHeader("Poole folder:", poole.folder);
    printStringWithHeader("Poole ipDiscovery:", poole.ipDiscovery);
    printInt("Poole portDiscovery:", poole.portDiscovery);
    printStringWithHeader("Poole ipPoole:", poole.ipPoole);
    printInt("Poole portPoole:", poole.portPoole);
     

    discoverySocketFd = connectToServer(poole.ipDiscovery, poole.portDiscovery);
    
    printInt("discoverySocketFd:", discoverySocketFd);


    char *miniBuffer;
    asprintf(&miniBuffer, "%s&%s&%d", poole.name, poole.ipPoole, poole.portPoole);
    sendNewConnectionPooleDiscovery(discoverySocketFd, miniBuffer);
    free(miniBuffer);

    
    int result = readFrame(discoverySocketFd, &frame);
    if (result <= 0) {
        printString("\nERROR: OK not receieved\n");
        ctrl_C_function();
        
    }else if(strcmp(frame.header, "CON_OK") != 0){
        printString("\nERROR: not what we were expecting\n");
        printFrame(&frame);
        ctrl_C_function();
        
    }else{
        printFrame(&frame);
        printString("\nConfirmation received from Discovery!\n");

        thisPooleFd = createServer(poole.portPoole);

        printString("\nPOOLE SERVER CREATED, waiting for bowman connections..\n");


        FD_ZERO(&setOfSockFd);
        FD_SET(thisPooleFd, &setOfSockFd);



        

        while (1) {
        
            fd_set auxiliarSetOf = setOfSockFd;  

            select(50, &auxiliarSetOf, NULL, NULL, NULL);

            for (int i = 0; i < 50; i++) {
                if (FD_ISSET(i, &auxiliarSetOf)) {
                    if (i == thisPooleFd){

                        int newBowman = accept(i, (void *)&c_addr, &c_len);
                        if (newBowman < 0) {
                            printString("\nERROR: newBowman not connected\n");
                            ctrl_C_function();
                            exit(EXIT_FAILURE);
                        }
                        printInt("\nNew Bowman connected with socket: ", newBowman);
                        sockets = realloc(sockets, sizeof(int) * (numberOfSockets + 1));
                        printInt("\nSizeof(Socket): ", sizeof(int) * (numberOfSockets + 1));
                        printInt("\nNew size of ..sockets.. array: ", sizeof(sockets));
                        sockets[numberOfSockets] = newBowman;
                        numberOfSockets++;
                        FD_SET(newBowman, &setOfSockFd);

                    } else {
                        //WE HAVE A MESSAGE!---------------------------------------------------------------------
                        int result = readFrame(i, &frame);
                        if (result <= 0) {
                            socketDisconnectedPoole(i);
                        }else if(strcmp(frame.header, "NEW_BOWMAN") == 0){
                            printFrame(&frame);
                            numberOfData = separateData(frame.data, &separatedData, &numberOfData);
                            printStringWithHeader("New Bowman connection:", separatedData[0]); //Here we have Marcel, Robert...
                            sendOkConnectionPooleBowman(i);



                        }else if(strcmp(frame.header, "EXIT_BOWMAN") == 0){
                            printFrame(&frame);
                            numberOfData = separateData(frame.data, &separatedData, &numberOfData);
                            printStringWithHeader("This Bowman Clossing Session: ", separatedData[0]);
                            sendLogoutResponse(i);



                        }else if(strcmp(frame.header, "LIST_SONGS") == 0){
                            printFrame(&frame);
                            int numberOfSongs = 0;
                            int numberOfFrames = 0;
                            char*** songsToSend = NULL;
                            char *miniBuffer;
                            char *miniBuffer2;
                            int *numberOfSongsPerFrame;

                            


                            if (readSongsFromFolder(poole.folder, &songsToSend, &numberOfSongs, &numberOfFrames, &numberOfSongsPerFrame) == 1) {
                                printInt("numberOfFrames:", numberOfFrames);
                                //FIRST send the number of frames:
                                asprintf(&miniBuffer, "%d", numberOfFrames);
                                sendSongsResponse(i, miniBuffer);
                                free(miniBuffer);

                                for (int e = 0; e < numberOfFrames; e++) {
                                        asprintf(&miniBuffer, "%s", songsToSend[e][0]);
                                       
                                    for (int j = 1; j < numberOfSongsPerFrame[e]; j++) { 
                                        
                                        asprintf(&miniBuffer2, "%s&%s", miniBuffer, songsToSend[e][j]);
                                        free(miniBuffer);

                                        asprintf(&miniBuffer, "%s", miniBuffer2);
                                        free(miniBuffer2);
                                    }
                                   
                                    sendSongsResponse(i, miniBuffer);
                                    
                                    free(miniBuffer);
                                    //free(miniBuffer2);
                                }

                                // Free the list of songs
                                freeSongsList(&songsToSend, numberOfFrames, numberOfSongsPerFrame);
                            } else {
                                printString("Failed to read songs from folder.\n");
                            }

                        }else if(strcmp(frame.header, "LIST_PLAYLISTS") == 0){
                            printFrame(&frame);
                            char *miniBuffer;
                           
                            Playlist *finalPlaylists;
                            int finalNumPlaylists;


                            if (readPlaylistsFromFolder( poole.folder, &finalPlaylists, &finalNumPlaylists) == 1) {
                                for(int mom = 0; mom < finalNumPlaylists; mom++){
                                    printStringWithHeader("Playlist: ", finalPlaylists[mom].name);
                                    for(int l = 0; l < finalPlaylists[mom].numSongs ; l++){
                                        printStringWithHeader("\t -", finalPlaylists[mom].songs[l]);
                                    }
                                }
                                
                                //FIRST send the number of frames:
                                asprintf(&miniBuffer, "%d", finalNumPlaylists);
                                sendPlaylistsResponse(i, miniBuffer);
                                free(miniBuffer);

                                for (int e = 0; e < finalNumPlaylists; e++) {
                                    if (finalPlaylists[e].numSongs == 0) {
                                        asprintf(&miniBuffer, "%s", finalPlaylists[e].name);
                                        sendPlaylistsResponse(i, miniBuffer);

                                        free(miniBuffer);
                                    } else {
                                        asprintf(&miniBuffer, "%s&%s", finalPlaylists[e].name, finalPlaylists[e].songs[0]);
                                        for (int j = 1; j < finalPlaylists[e].numSongs; j++) {
                                            char *miniBuffer2 = NULL;
                                            asprintf(&miniBuffer2, "%s&%s", miniBuffer, finalPlaylists[e].songs[j]);

                                            free(miniBuffer);
                                            miniBuffer = miniBuffer2;
                                        }

                                        sendPlaylistsResponse(i, miniBuffer);

                                        free(miniBuffer);
                                    }
                                }

                                freePlaylists(finalPlaylists, finalNumPlaylists);

                            } else {
                                printString("Failed to read playlists from folder.\n");
                            }
                            
                        }else if(strcmp(frame.header, "DOWNLOAD_SONG") == 0){
                            printFrame(&frame);
                            numberOfData = separateData(frame.data, &separatedData, &numberOfData);
                            printStringWithHeader("DownLoading Song: ", separatedData[0]);
                            //check if the song exists:

                            char ***songs;
                            int numSongs;
                            int numFrames;
                            int* numberOfSongsPerFrame;
                            int found = 0;
                            if (readSongsFromFolder( poole.folder, &songs, &numSongs, &numFrames, &numberOfSongsPerFrame) == 1) {
                                for(int mom = 0; mom < numFrames; mom++){
                                    //printStringWithHeader("Playlist: ", finalPlaylists[mom].name);
                                    for(int l = 0; l < numberOfSongsPerFrame[mom] ; l++){
                                        if(strcmp(songs[mom][l], separatedData[0]) == 0){ 
                                            found = 1;
                                        }
                                    }
                                }
                                freeSongsList(&songs, numFrames, numberOfSongsPerFrame);
                                if(found != 1){
                                    printString("\nNo song that matches that name :(\n");
                                    
                                    sendSoungNotFound(i);


                                }else{
                                    printString("\nThe song exists!\n");
                                    int idSending = rand() % 1000;
                                    
                                    sendSongToBowman(i, strdup(separatedData[0]), idSending);

                                    


                                }


                            }


                        }else if(strcmp(frame.header, "DOWNLOAD_LIST") == 0){
                            printFrame(&frame);
                            numberOfData = separateData(frame.data, &separatedData, &numberOfData);
                            printStringWithHeader("DownLoading Playlist: ", separatedData[0]);
                            //check if the song exists:

                            Playlist *finalPlaylists;
                            int finalNumPlaylists;
                            int found = 0;
                            char *miniBuffer;





                            if (readPlaylistsFromFolder( poole.folder, &finalPlaylists, &finalNumPlaylists) == 1) {
                                for(int mom = 0; mom < finalNumPlaylists; mom++){
                                    if(strcmp(finalPlaylists[mom].name, separatedData[0]) == 0){ 
                                        found = 1;
                                        printString("\nThe playlists exists!\n");
                                        asprintf(&miniBuffer, "%d& ", finalPlaylists[mom].numSongs);
                                        sendPlayListFound(i, miniBuffer );
                                        free(miniBuffer);

                                        
                                        for(int l = 0; l < finalPlaylists[mom].numSongs ; l++){
                                            int idSending = rand() % 1000;
                                            printStringWithHeader("\tDownloading -", finalPlaylists[mom].songs[l]);
                                            sendSongToBowman(i, strdup(finalPlaylists[mom].songs[l]), idSending);
                                        }
                                    }
                                    
                                }
                                freePlaylists(finalPlaylists, finalNumPlaylists);
                                if(found != 1){
                                    printStringWithHeader("\nNo playlist that matches that name :( :", separatedData[0]);
                                    sendPlayListNotFound(i);


                                }


                            }


                        }
                    }
                }
            }
        }
    }


}

//-----------------------------------------------------------------

int parseLine(const char *line, char *songName, int *count) {
    const char *ptr = strchr(line, ':');
    if (!ptr) return 0; // No colon found

    strncpy(songName, line, ptr - line);
    songName[ptr - line] = '\0'; // Null-terminate the song name
    *count = atoi(ptr + 2); // Skip the colon and space, then convert to integer
    return 1;
}



int main(int argc, char *argv[]) {



    //AUXILIAR VARIABLES
    initFrame(&frame);
    numberOfSockets = 0;
    //SIGNAL ctrl C

    //SEMPAHORE
    key_t key = 1234;  
    SEM_constructor_with_name(&sem, key);
    SEM_init(&sem, 1);  
    



    checkArgc(argc);


    

    //MONOLITHIC---------------------------------------------------------------------------------------------------------------------------------
    
    if (pipe(pipefd) == -1) {
        perror("pipe");
        ctrl_C_function();
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        ctrl_C_function();
    }

    if (pid == 0) {  // Child process
        signal(SIGINT, ctrl_C_function_monolith);
        close(pipefd[1]); // Close unused write end
        Frame newFrame;
        initFrame(&newFrame);

        while (readFrame(pipefd[0], &newFrame) > 0) {
            if (strcmp(newFrame.header, "STAT") == 0) {
                int numberOfData;
                char** separatedData = NULL;
                numberOfData = separateData(newFrame.data, &separatedData, &numberOfData);

                SEM_wait(&sem);

                char* songName = separatedData[1];
                int fd = open("stats.txt", O_RDWR | O_CREAT, 0644);
                if (fd == -1) {
                    perror("open");
                    SEM_signal(&sem);
                    continue;
                }

                char fileContent[4096] = {0};
                read(fd, fileContent, sizeof(fileContent) - 1);
                char newContent[4096] = {0};
                int found = 0;

                char *line = strtok(fileContent, "\n");
                char lineSong[255];
                int downloadCount;

                while (line) {
                    if (parseLine(line, lineSong, &downloadCount)) {
                        if (strcmp(lineSong, songName) == 0) {
                            downloadCount++;
                            found = 1;
                        }
                        char *buffer;
                        asprintf(&buffer, "%s: %d\n", lineSong, downloadCount);
                        strcat(newContent, buffer);
                    }
                    line = strtok(NULL, "\n");
                }

                if (!found) {
                    char buffer[255];
                    sprintf(buffer, "%s: 1\n", songName);
                    strcat(newContent, buffer);
                }

                // Rewrite the file with updated content
                ftruncate(fd, 0);
                lseek(fd, 0, SEEK_SET);
                write(fd, newContent, strlen(newContent));

                close(fd);

                SEM_signal(&sem);

                freeSeparatedData(&separatedData, &numberOfData);
            } else {
                ctrl_C_function_monolith();
            }
        }
    }else {  // Parent process: Poole
        close(pipefd[0]); // Close unused read end

        signal(SIGINT, ctrl_C_function);
        mainPooleProcess(argv);

       
    }

   


    
   
    return 0;
}
