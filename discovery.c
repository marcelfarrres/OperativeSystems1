#include "common.h"
#include "structures.h"

Discovery discovery;
int pooleSocketFd, bowmanSocketFd;
fd_set setOfSockFd;
int *sockets;
int numberOfSockets;
Frame frame; 
PooleServer ** listOfPooleServers;
int numberOfPooleServers;
char** separatedData;
int numberOfData = 0;


struct sockaddr_in c_addr;
socklen_t c_len = sizeof(c_addr);


//SIGNALS PHASE-----------------------------------------------------------------
void ctrl_C_function() {
    printStringWithHeader("^\nFreeing memory...", " ");
    printStringWithHeader(".", " ");
    
    free(discovery.ipPoole);
    free(discovery.ipBowman);

    

    close(pooleSocketFd);
    close(bowmanSocketFd);
    
    printStringWithHeader(" .", " ");
    printStringWithHeader("  .", " ");
    
    FD_ZERO(&setOfSockFd);
    for (int i = 0; i < numberOfSockets; i++) {
        close(sockets[i]);
    }
    free(sockets);

    free(frame.data);
    free(frame.header);

    freeSeparatedData(&separatedData, &numberOfData);

    for (int i = 0; i < numberOfPooleServers; i++) {
        for (int j = 0; j < listOfPooleServers[i]->numConnections; j++) {
            free(listOfPooleServers[i]->bowmans[j]);
        }
        free(listOfPooleServers[i]->bowmans);
        free(listOfPooleServers[i]->name);
        free(listOfPooleServers[i]->ip);
        free(listOfPooleServers[i]);
    }
    free(listOfPooleServers);

    printStringWithHeader("    .", " ");
    printStringWithHeader("     ...All memory freed...", "\n\nReady to EXIT this DISCOVERY Process.");
    
    exit(EXIT_SUCCESS);
} 

//POOLE SERVERS FUCNTIONS--------------------------------------------------------------------

void addPooleServerToTheList(PooleServer * newServer){

    numberOfPooleServers++;
    listOfPooleServers = realloc(listOfPooleServers, sizeof(PooleServer*) * numberOfPooleServers);
    listOfPooleServers[numberOfPooleServers - 1] = newServer;

}

PooleServer* findPooleServerWithLeastConnections() {
    if (numberOfPooleServers == 0) {
        return NULL; // Return NULL if the list is empty
    }

    PooleServer *leastConnectionsServer = NULL;
    int leastConnections = 999999;

    for (int i = 0; i < numberOfPooleServers; i++) {
        if (listOfPooleServers[i]->numConnections < leastConnections) {
            leastConnections = listOfPooleServers[i]->numConnections;
            leastConnectionsServer = listOfPooleServers[i];
        }
    }

    return leastConnectionsServer;
}

void addBowman(PooleServer *server, const char *newBowman) {
  
    server->bowmans = realloc(server->bowmans, sizeof(char*) * (server->numConnections + 1));
    if (server->bowmans == NULL) {
        printString("Failed to allocate memory for bowmans array\n");
        return;
    }

    server->bowmans[server->numConnections] = strdup(newBowman);
    if (server->bowmans[server->numConnections] == NULL) {
        printString("Failed to allocate memory for new bowman\n");
        return;
    }
    server->numConnections++;
}
void removeConnection(const char *name) {
    for (int s = 0; s < numberOfPooleServers; s++) {
        PooleServer *server = listOfPooleServers[s];
        for (int i = 0; i < server->numConnections; i++) {
            if (strcmp(server->bowmans[i], name) == 0) {
                // Free the memory for the name
                free(server->bowmans[i]);

                // Shift remaining connections down
                for (int j = i; j < server->numConnections - 1; ++j) {
                    server->bowmans[j] = server->bowmans[j + 1];
                }

                // Reallocate memory for bowmans array
                server->numConnections--;
                server->bowmans = realloc(server->bowmans, server->numConnections * sizeof(char*));

                if (server->bowmans == NULL && server->numConnections > 0) {
                    // Handle memory allocation failure
                    perror("Failed to reallocate memory");
                    exit(EXIT_FAILURE);
                }

                break;
            }
        }
    }
}



//DISCOVERY SOCKET FUCNCTIONS-----------------------------------------------------------------

