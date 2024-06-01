#include "common.h"
#include "structures.h"

Discovery discovery;
int pooleSocketFd, bowmanSocketFd;
fd_set setOfSockFd;
int *sockets;
int numberOfSockets;
Frame frame; 
PooleServer ** listOfPooleServers;
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

    FD_ZERO(&setOfSockFd);

    close(pooleSocketFd);
    close(bowmanSocketFd);
    
    printStringWithHeader(" .", " ");
    printStringWithHeader("  .", " ");
    
   for (int i = 0; i < numberOfSockets; i++) {
        close(sockets[i]);
    }
    free(sockets);

    free(frame.data);
    free(frame.header);

    freeSeparatedData(&separatedData, &numberOfData);

    printStringWithHeader("    .", " ");
    printStringWithHeader("     ...All memory freed...", "\n\nReady to EXIT this DISCOVERY Process.");
    
    exit(EXIT_SUCCESS);
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

void handleNewMessage(int messageSocket) {
    

    int result = readFrame(messageSocket, &frame);
    if (result <= 0) {
        socketDisconnectedDiscovery(messageSocket);
        
    }else if(strcmp(frame.header, "NEW_POOLE") == 0){
        printFrame(&frame);
        
        
        numberOfData = separateData(frame.data, &separatedData, &numberOfData);

        //numberOfData = separateData(frame.data, &separatedData, numberOfData);


        

        


        
        
            
        
            /*
            PooleServer *newServer;
            newServer = (PooleServer *) malloc(sizeof(PooleServer));

            newServer->name = strdup(info[0]);
            newServer->ip = strdup(info[1]);
            newServer->port = atoi(info[2]);
            newServer->connnections = 0;
                        servers = realloc(servers, sizeof(PooleServer) * (num_servers + 1));
            servers[num_servers] = newServer;
            num_servers++;
                        sendMessage(sockfd, 0x01, strlen(HEADER_CON_OK), HEADER_CON_OK, "");
            printf("New poole server added\n");
            */
                           
    }
    
}

//-----------------------------------------------------------------


int main(int argc, char *argv[]) {
    //AUXILIAR VARIABLES
    int discoveryFd = -1; //DISCOVERY FILE
    numberOfSockets = 0;
    initFrame(&frame);
    

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
    
    pooleSocketFd = createServer(discovery.portPoole, discovery.ipPoole);
    printInt("\npooleSocketFd: ", pooleSocketFd);
    bowmanSocketFd = createServer(discovery.portBowman, discovery.ipBowman);
    printInt("\nBowmanSocketFd: ", bowmanSocketFd);

    FD_ZERO(&setOfSockFd);
    FD_SET(pooleSocketFd, &setOfSockFd);
    FD_SET(bowmanSocketFd, &setOfSockFd);
    
    
    while (1) {
        
        fd_set auxiliarSetOf = setOfSockFd;  // Copy the set for select

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
                    handleNewMessage(i);
                }
            }
        }
    }





    return 0;
}
