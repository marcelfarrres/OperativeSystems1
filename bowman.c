#include "common.h"
#include "structures.h"

Bowman bowman;
int discoverySocketFd = -1;
int pooleSocketFd = -1;
char** separatedData;
int numberOfData = 0;
Frame frame;
DownloadList list;



int numberOfSongsToDownload = 0;
int finishedDownloads = 0;
int numNewFilesReceived = 0;
Download *downloads = NULL;
int fileDescriptors[100];
int NumBytesWritten[100];
int errorDownloads = 0;

int songsResponseRunning = 0;
int songsResponseRunningOnly = 0;
int numberOfFramesListPlaylists = 0;
int numberOfFramesListPlaylistsOnly = 0;
int counterOfSongs = 1;
int framesDownloaded = 0;
int framesDownloadedOnly = 0;


int connected = 0;



pthread_t thread;

int finished = 1;

int disconnectionsTracker = 0;


pthread_mutex_t download_mutex = PTHREAD_MUTEX_INITIALIZER;
DownloadElement *currentDownloads = NULL;


void removeAmp(char *buffer,  int *Amp) {
    int length = strlen(buffer); 
    

    for (int i = 0; i < length; i++) {
        if (buffer[i] == '&') {
            (*Amp)++;
        } else {
            buffer[i - (*Amp)] = buffer[i];
        }
    }

    buffer[length - (*Amp)] = '\0';
    (*Amp)++;
}


int readFrameBinary(int socketFd, Frame *frame) {
    freeFrame(frame);
    initFrame(frame);

    char buffer[256];
    int numBytes = read(socketFd, buffer, sizeof(buffer));
    if (numBytes <= 0) {
        return numBytes; 
    }

    if (numBytes < 3) { 
        return -1; 
    }

    frame->type = buffer[0];
    frame->headerLength = (buffer[2] << 8) | buffer[1];
    frame->header = (char *)malloc(frame->headerLength + 1);
    if (!frame->header) return -1; // Memory allocation failure
    memcpy(frame->header, buffer + 3, frame->headerLength);
    frame->header[frame->headerLength] = '\0';

    
    if (strcmp(frame->header, "FILE_DATA") == 0 || strcmp(frame->header, "END") == 0) {
        if (numBytes < 3 + frame->headerLength + 4 + 4) { 
            freeFrame(frame);
            printString("\nError code for insufficient data\n");
            return -1; 
        }
        frame->id = ntohl(*(uint32_t *)(buffer + 3 + frame->headerLength));
        frame->dataLength = ntohl(*(uint32_t *)(buffer + 7 + frame->headerLength));


        int dataStart = 3 + frame->headerLength + 4 + 4;
        int dataLength = numBytes - dataStart;
        frame->data = (char *)malloc(dataLength);
        if (!frame->data) {
            freeFrame(frame);
            return -1; 
        }
        memcpy(frame->data, buffer + dataStart, dataLength);
        return dataLength;
    } else {
        int dataStart = 3 + frame->headerLength;
        int dataLength = numBytes - dataStart;
        frame->data = (char *)malloc(dataLength);
        if (!frame->data) {
            freeFrame(frame);
            return -1; 
        }
        memcpy(frame->data, buffer + dataStart, dataLength);
        frame->data[dataLength - 1] = '\0'; 
        return dataLength;
    }
}


//DOWNLOADS-----------------------------------------------------------------------------------

void initDownloadList(DownloadList *list) {
    list->elements = NULL;
    list->size = 0;
    list->capacity = 0;
}

void freeDownloadList(DownloadList *list) {
    pthread_mutex_lock(&download_mutex);
    for (int i = 0; i < list->size; i++) {
        free(list->elements[i].name);
    }
    free(list->elements);
    list->elements = NULL;
    list->size = 0;
    list->capacity = 0;
    pthread_mutex_unlock(&download_mutex);
}

void clearDownloads(DownloadList *list) {
    pthread_mutex_lock(&download_mutex);  

    int j = 0;  
    for (int i = 0; i < list->size; i++) {
        if ((list->elements)[i].active == 0) {
            free((list->elements)[i].name);  
        } else {
            if (j != i) {
               
                (list->elements)[j] = (list->elements)[i];
            }
            j++; 
        }
    }

    
    list->size = j;

    pthread_mutex_unlock(&download_mutex);  
}



