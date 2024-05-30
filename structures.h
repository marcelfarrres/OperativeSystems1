#ifndef _STRUCTURES_H_
#define _STRUCTURES_H_


typedef struct{
    char *name;
    char *folder;
    char *ipDiscovery;
    int portDiscovery;
    char *ipPoole;
    int portPoole;
}Poole;

typedef struct{
    char *name;
    char *folder;
    char *ipDiscovery;
    int portDiscovery;
}Bowman;

typedef struct{
    char *ipPoole;
    int portPoole;
    char *ipBowman;
    int portBowman;
}Discovery;

typedef struct{
    uint8_t type;
    uint16_t headerLength;
    char *header;
    char *data;
}Frame; 

#endif