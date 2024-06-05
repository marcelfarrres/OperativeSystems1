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


                            if (readSongsFromFolder(poole.folder, &songsToSend, &numberOfSongs, &numberOfFrames) == 1) {
                                printInt("numberOfFrames:", numberOfFrames);
                                //FIRST send the number of frames:
                                asprintf(&miniBuffer, "%d", numberOfFrames);
                                sendSongsResponse(i, miniBuffer);
                                free(miniBuffer);

                                for (int e = 0; e < numberOfFrames; e++) {
                                        asprintf(&miniBuffer, "%s", songsToSend[e][0]);
                                        
                                    for (int j = 1; songsToSend[e][j] != NULL; j++) {   
                                        asprintf(&miniBuffer2, "%s&%s", miniBuffer, songsToSend[e][j]);
                                        asprintf(&miniBuffer, "%s", miniBuffer2);
                                    }
                                    sendSongsResponse(i, miniBuffer);
                                    
                                    free(miniBuffer);
                                    free(miniBuffer2);
                                }

                                // Free the list of songs
                                freeSongsList(&songsToSend, numberOfFrames);
                            } else {
                                printString("Failed to read songs from folder.\n");
                            }

                        }else if(strcmp(frame.header, "LIST_PLAYLISTS") == 0){
                            printFrame(&frame);
                            char *miniBuffer;
                            char *miniBuffer2;
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
                                    if(finalPlaylists[e].numSongs == 0){
                                        asprintf(&miniBuffer, "%s", finalPlaylists[e].name);
                                        sendPlaylistsResponse(i, miniBuffer);

                                        free(miniBuffer);
                                        

                                    }else{
                                        asprintf(&miniBuffer, "%s&%s", finalPlaylists[e].name, finalPlaylists[e].songs[0]);
                                        int freebuffer2 =0;
                                        for (int j = 1; j < finalPlaylists[e].numSongs; j++) {   
                                            freebuffer2 =1;
                                            asprintf(&miniBuffer2, "%s&%s", miniBuffer, finalPlaylists[e].songs[j]);
                                            asprintf(&miniBuffer, "%s", miniBuffer2);
                                        }

                                        sendPlaylistsResponse(i, miniBuffer);
                                        
                                        free(miniBuffer);
                                        if(freebuffer2 == 1){
                                            free(miniBuffer2);
                                            
                                        }
                                    }

                                    
                                    
                                    
                                }

                                
                            } else {
                                printString("Failed to read songs from folder.\n");
                            }
                            
                        }
                    }
                }
            }
        }
    }




   
    return 0;
}