void updateDownloadSize(DownloadList *list, const char *name, int size) {
    //printString("\nWE GOT IN! updateDownloadSize\n");
    
    pthread_mutex_lock(&download_mutex);
    for (int i = 0; i < list->size; i++) {
        if (strcmp((list->elements)[i].name, name) == 0) {
            (list->elements)[i].currentFileSize = size;
            
        }
    }
    pthread_mutex_unlock(&download_mutex);
    //printString("\nWE GOT OUT! updateDownloadSize\n");
}

void deactivateDownload(DownloadList *list, const char *name) {
    //printString("\nWE GOT IN! updateDownloadSize\n");
    pthread_mutex_lock(&download_mutex);
    for (int i = 0; i < list->size; i++) {
        if (strcmp((list->elements)[i].name, name) == 0) {
            (list->elements)[i].active = 0;
            
        }
    }
    pthread_mutex_unlock(&download_mutex);
    //printString("\nWE GOT OUT! updateDownloadSize\n");

}




void freeDownloadArray( int numDownloads) {
    if (downloads == NULL) {
        return; // If the passed array is NULL, there is nothing to do.
    }
    //printInt("numDownloads:", numDownloads);
    // Iterate over each Download struct in the array
    for (int i = 0; i < numDownloads; i++) {
        //printStringWithHeader("downloads[i].name:", downloads[i].name);
        //printStringWithHeader("downloads[i].pathOfTheFile:", downloads[i].pathOfTheFile);
        //printStringWithHeader("downloads[i].md5:", downloads[i].md5);


        free(downloads[i].name);         // Free the name string, if allocated
        free(downloads[i].pathOfTheFile); // Free the path of the file string, if allocated
        free(downloads[i].md5);           // Free the md5 string, if allocated

        // Set pointers to NULL to avoid use-after-free errors
        downloads[i].name = NULL;
        downloads[i].pathOfTheFile = NULL;
        downloads[i].md5 = NULL;
    }

    // Finally, free the array itself
    free(downloads);
}

void printDownloadProgress(const DownloadList *list) {
    if (list == NULL || list->elements == NULL) {
        printf("No downloads to display.\n");
        return;
    }

    printf("Download Progress:\n");
    for (int i = 0; i < list->size; i++) {
        
        float progress = (float)list->elements[i].currentFileSize / list->elements[i].maxSize;
        int percent = (int)(progress * 100);
        int barWidth = 50; // Width of the progress bar in characters
        printString(list->elements[i].name);
        printString(": [");
        int pos = (int)(barWidth * progress);
        for (int j = 0; j < barWidth; ++j) {
            if (j < pos) printString("=");
            else if (j == pos) printString(">");
            else printString(" ");
        }
        printString("] ");
        printOnlyInt( percent);
        printString("% \n");

        
    }
}


void addDownload( DownloadList *list, Download *args) {
    pthread_mutex_lock(&download_mutex);
    if (list->size == list->capacity) {
        int new_capacity = list->capacity == 0 ? 1 : list->capacity * 2;
        list->elements = realloc(list->elements, new_capacity * sizeof(DownloadElement));
        list->capacity = new_capacity;
    }
    DownloadElement *new_element = &list->elements[list->size++];
    new_element->name = strdup(args->name);
    new_element->maxSize = args->maxSize;
    new_element->currentFileSize = 0;
    new_element->active = 1;
    pthread_mutex_unlock(&download_mutex);
}



//SIGNALS PHASE-----------------------------------------------------------------
void ctrl_C_function(){
    if(connected == 0){
        exit(EXIT_SUCCESS);
    }else{
        sendLogoutBowman(pooleSocketFd, bowman.name);
    }
    
}





void *downloadPlaylistThread(void *arg);

