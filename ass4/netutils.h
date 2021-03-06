#ifndef NETUTILS_H_
#define NETUTILS_H_
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <strings.h>



/**
 *
 * Connects to any TCP server with specified host and port
 *
 * Params:
 *  hostname - the ip address/hostname to connect to
 *  port - the port number on the host machine to connect to
 *
 * NOTE: This was developed with help from Beej's Guide to Network Programming
 *
 *  Returns: -1 on failure or a valid socket file descriptor on success
 *
 * */
int connect_to_tcp_server(char* hostname, char* port);


/**
 * 
 * Starts a TCP server listening on the specified port this
 * can be run as a nobblocking server that is threaded by
 * making sure that the clientCallback function does not block.
 *
 * If the callback blocks then the server will not be accepting 
 * more connections.
 *
 * Params:
 *  listeningPort -the port the server will listen for connections
 *  on 
 *  clientCallback - a call back to handle a client
 *  emitPort - whether to print the ephemeral port to stderr
 *  tryAllInterfaces - whether to try to listen on any available interface
 *   
 *  NOTE: This is adapted from a C++ IRC server clone I wrote called Chatnet 
 * Returns: -1 on any error 
 * */
int tcp_server(char* listeningPort, 
        void (*clientCallback) (int clientSocket), bool emitPort,
        bool tryAllInterfaces);

/**
 * Utility to return the port the sever is listening on
 * 
 * Params:
 * socket - the socket the server is listening on
 * sa - the address the server is listening on
 *
 *
 * Returns: -1 on error or a valid port > 0
 * */
int get_port_bounded_to(int socket, struct sockaddr* sa);
#endif 

