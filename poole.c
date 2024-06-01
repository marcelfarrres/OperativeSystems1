#include "common.h"
#include "structures.h"

Poole poole;
int pooleFd = -1; //POOLE FILE
int discoverySocketFd = -1;




void removeAmp(char *buffer, int *Amp) {
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
void ctrl_C_function() {
    printStringWithHeader("^\nFreeing memory...", " ");
    printStringWithHeader(".", " ");
    free(poole.name);
    free(poole.ipDiscovery);
    free(poole.folder);
    free(poole.ipPoole);

    close(pooleFd);
    close(discoverySocketFd);

    printStringWithHeader(" .", " ");
    printStringWithHeader("  .", " ");
    printStringWithHeader("   .", " ");
    printStringWithHeader("    .", " ");
    printStringWithHeader("     ...All memory freed...", "\n\nReady to EXIT this POOLE Process.");

    exit(EXIT_SUCCESS);
}

//-----------------------------------------------------------------

int main(int argc, char *argv[]) {
    //AUXILIAR VARIABLES

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
    //printString(miniBuffer);
    sendNewConnectionPooleDiscovery(discoverySocketFd, miniBuffer);
    free(miniBuffer);

   

    sleep(100);
    return 0;
}
