#include "common.h"
#include "structures.h"

Bowman bowman;



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
    printStringWithHeader("^\nFreeing memory..."," ");
    printStringWithHeader("."," ");
    //sleep(1);
    free(bowman.name);
    free(bowman.ipDiscovery);
    free(bowman.folder);

    printStringWithHeader(" ."," ");
    //sleep(1);

    //free(pooleToConnect.name);
    //free(pooleToConnect.ip);

    printStringWithHeader("  ."," ");
    //sleep(1);
    //close(discoverySockfd);
    //close(pooleSockfd);

    printStringWithHeader("   ."," ");
    //sleep(1);
    //freeFrame(frame);

    printStringWithHeader("    ."," ");
    //sleep(1);


    printStringWithHeader("     ...All memory freed...","\n\nReady to EXIT this BOWMAN Process.");


    exit(EXIT_SUCCESS);
}

//-----------------------------------------------------------------





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
        
        printString("\n$ ");

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
        //printInt("numberOfSpaces: ", numberOfSpaces);
        //printInt("inputLength: ", inputLength);
        //printInt("numberOfWords: ", numberOfWords);
        //printString("eo\n");
        char *input[numberOfSpaces + 1];
        //slit into tokens by space
        char *token = strtok(buffer, " ");
        
        while (numberOfWords <= numberOfSpaces && token != NULL) {
            //printStringWithHeader("token: ", token);
            input[numberOfWords] = token;
            token = strtok(NULL, " ");
            numberOfWords++;
        }
        //printInt("numberOfSpaces: ", numberOfSpaces);
        //printInt("inputLength: ", inputLength);
        //printInt("numberOfWords: ", numberOfWords);


        if (numberOfWords == 1 && strcasecmp(input[0], "CONNECT") == 0) {
            if (connected) {
                printString("Floyd is already connected.\n");
            } else {
                connected = 1;
                printString("Floyd connected to HAL 9000 system, welcome music lover!\n");
            }
        } else if (numberOfWords == 1 && strcasecmp(input[0], "LOGOUT") == 0) {
            if (connected) {
                printString("Thanks for using HAL 9000, see you soon, music lover!\n");
                active = 0;
                connected = 0;
            } else {
                printString("Cannot Logout, you are not connected to HAL 9000\n");
            }
        } else if (numberOfWords == 2 && strcasecmp(input[0], "LIST") == 0 && strcasecmp(input[1], "SONGS") == 0) {
            if (connected) {
                printString("There are 6 songs available for download:\n1. Macarena.mp3\n2. Walk_of_life.mp3\n3. Levels.mp3\n4. Less_is_more.mp3\n5. Stand_up.mp3\n6. Isla_nostalgia.mp3\n");
            } else {
                printString("Cannot List Songs, you are not connected to HAL 9000\n");
            }
        } else if (numberOfWords == 2 && strcasecmp(input[0], "LIST") == 0 && strcasecmp(input[1], "PLAYLISTS") == 0) {
            if (connected) {
                printString("There are 2 lists available for download:\n1. Pim_pam_trucu_trucu\na. Levels.mp3\nb. Stand_up.mp3\nc. Macarena.mp3\n2. Copeo_pre_costa\na. Macarena.mp3\nb. Isla_nostalgia.mp3\n");
            } else {
                printString("Cannot List Playlist, you are not connected to HAL 9000\n");
            }
        } else if (numberOfWords >= 1 && strcasecmp(input[0], "DOWNLOAD") == 0) {
            if (connected) {
                printString("Download started!\n");
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
    
    //SIGNAL ctrl C
    signal(SIGINT, ctrl_C_function);

    checkArgc(argc);

    openFile(argv[1], &bowFd);

    buffer = read_until(bowFd, '\n');
    size_t length = strlen(buffer);
    
    removeAmp(buffer, &Amp);

    bowman.name = malloc((length - Amp ) * sizeof(char) );
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