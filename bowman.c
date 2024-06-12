#include "common.h"
#include "structures.h"





typedef struct DownloadElement {
    pthread_t thread_id;
    char *name;
    int maxSize;
    int currentFileSize;
    int active;
} DownloadElement;

typedef struct {
    DownloadElement *elements;
    int size;
    int capacity;
} DownloadList;

typedef struct {
    char *name;
    int fdAttached;
    int maxSize; 
    char *pathOfTheFile;
    char *md5;
    int id;
    DownloadList * list;
} FileArgs;

typedef struct {
    char *name;
    int fdAttached;
    int maxSize; 
    char *pathOfTheFile;
    char *md5;
    int id;
} Download;

typedef struct {
    int numberOfSongsToDownload;
    int finishedDownloads;
} ArgsForDownloadPlaylist;


Bowman bowman;
int discoverySocketFd = -1;
int pooleSocketFd = -1;
char** separatedData;
int numberOfData = 0;
Frame frame;
DownloadList list;








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
    int numBytes = read(socketFd, buffer, 256);
    if (numBytes <= 0) {
        return numBytes;
    }

    frame->type = buffer[0];
    frame->headerLength = (buffer[2] << 8) | buffer[1];
    frame->header = (char *)malloc(((frame->headerLength) + 1) * sizeof(char));
    memcpy(frame->header, buffer + 3, frame->headerLength);
	(frame->header)[((frame->headerLength))] = '\0';

    if(strcmp(frame->header,"FILE_DATA") == 0 || strcmp(frame->header,"END") == 0){
        // Read the integer ID
        if (numBytes >= 3 + frame->headerLength + 4) { // Ensure there are enough bytes for the ID
            frame->id = ntohl(*(uint32_t *)(buffer + 3 + frame->headerLength)); // Convert from network byte order
        } else {
            return -1;  // Not enough data received
        }
        // Read the data
        int dataStart = 3 + frame->headerLength + 4;
        int dataLength = numBytes - dataStart;
        frame->data = (char *)malloc(dataLength);
        memcpy(frame->data, buffer + dataStart, dataLength);

        return BINARY_SENDING_SIZE;

    }else{

        int dataLength = numBytes - (3 + frame->headerLength); 
   
        frame->data = (char *)malloc(dataLength * sizeof(char));
        memcpy(frame->data, buffer + 3 + frame->headerLength, dataLength);
        (frame->data)[dataLength - 1] = '\0'; 

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


void freeDownloadArgs(FileArgs *args) {
    if (args != NULL) {
        if (args->pathOfTheFile != NULL) {
            free(args->pathOfTheFile);
            args->pathOfTheFile = NULL;  
        }
        if (args->md5 != NULL) {
            free(args->md5);
            args->md5 = NULL;
        }
        if (args->name != NULL) {
            free(args->name);
            args->name = NULL;
        }

        free(args);
    }
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



//SIGNALS PHASE-----------------------------------------------------------------
void ctrl_C_function(){
    printString("Closing Session..\n");
    if(pooleSocketFd > 0){
        sendLogoutBowman(pooleSocketFd, bowman.name);
        int result = readFrame(pooleSocketFd, &frame);
        if (result <= 0) {
            printString("\nERROR: OK not receieved\n");
            
            
        }else if(strcmp(frame.header, "CONKO") == 0){
            printString("\nERROR:Poole KO CONNECTION.\n");
            printFrame(&frame);
            
    
        }else if(strcmp(frame.header, "CONOK") != 0){
            printString("\nERROR: not what we were expecting\n");
            printFrame(&frame);
            
        }else{
            printFrame(&frame);
            printString("\nConfirmation received from Poole! Clossing Session..\n");
        }
    }
    if(discoverySocketFd > 0){
        sendLogoutBowman(discoverySocketFd, bowman.name);
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


    exit(EXIT_SUCCESS);
}

//-----------------------------------------------------------------




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




void *downloadThread(void *arg) {
    FileArgs *fileArgs = (FileArgs *)arg;
    

    Frame frameT;
    initFrame(&frameT);
    
    int fd = open(fileArgs->pathOfTheFile, O_WRONLY | O_TRUNC | O_CREAT, 0666);
    if (fd == -1) {
        perror("Error opening file");
        ctrl_C_function();
    }
    
    int NumBytesWritten = 0;
    while (NumBytesWritten < fileArgs->maxSize) {
        
        freeFrame(&frameT);
        initFrame(&frameT);
        int dataLength = readFrameBinary(fileArgs->fdAttached, &frameT);
        if (dataLength <= 0) {
            printString("Error reading frame or no data left\n");
            break;
        }

        if (frameT.data != NULL) {
           // printf("Data pointer: %p, Data content: %02x\n", frameT.data, *frameT.data);

            write(fd, frameT.data, dataLength);
            NumBytesWritten += dataLength;
        }

        updateDownloadSize(&list, fileArgs->name, NumBytesWritten);
       // printDownloadProgress(list);
    }
    

    char *md5sum = "qwertyuiopasdfghjklzxcvbnmqwertyu"; // This would be replaced by a real MD5 checksum computation

    if (strcmp(md5sum, fileArgs->md5) != 0) {
        sendCheckResult(fileArgs->fdAttached, 0);
        //printInt("[FAILED M5]sendCheckResult->FD:", fileArgs->fdAttached );

    } else {
        sendCheckResult(fileArgs->fdAttached, 1);
        //printInt("[GOOD M5]sendCheckResult->FD:", fileArgs->fdAttached );

    }
    close(fd);

    deactivateDownload(&list, fileArgs->name);

    if (frameT.data) free(frameT.data);
    if (frameT.header) free(frameT.header);
    
    freeDownloadArgs(fileArgs);
    return NULL;
}


//-----------------------------------------------------------------

void manageLogOut(){
    printString("Closing Session..\n");
    if(pooleSocketFd > 0){
        sendLogoutBowman(pooleSocketFd, bowman.name);
        int result = readFrame(pooleSocketFd, &frame);
        if (result <= 0) {
            printString("\nERROR: OK not receieved\n");
            
            
        }else if(strcmp(frame.header, "CONKO") == 0){
            printString("\nERROR:Poole KO CONNECTION.\n");
            printFrame(&frame);
            
    
        }else if(strcmp(frame.header, "CONOK") != 0){
            printString("\nERROR: not what we were expecting\n");
            printFrame(&frame);
            
        }else{
            printFrame(&frame);
            printString("\nConfirmation received from Poole! Clossing Session..\n");
        }

    }
    if(discoverySocketFd > 0){
        sendRemoveConnectionBowman(discoverySocketFd, bowman.name);
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
    
    close(pooleSocketFd);
}

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
        printFrame(&frame);
        ctrl_C_function();

    }else if(strcmp(frame.header, "CON_OK") != 0){
        printString("\nERROR: not what we were expecting\n");
        printFrame(&frame);
        ctrl_C_function();
        
    }else{
        printFrame(&frame);
        printString("\nConfirmation received from Discovery!\n");

        numberOfData = separateData(frame.data, &separatedData, &numberOfData);
    }
    char * auxIp = strdup(separatedData[1]);
    pooleSocketFd = connectToServer(auxIp, atoi(separatedData[2]));
    free(auxIp);
    sendNewConnectionBowmanPoole(pooleSocketFd, bowman.name);
    result = readFrame(pooleSocketFd, &frame);
    if (result <= 0) {
        printString("\nERROR: OK not receieved\n");
        ctrl_C_function();
        
    }else if(strcmp(frame.header, "CON_KO") == 0){
        printString("\nERROR:Poole KO CONNECTION.\n");
        printFrame(&frame);
        ctrl_C_function();

    }else if(strcmp(frame.header, "CON_OK") != 0){
        printString("\nERROR: not what we were expecting\n");
        printFrame(&frame);
        ctrl_C_function();
    }else{
        printFrame(&frame);
        printString("\nConfirmation received from Poole!\n");
    }
}

void manageListSongs(){
    sendListSongs(pooleSocketFd);
    int result = readFrame(pooleSocketFd, &frame);
    printFrame(&frame);

    if (result <= 0) {
        printString("\nERROR: OK not receieved\n");
        ctrl_C_function();
    }else if(strcmp(frame.header, "SONGS_RESPONSE") != 0){
        printString("\nERROR: not what we were expecting\n");
        printFrame(&frame);
        ctrl_C_function();
        
    }else{
        int counterOfSongs = 1;
        numberOfData = separateData(frame.data, &separatedData, &numberOfData);
        //printInt("atoi(separatedData[0]):", atoi(separatedData[0]));
        int numberOfFrames = atoi(separatedData[0]);
        for(int re = 0; re < numberOfFrames; re++){
            result = readFrame(pooleSocketFd, &frame);
            //printFrame(&frame);
            numberOfData = separateData(frame.data, &separatedData, &numberOfData);
            for(int mi = 0; mi < numberOfData; mi++){
                printString(" ");
                printOnlyInt(counterOfSongs);
                printStringWithHeader(".", separatedData[mi]);
                counterOfSongs++;

            }
        }

        printString("\nALL SONGS PRINTED!");
    }
}

void manageListPlaylists(){
    sendListPlaylists(pooleSocketFd);
    int result = readFrame(pooleSocketFd, &frame);
    printFrame(&frame);

    if (result <= 0) {
        printString("\nERROR: OK not receieved\n");
        ctrl_C_function();
    }else if(strcmp(frame.header, "SONGS_RESPONSE") != 0){
        printString("\nERROR: not what we were expecting\n");
        printFrame(&frame);
        ctrl_C_function();
        
    }else{
        numberOfData = separateData(frame.data, &separatedData, &numberOfData);
        //printInt("atoi(separatedData[0]):", atoi(separatedData[0]));
        int numberOfFrames = atoi(separatedData[0]);
        for(int re = 0; re < numberOfFrames; re++){
            result = readFrame(pooleSocketFd, &frame);
            //printFrame(&frame);
            numberOfData = separateData(frame.data, &separatedData, &numberOfData);
            printOnlyInt(re + 1);
                
            printStringWithHeader(".", separatedData[0]);

            for(int mi = 1; mi < numberOfData; mi++){
                printStringWithHeader("\t-", separatedData[mi]);
            }
        }

        printString("\nALL PLAYLISTS PRINTED!");
    }
}

void manageDownload(char ** input, int wordCount){

    char * song = concatenateWords(input, wordCount);
    //char * song2 = "example.txt";
    

    sendDownloadSong(pooleSocketFd,  song);
    int result = readFrame(pooleSocketFd, &frame);
    printFrame(&frame);

    if (result <= 0) {
        printString("\nERROR: OK not receieved\n");
        ctrl_C_function();
    }else if(strcmp(frame.header, "NOT_FOUND") == 0){
        printString("\nERROR: File doesn't exists in the server\n");
        printFrame(&frame);
        
        
    }else if(strcmp(frame.header, "NEW_FILE") != 0){
        printString("\nERROR: not what we were expecting\n");
        printFrame(&frame);
        ctrl_C_function();
        
    }else{
        printStringWithHeader("Download started:", song);
        numberOfData = separateData(frame.data, &separatedData, &numberOfData);


        pthread_t thread;
        Download *args = malloc(sizeof(Download));

        char *miniBuffer;

        asprintf(&miniBuffer, "%s/%s", bowman.folder, song);

        printStringWithHeader("\nFilenameeee:", miniBuffer);

        args->fdAttached = pooleSocketFd;
        args->maxSize = atoi(separatedData[1]);
        args->pathOfTheFile = strdup(miniBuffer);
        args->md5 = strdup(separatedData[2]);
        args->name = strdup(separatedData[0]);
        

        addDownload(&list, args);

        if (pthread_create(&thread, NULL, downloadThread, args) != 0) {
            perror("Failed to create download thread");
            free(args);
            ctrl_C_function();
        }
        free(miniBuffer);

        pthread_detach(thread);
    }
    free(song);
}

void *downloadPlaylistThread(void *arg) {
    ArgsForDownloadPlaylist *args = (ArgsForDownloadPlaylist *)arg;

    Download *downloads = malloc(args->numberOfSongsToDownload * sizeof(Download));

    int fileDescriptors[args->numberOfSongsToDownload];
    int NumBytesWritten[args->numberOfSongsToDownload];

    char** separatedData;
    int numberOfData = 0;
    Frame frameD;
    initFrame(&frameD);
    
    int numNewFilesReceived = 0;
    
    while(args->finishedDownloads < args->numberOfSongsToDownload){
            readFrameBinary(pooleSocketFd, &frameD);
            //printFrame(&frameD);
            numberOfData = separateData(frameD.data, &separatedData, &numberOfData);

            if(strcmp(frameD.header, "NEW_FILE") == 0){
                char *miniBuffer;
                
                asprintf(&miniBuffer, "%s/%s", bowman.folder, separatedData[0]);
                printStringWithHeader("\nNEW Filenameeee:", miniBuffer);

                downloads[numNewFilesReceived].pathOfTheFile = strdup(miniBuffer);
                downloads[numNewFilesReceived].fdAttached = pooleSocketFd;
                downloads[numNewFilesReceived].maxSize =  atoi(separatedData[1]);
                downloads[numNewFilesReceived].id = atoi(separatedData[3]);
                downloads[numNewFilesReceived].md5 = strdup(separatedData[2]);
                downloads[numNewFilesReceived].name = strdup(separatedData[0]);

                fileDescriptors[numNewFilesReceived] = open( strdup(miniBuffer), O_WRONLY | O_TRUNC | O_CREAT, 0666);
                if (fileDescriptors[numNewFilesReceived] == -1) {
                    perror("Error opening file");
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
                               // printf("Data pointer: %p, Data content: %02x\n", frameT.data, *frameT.data);
                                write(fileDescriptors[er], frameD.data, BINARY_SENDING_SIZE);
                                NumBytesWritten[er] += BINARY_SENDING_SIZE;
                            }
                            updateDownloadSize(&list, downloads[er].name, NumBytesWritten[er]);
                        }
                    }
                }
            }else if(strcmp(frameD.header, "END") == 0){
                for(int er = 0; er < numNewFilesReceived; er++){
                    if((int)frameD.id == downloads[er].id){
                        deactivateDownload(&list, downloads[er].name);
                        args->finishedDownloads++;
                        close(fileDescriptors[er]);
                    }
                }
                
            }


    }

 

    return NULL;

    
}


