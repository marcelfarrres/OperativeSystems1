#include "common.h"
#include "structures.h"

Discovery discovery;
int pooleSocketFd, bowmanSocketFd;
fd_set setOfSockFd;
int *sockets;
int numberOfSockets;


//SIGNALS PHASE-----------------------------------------------------------------
//SIGNALS PHASE-----------------------------------------------------------------
void ctrl_C_function() {
    printStringWithHeader("^\nFreeing memory...", " ");
    printStringWithHeader(".", " ");
    
    free(discovery.ipPoole);
    free(discovery.ipBowman);

    FD_ZERO(&setOfSockFd);

    close(pooleSocketFd);
    close(bowmanSocketFd);
    
    printStringWithHeader(" .", " ");
    printStringWithHeader("  .", " ");
    
    if (sockets != NULL) {
        free(sockets);
        printStringWithHeader("   .", " ");
    }

    printStringWithHeader("    .", " ");
    printStringWithHeader("     ...All memory freed...", "\n\nReady to EXIT this DISCOVERY Process.");
    
    exit(EXIT_SUCCESS);
} 


//DISCOVERY SOCKET FUCNCTIONS-----------------------------------------------------------------
void handleNewConnection(int sockfd) {
    struct sockaddr_in auxSocket;
    socklen_t auxSocketLength = sizeof(auxSocket);
    int newSocket = accept(sockfd, (struct sockaddr *)&auxSocket, &auxSocketLength);

    if (newSocket < 0) {
        printString("\nERROR ACCEPTING NEW CONNECTION");
        exit(EXIT_FAILURE);
    }else{
        printString("\nNEW CONNECTION ACCEPTED");
    }

    sockets = realloc(sockets, sizeof(int) * (numberOfSockets + 1));
    sockets[numberOfSockets] = newSocket;
    numberOfSockets++;
    FD_SET(newSocket, &setOfSockFd);
    printInt("NEW CONNECTION, SOCKET ID: ", newSocket );
    
}


void processClientMessage(int sockfd) {
    printInt("\nNEW MESSAGE FROM: ", sockfd );
    
}
//-----------------------------------------------------------------


int main(int argc, char *argv[]) {
    //AUXILIAR VARIABLES
    int discoveryFd = -1; //DISCOVERY FILE
    numberOfSockets = 0;

    //SIGNAL ctrl C
    signal(SIGINT, ctrl_C_function);

    checkArgc(argc);

    openFile(argv[1], &discoveryFd);

    readStringFromFile(discoveryFd, '\n', &discovery.ipPoole);
    readIntFromFile(discoveryFd, '\n', &discovery.portPoole);
    readStringFromFile(discoveryFd, '\n', &discovery.ipBowman);
    readIntFromFile(discoveryFd, '\n', &discovery.portBowman);

    close(discoveryFd);

    printStringWithHeader("Discovery ipPoole:", discovery.ipPoole);
    printInt("Discovery portPoole:", discovery.portPoole);
    printStringWithHeader("Discovery ipBowman:", discovery.ipBowman);
    printInt("Discovery portBowman:", discovery.portBowman);

    //sleep(15);

    printString("Creating Socket for Poole and Bowman..\n");
    



    pooleSocketFd = createServer(discovery.portPoole, discovery.ipPoole);
    printInt("\npooleSocketFd: ", pooleSocketFd);
    bowmanSocketFd = createServer(discovery.portBowman, discovery.ipBowman);
    printInt("\nBowmanSocketFd: ", bowmanSocketFd);

/*
    struct sockaddr_in auxSocket;
    socklen_t auxSocketLength = sizeof(auxSocket);


    int newSocket = accept(pooleSocketFd, (struct sockaddr *)&auxSocket, &auxSocketLength);

    if (newSocket < 0) {
        printString("\nERROR ACCEPTING NEW CONNECTION");
        exit(EXIT_FAILURE);
    }else{
        printString("\nNEW CONNECTION ACCEPTED");
    }
*/
    FD_ZERO(&setOfSockFd);
    FD_SET(pooleSocketFd, &setOfSockFd);
    FD_SET(bowmanSocketFd, &setOfSockFd);

    //int maxNumberOfSockFd = 512;   
    while (1) {
        struct sockaddr_in c_addr;
        socklen_t c_len = sizeof(c_addr);

        fd_set tmp_fds = setOfSockFd;  // Copy the set for select

        select(512, &tmp_fds, NULL, NULL, NULL);

        for (int i = 0; i < 512; i++) {
            if (FD_ISSET(i, &tmp_fds)) {
                if (i == pooleSocketFd || i == bowmanSocketFd) {
                    printString("New connection\n");
                    int newsock = accept(i, (void *)&c_addr, &c_len);

                    printInt("New client with socket:", newsock);
                    if (newsock < 0) {
                        perror("accept");
                        exit(EXIT_FAILURE);
                    }

                    sockets = realloc(sockets, sizeof(int) * (numberOfSockets + 1));
                    sockets[numberOfSockets] = newsock;
                    numberOfSockets++;
                    FD_SET(newsock, &setOfSockFd);

                    if (i == pooleSocketFd) {
                        printf("Poole connected\n");
                    } else {
                        printf("Bowman connected\n");
                    }
                } else {
                    //printf("New message from %d\n", i);
                    //discoveryMenu(i);
                }
            }
        }
    }





    return 0;
}
