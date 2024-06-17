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
    char *name;
    int port;
    char *ip;
    int numConnections;
    char** bowmans;
}PooleServer;

typedef struct {
    uint8_t type;
    uint16_t headerLength;
    char *header;
    uint32_t id;  
    char *data;
    uint32_t dataLength;
} Frame;

typedef struct{
    char *name;
    int numSongs;
    char** songs;
}Playlist; 

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

typedef struct{
    int id;
    int fd;
    char *filePath;
    int songSize;
} SendThread;

typedef struct{
    int socket;
    char * name;
}songDownloaded;


#endif