void manageLogIn(){
    discoverySocketFd = connectToServer(bowman.ipDiscovery, bowman.portDiscovery);
    printInt("discoverySocketFd:", discoverySocketFd);

    sendNewConnectionBowmanDiscovery(discoverySocketFd, bowman.name);
    
    int result = readFrame(discoverySocketFd, &frame);
    if (result <= 0) {
        printString("\nERROR: OK not receieved\n");
        ctrl_C_function();
        
    }else if(strcmp(frame.header, "CON_KO") == 0){
        printString("\nERROR: There were no Poole servers connected.\n");
        //printFrame(&frame);
        ctrl_C_function();

    }else if(strcmp(frame.header, "CON_OK") != 0){
        printString("\nERROR: not what we were expecting\n");
        //printFrame(&frame);
        ctrl_C_function();
        
    }else{
        //printFrame(&frame);
        printString("\nConfirmation received from Discovery!\n");

        numberOfData = separateData(frame.data, &separatedData, &numberOfData);
        
    }
    char * auxIp = strdup(separatedData[1]);
    pooleSocketFd = connectToServer(auxIp, atoi(separatedData[2]));
    freeSeparatedData(&separatedData,&numberOfData);
    free(auxIp);
    sendNewConnectionBowmanPoole(pooleSocketFd, bowman.name);
    result = readFrame(pooleSocketFd, &frame);
    if (result <= 0) {
        printString("\nERROR: OK not receieved\n");
        ctrl_C_function();
        
    }else if(strcmp(frame.header, "CON_KO") == 0){
        printString("\nERROR:Poole KO CONNECTION.\n");
        //printFrame(&frame);
        ctrl_C_function();

    }else if(strcmp(frame.header, "CON_OK") != 0){
        printString("\nERROR: not what we were expecting\n");
        //printFrame(&frame);
        ctrl_C_function();
    }else{
        //printFrame(&frame);
        printString("\nConfirmation received from Poole!\n");

        disconnectionsTracker = 0;

        //------------------------------------------------READING FRAMES THREAD

        

        if (pthread_create(&thread, NULL, downloadPlaylistThread, NULL) != 0) {
            perror("Failed to create download thread");
            
            ctrl_C_function();
        }else{
            printString("Thread Created!");
        }

        pthread_detach(thread);

    //------------------------------------------------
    }
}

void manageListSongs(){
    sendListSongs(pooleSocketFd);
   
}

void manageListPlaylists(){
    sendListPlaylists(pooleSocketFd);
   
}


