#include "Server.h"
#include "helpers.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static const int MAX_MESSAGE_SIZE = 256;
static const char *RESPONSE_OK = "200";
static const char *RESPONSE_ERROR = "400";

Server::Server(int port, int queue_size)
    : port(port), queue_size(queue_size), sockfd(-1) {}

int Server::setup_server_socket() {
    // (1) Create the socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Error creating socket");
        return -1;
    }

    // (2) Set the "reuse port" option
    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("Error setting socket options");
        close(sockfd);
        return -1;
    }

    // (3) Set up the server address using helper function
    struct sockaddr_in addr;
    if (make_server_sockaddr(&addr, port) == -1) {
        close(sockfd);
        return -1;
    }

    // (4) Bind the socket to the address
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("Error binding socket");
        close(sockfd);
        return -1;
    }

    // (5) Start listening on the socket
    if (listen(sockfd, queue_size) == -1) {
        perror("Error listening on socket");
        close(sockfd);
        return -1;
    }

    printf("Server is listening on port %d...\n", port);
    return 0;
}

int Server::handle_connection(int connectionfd) {
    char buffer[MAX_MESSAGE_SIZE] = {};
    int loc = 0;
    bool response_ok = true;

    // (1) Receive the message from the client
    while (true) {
        int received = recv(connectionfd, buffer + loc, MAX_MESSAGE_SIZE - loc, 0);
        if (received == -1) {
            perror("Error receiving message");
            response_ok = false;
            break;
        }
        loc += received;
        if (received == 0) {
            break;
        }
        if (loc > MAX_MESSAGE_SIZE) {
            response_ok = false;
            break;
        }
    }

    // (2) Print the received message
    if (response_ok) {
        printf("Received: %s\n", buffer);
    }

    // (3) Send a response to the client
    const char *response = response_ok ? RESPONSE_OK : RESPONSE_ERROR;
    if (send(connectionfd, response, strlen(response), 0) == -1) {
        perror("Error sending response");
        return -1;
    }

    // (4) Close the connection
    close(connectionfd);
    return 0;
}

int Server::run_server() {
    // (1) Setup server socket
    if (setup_server_socket() == -1) {
        return -1;
    }

    // (2) Serve incoming connections
    while (true) {
        // Accept a connection
        int connectionfd = accept(sockfd, nullptr, nullptr);
        if (connectionfd == -1) {
            perror("Error accepting connection");
            close(sockfd);
            return -1;
        }

        // Handle the connection
        printf("Accepted a connection...\n");
        if (handle_connection(connectionfd) == -1) {
            close(sockfd);
            return -1;
        }

        printf("Connection handled successfully.\n");
    }

    // Close the server socket
    close(sockfd);
    return 0;
}
