#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>

#include "netutils.h"

int connect_to_tcp_server(char* hostname, char* port) {
    int socketFileDescriptor;
    struct addrinfo hints;
    struct addrinfo* serverInfo;
    struct addrinfo* currentServerInfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // use either Ipv4 or Ipv6
    hints.ai_socktype = SOCK_STREAM; // TCP
    
    if (getaddrinfo(hostname, port, &hints, &serverInfo) != 0) {
        return -1;
    }

    for (currentServerInfo = serverInfo; currentServerInfo != NULL; 
            currentServerInfo = currentServerInfo->ai_next) {
    
        if ((socketFileDescriptor = socket(currentServerInfo->ai_family,
                currentServerInfo->ai_socktype,
                currentServerInfo->ai_protocol)) == -1) {
            continue; // try again
        }

        if (connect(socketFileDescriptor, currentServerInfo->ai_addr,
                currentServerInfo->ai_addrlen) == -1) {
            continue; // try again
        }
    
        break; // break on first success
    }

    if (currentServerInfo == NULL) {
        return -1; // We never connected
    }

    return socketFileDescriptor;
}


/**
 * A Helper method to make the creation of TCP server cleaner
 *
 * Params:
 *  currentServerInfo - the pointer to store the successful server info
 *  serverInfo - the link list to the head of server info
 *
 *  Returns a valid socket file descriptor or -1
 *
 * */
static int bind_to_socket(struct addrinfo** currentServerInfo, 
        struct addrinfo** serverInfo, 
        bool useEphemeralPort,
        bool tryAllInterfaces) {
    int serverSocketFd;
    char yes = '1'; // to make setsockopt happy
    // loop through the linklist and find a correct socket
    for (*currentServerInfo = *serverInfo; *currentServerInfo != NULL;
            *currentServerInfo = (*currentServerInfo)->ai_next) {
        
        serverSocketFd = socket((*currentServerInfo)->ai_family,
                (*currentServerInfo)->ai_socktype,
                (*currentServerInfo)->ai_protocol);

        if (serverSocketFd == -1) {
            continue;
        }
        if (!useEphemeralPort) {
            //The address might be used again
            if (setsockopt(serverSocketFd, SOL_SOCKET, SO_REUSEADDR,
                    &yes, sizeof(int)) == -1) {
                return -1;
            }
        }
        //Try Bind to the socket
        if (bind(serverSocketFd, (*currentServerInfo)->ai_addr,
                (*currentServerInfo)->ai_addrlen) == -1) {
            if (!tryAllInterfaces) {
                return -1;
            }
            continue;
        }
        //We are binded to the socket and everything went well break the loop
        break;
    }
    //Things might have not went well
    if (currentServerInfo == NULL) {
        return -1;
    }

    return serverSocketFd;
}

int tcp_server(char* listeningPort,
        void (*clientCallback)(int clientSocket),
        bool emitPort,
        bool tryAllInterfaces) {

    int serverSocketFd, clientSocketFd, rv;
    struct addrinfo hints;
    struct addrinfo* serverInfo = NULL;
    struct addrinfo* currentServerInfo = NULL;
    socklen_t clientAddressSize;
    struct sockaddr_storage clientAddress;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    bool useEphemeralPort = false;

    if (!strcmp(listeningPort, "-1")) {
        useEphemeralPort = true;
    }
    //Do DNS lookup
    if ((rv = getaddrinfo(NULL, useEphemeralPort ? "0"  :
            listeningPort, &hints, &serverInfo)) == -1) {
        return -1;
    }
    if ((serverSocketFd = bind_to_socket(&currentServerInfo,
            &serverInfo, useEphemeralPort, tryAllInterfaces)) == -1) {
        return -1;
    }
    if (emitPort) {
        fprintf(stderr, "%d\n", get_port_bounded_to(serverSocketFd, 
                currentServerInfo->ai_addr));
    }
    //We are done with the linklist
    freeaddrinfo(serverInfo);
    //Listen on the port specified, with a backlog of 10
    if (listen(serverSocketFd, 10) == -1) {
        return -1;
    }
    while(true) {
        clientAddressSize = sizeof(clientAddress);
        //Accept a new client
        clientSocketFd = accept(serverSocketFd, 
                (struct sockaddr*) &clientAddress,
                &clientAddressSize);
        if (clientSocketFd == -1) {
            continue;
        }
        // Call the callback with accepted socket
        (*clientCallback)(clientSocketFd); 
    }

    return 0; // never reached
}

int get_port_bounded_to(int socket, struct sockaddr* sa) {
    if (sa->sa_family == AF_INET) {
        struct sockaddr_in ad;
        memset(&ad, 0, sizeof(struct sockaddr_in));
        socklen_t len = sizeof(struct sockaddr_in);
        if (getsockname(socket, (struct sockaddr*)&ad, &len)) {
            return -1;
        } else {
            return ntohs(ad.sin_port);
        }
    }

    struct sockaddr_in6 ad6;
    memset(&ad6, 0, sizeof(struct sockaddr_in6));
    socklen_t len6 = sizeof(struct sockaddr_in6);
    if (getsockname(socket, (struct sockaddr*)&ad6, &len6)) {
        return -1;
    } else {
        return ntohs(ad6.sin6_port);
    }

    return -1;
}
