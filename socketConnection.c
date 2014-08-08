#include "ftpServer.h"
#include "response.h"
#include "socketConnection.h"

int prepare_socket(int port, struct addrinfo *results) {

    int listening_socket;
    
    // Socket address information:
    struct sockaddr_in address_in; // sockaddr_in contains IPv4 information 
    address_in.sin_family = AF_INET; // IPv4
    address_in.sin_port = htons(port);
    address_in.sin_addr.s_addr = INADDR_ANY; // expects 4-byte IP address (INADDR_ANY = use my IPv4 address)
        
    struct addrinfo address; // addrinfo contains info about the socket
    memset(&address, 0, sizeof(address));
    address.ai_socktype = SOCK_STREAM;
    address.ai_protocol = 0; // 0 = choose correct protocol for stream vs datagram
    address.ai_addr = (struct sockaddr *) &address_in;
    address.ai_flags = AI_PASSIVE; // fills in IP automatically
    
    char port_str[5];
    sprintf(port_str, "%d", port);
     
    int status = getaddrinfo(NULL, port_str, &address, &results); // results stored in results
    char message[50];
    sprintf(message, "getaddrinfo error: %s\n", gai_strerror(status));
    check_status(status, message);
    
    // Create socket
    listening_socket = socket(results->ai_family, results->ai_socktype, results->ai_protocol); 
    if (listening_socket > 0) {
        printf("Socket created, listening on port %s.\n", port_str);
    }
    
    // Allow reuse of port -- from Beej
    int yes = 1;
    int sockopt_status = setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    check_status(sockopt_status, "setsockopt");
    
    // Bind socket to address
    // socket id, *sockaddr struct w address info, length (in bytes) of address
    int bind_status = bind(listening_socket, (struct sockaddr *) &address_in, sizeof(address_in));                                                          
    check_status(bind_status, "bind");
    printf("Binding socket...\n");
    
    return listening_socket;
}

// Accept connection and spawn new thread
int begin_connection(int listening_socket, void *on_create_function) {
    // Make a new socket specifically for sending/receiving data w this client
    int new_socket;
    
    // Info about incoming connection goes into sockaddr_storage struct 
    struct sockaddr_storage client;
    socklen_t addr_size = sizeof(client);
    
    new_socket = accept(listening_socket, (struct sockaddr *) &client, &addr_size);
    check_status(new_socket, "accept");

    if (new_socket > 0) {
        pthread_t tid;
        NUM_THREADS++;
        pthread_create(&tid, NULL, on_create_function, &new_socket);
        printf("\nA client has connected (socket %d), new thread created. Total threads: %d.\n", new_socket, NUM_THREADS);
    }
    return new_socket;
}