void manageDownloadPlaylist(char ** input, int wordCount){

    char * playlistName = concatenateWords2(input, wordCount);
   
    

    sendDownloadPlaylist(pooleSocketFd,  playlistName);
    int result = readFrame(pooleSocketFd, &frame);
    printFrame(&frame);

    if (result <= 0) {
        printString("\nERROR: OK not receieved\n");
        ctrl_C_function();
    }else if(strcmp(frame.header, "PLAYLIST_NOT_FOUND") == 0){
        printString("\nERROR: File doesn't exists in the server\n");
        printFrame(&frame);
        
        
    }else if(strcmp(frame.header, "PLAYLIST_FOUND") != 0){
        printString("\nERROR: not what we were expecting\n");
        printFrame(&frame);
        ctrl_C_function();
        
    }else{
        printStringWithHeader("Download started:", playlistName);
        

        numberOfData = separateData(frame.data, &separatedData, &numberOfData);

        printInt("numberOfSongsToDownload:", atoi(separatedData[0]));




        ArgsForDownloadPlaylist * argsFor = malloc(sizeof(ArgsForDownloadPlaylist));
        
        argsFor->finishedDownloads = 0;
        argsFor->numberOfSongsToDownload = atoi(separatedData[0]);


       
        pthread_t thread;

        if (pthread_create(&thread, NULL, downloadPlaylistThread, argsFor) != 0) {
            perror("Failed to create download thread");
            
            ctrl_C_function();
        }

        pthread_detach(thread);

        
    }
    
}

