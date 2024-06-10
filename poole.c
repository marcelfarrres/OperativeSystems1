#include "common.h"
#include "structures.h"

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

struct sockaddr_in c_addr;
socklen_t c_len = sizeof (c_addr);


//SIGNALS PHASE-----------------------------------------------------------------
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

///FUNCION A COPIAR
void* downloadThread(void* args){
    SendThread *sendThread = (SendThread *) args;
    //char *file_path = arguments->file_path;
    //int id = arguments->id;

    int fd = -1;
    openFile(sendThread->filePath, &fd);


    int bytesAlreadyRead;
    int id_net = htonl(id);
    char ampersand = '&';
    //char dataBuffer[256 - (3 + strlen("FILE_DATA") + sizeof(id_net) + 1 ) ]; 
    char dataBuffer[200]; 
    int moreBytesToread = 1;

    while (moreBytesToread == 1) {
        if((bytesAlreadyRead = (int) read(fd, dataBuffer, sizeof(dataBuffer))) <= 0){
            moreBytesToread = 0;
        }
        

        
        if (bytesAlreadyRead == -1){
            printString("\nError reading file in thread\n");
            moreBytesToread = 0;
        }

        sendFileData(sendThread->fd, dataBuffer);

        
    }

    readFrame(sendThread->fd, &frame);

    numberOfData = separateData(frame.data, &separatedData, &numberOfData);
    
    if (strcmp(frame.header, "CHECK_OK") == 0){
        write(1, "File sent successfully\n\n", 24);
    } else {
        write(1, "Error sending file\n\n", 20);
    }

    close(fd);
    free(sendThread);
    
    return NULL;    
}


void sendSongToBowman(int socketToSendSong){

    char * songPath = NULL;                         
    asprintf(&songPath, "%s/%s", poole.folder, separatedData[0]);

    int fd = -1;
    openFile( songPath, &fd);

    //ssize_t file_size = lseek(fd, 0, SEEK_END);
    int songSize = (int) lseek(fd, 0, SEEK_END);

    //printf("File Size in ssize_t: %ld\n", file_size);
    printf("File Size in int: %d\n", songSize);

    char* md5sum = calloc(33, sizeof(char));  

    int idSending = rand() % 1000;
    
    /* falta

    int md5 = calculate_md5sum(desired_path, md5sum);
    if (md5 == -1){
        perror("Error calculating md5sum");
        
    }
    */
    char *miniBuffer;
    asprintf(&miniBuffer, "%s&%d&%s&%d", separatedData[0], songSize, md5sum, idSending);
    sendFileInfo(socketToSendSong, miniBuffer);
    free(miniBuffer);

    pthread_t thread;

    SendThread *args = malloc(sizeof(SendThread));

    args->sockfd = socketToSendSong;
    asprintf(&(args->file_path), "%s", songPath);
    args->id = idSending;

    if (pthread_create(&thread, NULL, downloadThread, args) != 0){
        printString("\nError creating download thread\n");
        ctrl_C_function();
    }

    pthread_detach(thread);

    

}
//-----------------------------------------------------------------

int main(int argc, char *argv[]) {
    //AUXILIAR VARIABLES
    initFrame(&frame);
    numberOfSockets = 0;
    //SIGNAL ctrl C
    signal(SIGINT, ctrl_C_function);

    checkArgc(argc);

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

                                }else{
                                    printString("\nThe song exists!\n");
                                    
                                    sendSongToBowman(i);

                                    


                                }


                            }


                        }
                    }
                }
            }
        }
    }




   
    return 0;
}
