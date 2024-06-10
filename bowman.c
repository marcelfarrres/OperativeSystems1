#include "common.h"
#include "structures.h"

Bowman bowman;
int discoverySocketFd = -1;
int pooleSocketFd = -1;
char** separatedData;
int numberOfData = 0;

FileDownload *download_head = NULL;
pthread_mutex_t download_mutex = PTHREAD_MUTEX_INITIALIZER;


Frame frame;




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

    
    

    printStringWithHeader("     ...All memory freed...","\n\nReady to EXIT this BOWMAN Process.");


    exit(EXIT_SUCCESS);
}

//-----------------------------------------------------------------

void add_download(DownloadArgs *args) {
    pthread_mutex_lock(&download_mutex);
    
    FileDownload *new_download = malloc(sizeof(FileDownload));
    if (new_download == NULL) {
        perror("Memory allocation failed for new download");
        ctrl_C_function();
    }

    new_download->file_name = strdup(args->file_name); // Deep copy the file name
    new_download->totalFileSize = args->totalFileSize;
    new_download->currentFileSize = 0;
    new_download->active = 1;
    new_download->next = NULL;

    if (download_head == NULL) {
        download_head = new_download;
    } else {
        FileDownload *current = download_head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_download;
    }

    pthread_mutex_unlock(&download_mutex);
}


void *downloadThread(void *args) {

    DownloadArgs *downloadArgs = (DownloadArgs *)args;

    FileDownload *current;

    int pooleSockfd = downloadArgs->socket_fd;
    int file_size = downloadArgs->totalFileSize;
    char *file_path = downloadArgs->file_path;
    char *MD5SUM_Poole = downloadArgs->MD5SUM_Poole;

    int fd = open(file_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd == -1) {
            perror("Error opening file");
            ctrl_C_function();
        }

    int bytesReceived = 0;

    while (bytesReceived < file_size) {

        int dataLength = readFrame(pooleSockfd, &frame);

        numberOfData = separateData(frame.data, &separatedData, &numberOfData);

        write(fd, separatedData[1], dataLength - ((int)strlen(separateData[0])));

        bytesReceived += dataLength;


        pthread_mutex_lock(&download_mutex);
        FileDownload *current = download_head;
        while (current != NULL) {
            if (strcmp(current->file_name, downloadArgs->file_name) == 0) {
                current->currentFileSize = bytesReceived;
                break;
            }
            current = current->next;
        }
        pthread_mutex_unlock(&download_mutex);
        
    }

    // MD5 checksum
    char* md5sum = calloc(33, sizeof(char));
    //int md5sum_result = calculate_md5sum(file_path, md5sum);
    int md5sum_result = 1;

    if (md5sum_result == -1) {
        printString("\n MD5 checksum Errorrr\n");
        ctrl_C_function();
    } else {
        if (strcmp(md5sum, MD5SUM_Poole) == 0) {
            sendCheckResult(pooleSockfd, 1);
        } else {
            sendCheckResult(pooleSockfd, 0);
        }
    }
    

    pthread_mutex_lock(&download_mutex);
    current = download_head;
    while (current != NULL) {
        if (strcmp(current->file_name, downloadArgs->file_name) == 0) {
            current->active = 0;
            break;
        }
        current = current->next;
    }
    pthread_mutex_unlock(&download_mutex);

    close(fd);
    free(downloadArgs);
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
    sendDownloadSong(pooleSocketFd,  song);
    int result = readFrame(pooleSocketFd, &frame);
    printFrame(&frame);

    if (result <= 0) {
        printString("\nERROR: OK not receieved\n");
        ctrl_C_function();
    }else if(strcmp(frame.header, "NEW_FILE") != 0){
        printString("\nERROR: not what we were expecting\n");
        printFrame(&frame);
        ctrl_C_function();
        
    }else{
        printStringWithHeader("Download started:", song);
        numberOfData = separateData(frame.data, &separatedData, &numberOfData);


        pthread_t thread;
        DownloadArgs *args = malloc(sizeof(DownloadArgs));


        args->socket_fd = pooleSocketFd;
        args->totalFileSize = separateData[1];
        args->file_path = bowman.folder;
        args->MD5SUM_Poole = separateData[2];
        args->file_name = separateData[0];

        add_download(args);

        if (pthread_create(&thread, NULL, downloadThread, args) != 0) {
            perror("Failed to create download thread");
            free(args);
            ctrl_C_function();
        }

        pthread_detach(thread);

    }



    
    free(song);

}

void menu() {
    char buffer[200];
    int connected = 0;
    int active = 1;
    int inputLength;
    int numberOfWords = 0;
    int numberOfSpaces = 0;
    
    

    while (active) {
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
                        buffer[j] = '\0'; //erase the last space if any
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
        } else if (numberOfWords >= 1 && strcasecmp(input[0], "DOWNLOAD") == 0) {
            if (connected) {
                manageDownload(input, numberOfWords);
            } else {
                printString("Cannot download, you are not connected to HAL 9000\n");
            }
        } else if (numberOfWords == 2 && strcasecmp(input[0], "CHECK") == 0 && strcasecmp(input[1], "DOWNLOADS") == 0) {
            if (connected) {
                printString("You have no ongoing or finished downloads\n");
            } else {
                printString("Cannot Check Downloads, you are not connected to HAL 9000\n");
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