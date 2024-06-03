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

                    
                        
                    }
                }
            }
        }
    }
        


    }




    sleep(100);
    return 0;
}