void manageCheckDownloads( DownloadList *list){
   
    printDownloadProgress(list);

}

void menu() {
    char buffer[200];
    int connected = 0;
    int inputLength;
    int numberOfWords = 0;
    int numberOfSpaces = 0;

    
    initDownloadList(&list);
    
    
    

    while (1) {
        //reset for each iteration
        numberOfWords = 0;
        numberOfSpaces = 0;
        
        if(connected == 0){
            printString("\n[not connected] $ ");
        }else{
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

        if (numberOfWords == 1 && strcasecmp(input[0], "CONNECT") == 0) {
            if (connected) {
                printString("Floyd is already connected.\n");
            } else {
                connected = 1;
                manageLogIn();
                printString("Floyd connected to HAL 9000 system, welcome music lover!\n");
            }
        } else if (numberOfWords == 1 && strcasecmp(input[0], "LOGOUT") == 0) {
            if (connected) {
                printString("Thanks for using HAL 9000, see you soon, music lover!\n");
                connected = 0;
                manageLogOut();
            } else {
                printString("Cannot Logout, you are not connected to HAL 9000\n");
            }
        } else if (numberOfWords == 2 && strcasecmp(input[0], "LIST") == 0 && strcasecmp(input[1], "SONGS") == 0) {
            if (connected) {
                manageListSongs();
            } else {
                printString("Cannot List Songs, you are not connected to HAL 9000\n");
            }
        } else if (numberOfWords == 2 && strcasecmp(input[0], "LIST") == 0 && strcasecmp(input[1], "PLAYLISTS") == 0) {
            if (connected) {
                manageListPlaylists();
            } else {
                printString("Cannot List Playlist, you are not connected to HAL 9000\n");
            }
        } else if (numberOfWords >= 1 && strcasecmp(input[0], "DOWNLOAD") == 0 && strcasecmp(input[1], "PLAYLIST") == 0) {
            if (connected) {
                manageDownloadPlaylist(input, numberOfWords);
            } else {
                printString("Cannot download, you are not connected to HAL 9000\n");
            }
        } else if (numberOfWords == 2 && strcasecmp(input[0], "CHECK") == 0 && strcasecmp(input[1], "DOWNLOADS") == 0) {
            if (connected) {
                manageCheckDownloads(&list);
            } else {
                printString("Cannot Check Downloads, you are not connected to HAL 9000\n");
            }
        }else if (numberOfWords >= 1 && strcasecmp(input[0], "DOWNLOAD") == 0) {
            if (connected) {
                manageDownload(input, numberOfWords);
            } else {
                printString("Cannot download, you are not connected to HAL 9000\n");
            }
        } else {
            printString("ERROR: Please input a valid command.\n");
        }
    }
    ctrl_C_function();
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