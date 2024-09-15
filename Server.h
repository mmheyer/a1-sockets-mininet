#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>

class Server {
public:
    // Constructor to initialize port and queue size
    Server(int port, int queue_size);

    // Runs the server that listens for connections
    int run_server();

private:
    // Handles an individual connection from a client
    int handle_connection(int connectionfd);

    // Helper function to create and bind the server socket
    int setup_server_socket();

    int port;         // Port to listen on
    int queue_size;   // Size of the listen queue
    int sockfd;       // Server socket file descriptor
};

#endif