//THREAD for background downloading--------------------------------------------------------------------------------
void *downloadPlaylistThread(void *arg) {
    (void)arg;

    downloads = NULL;
   

    finishedDownloads = 0;
    numNewFilesReceived = 0;
    errorDownloads = 0;
   


    char** separatedData;
    int numberOfData = 0;
    Frame frameD;
    initFrame(&frameD);
 
    
    
    
    while(finished){
        
            readFrameBinary(pooleSocketFd, &frameD);
            
            //printFrame(&frameD);
            

            if(strcmp(frameD.header, "NEW_FILE") == 0){
                char *miniBuffer;
                numberOfData = separateData(frameD.data, &separatedData, &numberOfData);
                
                asprintf(&miniBuffer, "clients%s/%s", bowman.folder, separatedData[0]);
                printStringWithHeader("\nNEW Filenameeee:", miniBuffer);


                downloads[numNewFilesReceived].pathOfTheFile = strdup(miniBuffer);
                downloads[numNewFilesReceived].fdAttached = pooleSocketFd;
                downloads[numNewFilesReceived].maxSize =  atoi(separatedData[1]);
                downloads[numNewFilesReceived].id = atoi(separatedData[3]);
                downloads[numNewFilesReceived].md5 = strdup(separatedData[2]);
                downloads[numNewFilesReceived].name = strdup(separatedData[0]);

                

                fileDescriptors[numNewFilesReceived] = open( miniBuffer, O_WRONLY | O_TRUNC | O_CREAT, 0666);
                if (fileDescriptors[numNewFilesReceived] == -1) {
                    perror("Error opening file");
                    printString(" ??YEP ");
                    ctrl_C_function();
                }

                NumBytesWritten[numNewFilesReceived] = 0;
                addDownload(&list, &(downloads[numNewFilesReceived]));
                free(miniBuffer);
                numNewFilesReceived++;

            }else if(strcmp(frameD.header, "FILE_DATA") == 0){
                for(int er = 0; er < numNewFilesReceived; er++){
                    if((int)frameD.id == downloads[er].id){
                        if (NumBytesWritten[er] < downloads[er].maxSize) {
                            if (frameD.data != NULL) {
                                //printInt("datalength:", (int)frameD.dataLength);
                                write(fileDescriptors[er], frameD.data, (int)frameD.dataLength);
                                NumBytesWritten[er] += BINARY_SENDING_SIZE;
                            }
                            updateDownloadSize(&list, downloads[er].name, NumBytesWritten[er]);
                        }
                    }
                }
            }else if(strcmp(frameD.header, "END") == 0){
                for(int er = 0; er < numNewFilesReceived; er++){
                    if((int)frameD.id == downloads[er].id){

                        char* md5sum = malloc(33 * sizeof(char));  
                        //printStringWithHeader("CHECKSUM FOR FILE:", downloads[er].pathOfTheFile);

                        calculateMD5Checksum(downloads[er].pathOfTheFile, md5sum);
                        //-----
                        printStringWithHeader("\n[Server] Md5Checksum:", downloads[er].md5);
                        printStringWithHeader("[Client] Md5Checksum:", md5sum);

                        if (strcmp(md5sum, downloads[er].md5) == 0) {
                            printStringWithHeader("\nMD5 Checkum VERIFIED, you can now play this song on your:", downloads[er].name );
                        } else {
                            printStringWithHeader("\nMD5 Checkum FAILED, This song may be corrupted:", downloads[er].name );
                        }

                        printString("\n\n$ ");
                        
                        //-----
                        free(md5sum);

                        deactivateDownload(&list, downloads[er].name);
                        finishedDownloads++;
                        close(fileDescriptors[er]);
                    }
                }
                
            }else if(strcmp(frameD.header, "NOT_FOUND") == 0){
               
                finishedDownloads++;
                errorDownloads++;
                
                printString("\nERROR: File doesn't exists in the server\n");
                
            }else if(strcmp(frameD.header, "SONGS_RESPONSE_PLAYLISTS") == 0 && songsResponseRunning == 0){
                numberOfData = separateData(frameD.data, &separatedData, &numberOfData);
                //printInt("atoi(separatedData[0]):", atoi(separatedData[0]));
                numberOfFramesListPlaylists = atoi(separatedData[0]);
                
                songsResponseRunning = 1;
                framesDownloaded = 0;
                
            }else if(strcmp(frameD.header, "SONGS_RESPONSE_PLAYLISTS") == 0 && songsResponseRunning == 1){
                
              
                numberOfData = separateData(frameD.data, &separatedData, &numberOfData);
                printOnlyInt(framesDownloaded + 1);
                printStringWithHeader(".", separatedData[0]);
                for(int mi = 1; mi < numberOfData; mi++){
                    printStringWithHeader("\t-", separatedData[mi]);
                }
                framesDownloaded++;
                
                if(numberOfFramesListPlaylists <= framesDownloaded){
                    songsResponseRunning = 0;
                    numberOfFramesListPlaylists = 0;
                    printString("\nALL PLAYLISTS PRINTED!");
                    printString("\n$ ");

                }

                
            }else if (strcmp(frameD.header, "PLAYLIST_FOUND") == 0){
               

                numberOfData = separateData(frameD.data, &separatedData, &numberOfData);

                numberOfSongsToDownload = numberOfSongsToDownload + atoi(separatedData[0]);

                printInt("numberOfSongsToDownload:", numberOfSongsToDownload);

                downloads = realloc(downloads, numberOfSongsToDownload * sizeof(Download));
   
        
            }else if(strcmp(frameD.header, "CONOK") == 0){
                finished = 0;
                
               
                    sendRemoveConnectionBowman(discoverySocketFd, bowman.name);
                    int result = readFrame(discoverySocketFd, &frame);
                    if (result <= 0) {
                        printString("\nERROR: OK not receieved\n");


                    }else if(strcmp(frame.header, "CONKO") == 0){
                        printString("\nERROR:Discovery KO CONNECTION.\n");
                        //printFrame(&frame);


                    }else if(strcmp(frame.header, "CONOK") != 0){
                        printString("\nERROR: not what we were expecting\n");
                        //printFrame(&frame);

                    }else{
                        //printFrame(&frame);
                        printString("\nConfirmation received from Discovery! Closing session..\n");
                    }
                    close(pooleSocketFd);
                    printString("\nLogging out...\n");
                    close(pooleSocketFd);
                    close(discoverySocketFd);
                    printStringWithHeader("^\nFreeing memory..."," ");


                    free(bowman.name);
                    free(bowman.ipDiscovery);
                    free(bowman.folder);

                    free(frame.data);
                    free(frame.header);
                    freeSeparatedData(&separatedData, &numberOfData);

                    freeDownloadList(&list);



                    printStringWithHeader("     ...All memory freed...","\n\nReady to EXIT this BOWMAN Process.");

                    freeSeparatedData(&separatedData, &numberOfData);

                    freeFrame(&frameD);

                    freeDownloadArray(finishedDownloads - errorDownloads );

                    


                    exit(EXIT_SUCCESS);

                
            }else if(strcmp(frameD.header, "SONGS_RESPONSE_SONGS") == 0 && songsResponseRunningOnly == 0){
                numberOfData = separateData(frameD.data, &separatedData, &numberOfData);
                counterOfSongs = 1;
                numberOfFramesListPlaylistsOnly = atoi(separatedData[0]);
                
                songsResponseRunningOnly = 1;
                framesDownloadedOnly = 0;
                
            }else if(strcmp(frameD.header, "SONGS_RESPONSE_SONGS") == 0 && songsResponseRunningOnly == 1){
                
              
                numberOfData = separateData(frameD.data, &separatedData, &numberOfData);
                
              
                numberOfData = separateData(frameD.data, &separatedData, &numberOfData);
                for(int mi = 0; mi < numberOfData; mi++){
                    printString(" ");
                    printOnlyInt(counterOfSongs);
                    printStringWithHeader(".", separatedData[mi]);
                    counterOfSongs++;
                }
                framesDownloadedOnly++;

                
                if(numberOfFramesListPlaylistsOnly <= framesDownloadedOnly){
                    songsResponseRunningOnly = 0;
                    numberOfFramesListPlaylistsOnly = 0;
                    
                    printString("\n$ ");

                }

                
            }

        
    }



    
    //free
    freeSeparatedData(&separatedData, &numberOfData);

    freeFrame(&frameD);

    freeDownloadArray(finishedDownloads - errorDownloads );
  
    numberOfSongsToDownload = 0;
    printString("\nWE freed this memory");

    return NULL;

    
}