void socketDisconnectedDiscovery(int socket_){
    
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
    int discoveryFd = -1; //DISCOVERY FILE
    numberOfSockets = 0;
    initFrame(&frame);
    numberOfPooleServers = 0;
    

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

    printString("Creating Socket for Poole and Bowman..\n");
    
    pooleSocketFd = createServer(discovery.portPoole);
    printInt("\npooleSocketFd: ", pooleSocketFd);
    bowmanSocketFd = createServer(discovery.portBowman);
    printInt("\nBowmanSocketFd: ", bowmanSocketFd);

    FD_ZERO(&setOfSockFd);
    FD_SET(pooleSocketFd, &setOfSockFd);
    FD_SET(bowmanSocketFd, &setOfSockFd);
    
    
    while (1) {
        
        fd_set auxiliarSetOf = setOfSockFd;  

        select(50, &auxiliarSetOf, NULL, NULL, NULL);

        for (int i = 0; i < 50; i++) {
            if (FD_ISSET(i, &auxiliarSetOf)) {
                if (i == pooleSocketFd){
                    
                    int newPoole = accept(i, (void *)&c_addr, &c_len);
                    if (newPoole < 0) {
                        printString("\nERROR: NewPoole not connected\n");
                        ctrl_C_function();
                        exit(EXIT_FAILURE);
                    }
                    printInt("\nNew POOLE connected with socket: ", newPoole);
                    sockets = realloc(sockets, sizeof(int) * (numberOfSockets + 1));
                    printInt("\nSizeof(Socket): ", sizeof(int) * (numberOfSockets + 1));
                    printInt("\nNew size of ..sockets.. array: ", sizeof(sockets));
                    sockets[numberOfSockets] = newPoole;
                    numberOfSockets++;
                    FD_SET(newPoole, &setOfSockFd);

                } else if (i == bowmanSocketFd) {
                    int newBowman = accept(i, (void *)&c_addr, &c_len);
                    if (newBowman < 0) {
                        printString("\nERROR: NewBowman not connected\n");
                        ctrl_C_function();
                        exit(EXIT_FAILURE);
                    }
                    printInt("\nNew BOWMAN connected with socket: ", newBowman);
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
                        
                        socketDisconnectedDiscovery(i);
                        printAllPooleServers(listOfPooleServers, numberOfPooleServers);
                    }else if(strcmp(frame.header, "NEW_POOLE") == 0){
                        printFrame(&frame);
                        numberOfData = separateData(frame.data, &separatedData, &numberOfData);

                        PooleServer *newServer = (PooleServer *) malloc(sizeof(PooleServer));
        
                        newServer->name = strdup(separatedData[0]);
                        newServer->ip = strdup(separatedData[1]);
                        newServer->port = atoi(separatedData[2]);
                        newServer->numConnections = 0;
                        newServer->bowmans = NULL;
                        addPooleServerToTheList(newServer);
                        sendOkConnectionDiscoveryPoole(i);
                        printString("Poole server added!\n");

                        printAllPooleServers(listOfPooleServers, numberOfPooleServers);

                           
                    }else if(strcmp(frame.header, "NEW_BOWMAN") == 0){
                        printFrame(&frame);
                        numberOfData = separateData(frame.data, &separatedData, &numberOfData);
                        printStringWithHeader("separatedData[0]:", separatedData[0]);
                        if(numberOfPooleServers == 0){
                            printString("ERROR: No Poole Servers available right now... ");
                            sendKoConnectionDiscoveryBowman(i);
                            socketDisconnectedDiscovery(i);
                        }else{
                            printString("\nConnection to a Poole server...\n");
                            PooleServer * pooleToConnect = findPooleServerWithLeastConnections();
                            addBowman(pooleToConnect, separatedData[0]);
                             
                            printAllPooleServers(listOfPooleServers, numberOfPooleServers);


                            char *miniBuffer;
                            asprintf(&miniBuffer, "%s&%s&%d", pooleToConnect->name, pooleToConnect->ip, pooleToConnect->port);
                            sendOkConnectionDiscoveryBowman(i, miniBuffer);
                            free(miniBuffer);
                        }
                    }else if(strcmp(frame.header, "EXIT") == 0){
                        printFrame(&frame);
                        numberOfData = separateData(frame.data, &separatedData, &numberOfData);
                        printStringWithHeader("This Bowman Clossing Session: ", separatedData[0]);
                        removeConnection(separatedData[0]);
                        printAllPooleServers(listOfPooleServers, numberOfPooleServers);
                        sendLogoutResponse(i);
                        printString("\n-\n");
                        

                    }
    
                }
            }
        }
    }





    return 0;
}
