// isopen.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "isopen.h"  // Include the header file

// Function implementation
int isOpen(const char *ip, int port) {
    int sock;
    struct sockaddr_in server;
    
    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return 0; // Return false if socket creation fails
    }

    // Setup the server address structure
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // Try to connect
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("IP: %s Port: %d Closed\n", ip, port);
        close(sock); // Close the socket
        return 0; // Return false if connection fails
    }

    // If connection is successful
    printf("IP: %s Port: %d Opened\n", ip, port);
    
    // Shut down the connection and close the socket
    shutdown(sock, SHUT_RDWR);
    close(sock); // Close the socket

    return 1; // Return true if the port is open
}