void manageDownloadPlaylist(char ** input, int wordCount){
    

    char * playlistName = concatenateWords2(input, wordCount);

    sendDownloadPlaylist(pooleSocketFd,  playlistName);
     
    
}

void manageDownload(char ** input, int wordCount){

    char * song = concatenateWords(input, wordCount);
    //char * song2 = "example.txt";
    
    sendDownloadSong(pooleSocketFd,  song);

    numberOfSongsToDownload = numberOfSongsToDownload + 1;

    downloads = realloc(downloads, numberOfSongsToDownload * sizeof(Download));

    free(song);
  
    
    
}

void manageCheckDownloads( DownloadList *list){
   
    printDownloadProgress(list);

}

void menu() {
    char buffer[200];
    
    int inputLength;
    int numberOfWords = 0;
    int numberOfSpaces = 0;


    //create the Clients/[name] folder
    initDownloadList(&list);
    char *subFolder;
    asprintf(&subFolder, "clients%s", bowman.folder);
    mkdir(subFolder, 0755);
    free(subFolder);

    

    
    
    

    while (finished) {
        //reset for each iteration
        numberOfWords = 0;
        numberOfSpaces = 0;
        
        if(connected == 0){
            
            printString("\n[not connected] $ ");
        }else{
 

           printString(
            "\n┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n"
              "┃ Commands Shortcuts:   \n"
              "┃                          \n"
              "┃  1) CONNECT            5) DOWNLOAD   \n"
              "┃  2) LOGOUT             6 6) DOWNLOAD PLAYLIST (double 6) \n"
              "┃  3) LIST SONGS         7) CHECK DOWNLOADS   \n"
              "┃  4) LIST PLAYLISTS     8) CLEAR DOWNLOADS   \n"
              "┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n"
            );

            printString("\n$ ");

        }
        readFromConsole(&inputLength, &buffer);
        
        for (int j = 0; j < inputLength; j++) {
            if (buffer[j] == ' ') {
                if(buffer[j - 1] != ' '){
                    numberOfSpaces++;
                    
                }
                if(buffer[j + 1] == '\0'){
                        buffer[j] = '\0'; 
                    }
            }
        }
      
        char *input[numberOfSpaces + 1];
        char *token = strtok(buffer, " ");
        
        while (numberOfWords <= numberOfSpaces && token != NULL) {
            input[numberOfWords] = token;
            token = strtok(NULL, " ");
            numberOfWords++;
        }

        if ((numberOfWords == 1 && strcasecmp(input[0], "CONNECT") == 0) || (numberOfWords == 1 && strcasecmp(input[0], "1") == 0)) {
            if (connected) {
                printString("Floyd is already connected.\n");
            } else {
                connected = 1;
                manageLogIn();
                printString("Floyd connected to HAL 9000 system, welcome music lover!\n");
            }
        } else if ((numberOfWords == 1 && strcasecmp(input[0], "LOGOUT") == 0) || (numberOfWords == 1 && strcasecmp(input[0], "2") == 0)) {
            if (connected) {
                printString("Thanks for using HAL 9000, see you soon, music lover!\n");
                connected = 0;
                ctrl_C_function();
            } else {
                printString("Cannot Logout, you are not connected to HAL 9000\n");
            }
        } else if ((numberOfWords == 2 && strcasecmp(input[0], "LIST") == 0 && strcasecmp(input[1], "SONGS") == 0) || (numberOfWords == 1 && strcasecmp(input[0], "3") == 0)) {
            if (connected) {
                manageListSongs();
            } else {
                printString("Cannot List Songs, you are not connected to HAL 9000\n");
            }
        } else if ((numberOfWords == 2 && strcasecmp(input[0], "LIST") == 0 && strcasecmp(input[1], "PLAYLISTS") == 0) || (numberOfWords == 1 && strcasecmp(input[0], "4") == 0)) {
            if (connected) {
                manageListPlaylists();
            } else {
                printString("Cannot List Playlist, you are not connected to HAL 9000\n");
            }
        } else if ((numberOfWords > 2 && strcasecmp(input[0], "DOWNLOAD") == 0 && strcasecmp(input[1], "PLAYLIST") == 0) || (numberOfWords > 2 && strcasecmp(input[0], "6") == 0 && strcasecmp(input[1], "6") == 0)) {
            if (connected) {
                manageDownloadPlaylist(input, numberOfWords);
            } else {
                printString("Cannot download playlist, you are not connected to HAL 9000\n");
            }
        } else if ((numberOfWords == 2 && strcasecmp(input[0], "CHECK") == 0 && strcasecmp(input[1], "DOWNLOADS") == 0) || (numberOfWords == 1 && strcasecmp(input[0], "7") == 0)) {
            if (connected) {
                manageCheckDownloads(&list);
            } else {
                printString("Cannot Check Downloads, you are not connected to HAL 9000\n");
            }
        }else if ((numberOfWords > 1 && strcasecmp(input[0], "DOWNLOAD") == 0) || (numberOfWords > 1 && strcasecmp(input[0], "5") == 0)) {
            if (connected) {
                manageDownload(input, numberOfWords);
            } else {
                printString("Cannot download, you are not connected to HAL 9000\n");
            }
        }else if ((numberOfWords == 2 && strcasecmp(input[0], "CLEAR") == 0 && strcasecmp(input[1], "DOWNLOADS") == 0) || (numberOfWords == 1 && strcasecmp(input[0], "8") == 0 )) {
            if (connected) {
                clearDownloads(&list);
            } else {
                printString("Cannot clear, you are not connected to HAL 9000\n");
            }
        } else {
            printString("ERROR: Please input a valid command.\n");
        }
    }
    //ctrl_C_function();
}

