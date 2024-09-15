#ifndef CLIENT_H
#define CLIENT_H

#include <string>

class Client {
public:
    // Constructor
    Client(const std::string &hostname, int port);

    // Function to send a message to the server
    int send_message(const std::string &message);

private:
    std::string hostname;  // Server hostname
    int port;              // Server port

    // Helper function to make socket and connect to the server
    int connect_to_server();
};

#endif // CLIENT_H
