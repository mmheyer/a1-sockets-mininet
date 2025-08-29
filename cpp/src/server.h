#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>

class Server {
private:
    int listen_port;
    int queue_size;

public:
    Server(int port, int q_size) : listen_port(port), queue_size(q_size) {}

    // Member function to handle a single connection from a client
    int handle_connection(int connectionfd);

    // Member function to run the server and listen for incoming connections
    int run_server();

    // Helper function to validate the port number
    bool validate_port() {
        return listen_port >= 1024 && listen_port <= 65535;
    }
};

#endif