//---------------------------------------------------------------------------------------


int main(int argc, char *argv[]){
    //AUXILIAR VARIABLES
    char *buffer; //Buffer
    int Amp = 0; //Number of &
    int bowFd = -1; //BOWMAN FILE
    initFrame(&frame);

   
    //SIGNAL ctrl C
    signal(SIGINT, ctrl_C_function);

    checkArgc(argc);

    openFile(argv[1], &bowFd);

    buffer = read_until(bowFd, '\n');
    size_t length = strlen(buffer);
    
    removeAmp(buffer, &Amp);

    bowman.name =(char *) calloc((length - Amp + 2 ), sizeof(char) );

    strcpy(bowman.name, buffer);
    bowman.name[length - Amp] = '\0';
    free(buffer);

    readStringFromFile(bowFd, '\n', &bowman.folder);
    readStringFromFile(bowFd, '\n', &bowman.ipDiscovery);
    readIntFromFile(bowFd, '\n', &bowman.portDiscovery);


    close(bowFd);
    

    printStringWithHeader("Bowman name:",bowman.name);
    printStringWithHeader("Bowman folder:",bowman.folder);
    printStringWithHeader("Bowman ipDiscovery:",bowman.ipDiscovery);
    printInt("Bowman portDiscovery:",bowman.portDiscovery);

    sleep(1);

    
    menu();
    
    return 0;
}