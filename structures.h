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

typedef struct{
    uint8_t type;
    uint16_t headerLength;
    char *header;
    char *data;
}Frame; 

typedef struct{
    char *name;
    int numSongs;
    char** songs;
}Playlist; 

typedef struct{
    int id;
    int fd;
    char *filePath;
} SendThread;

typedef struct {
    int socket_fd;
    int totalFileSize; 
    char *file_path;
    char *MD5SUM_Poole;
    char *file_name;
} DownloadArgs;

typedef struct FileDownload{
    pthread_t thread_id;
    char *file_name;
    int totalFileSize;
    int currentFileSize;
    int active;
    struct FileDownload *next;
} FileDownload;

#endif