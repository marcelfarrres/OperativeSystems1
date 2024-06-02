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
                    int result = readFrame(i, &frame);
                    if (result <= 0) {
                        socketDisconnectedDiscovery(i);
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

                        printPooleServer(listOfPooleServers[numberOfPooleServers - 1]);
                           
                    }else if(strcmp(frame.header, "NEW_BOWMAN") == 0){
                        printFrame(&frame);
                        numberOfData = separateData(frame.data, &separatedData, &numberOfData);
                        printStringWithHeader("separatedData[0]:", separatedData[0]);
                        //TODO: Select the Poole to send the data and store the name inside and send the frame to the bowman with the poole port and ip

                    }
    
                }
            }
        }
    }





    return 0;
}
