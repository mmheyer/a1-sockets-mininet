#include "Client.h"
#include "helpers.h"  // For make_client_sockaddr()
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

static const int MAX_MESSAGE_SIZE = 256;

Client::Client(const std::string &hostname, int port)
    : hostname(hostname), port(port) {}

// This function creates a socket and connects to the server
int Client::connect_to_server() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Error: Unable to create socket\n";
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    struct hostent *host_entry = gethostbyname(hostname.c_str());
    if (!host_entry) {
        std::cerr << "Error: Unknown host " << hostname << "\n";
        close(sock);
        return -1;
    }

    memcpy(&addr.sin_addr, host_entry->h_addr, host_entry->h_length);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        std::cerr << "Error: Unable to connect to " << hostname << ":" << port << "\n";
        close(sock);
        return -1;
    }

    return sock;
}

// Sends the message to the server and waits for a response
int Client::send_message(const std::string &message) {
    if (message.length() > MAX_MESSAGE_SIZE) {
        std::cerr << "Error: Message exceeds maximum length\n";
        return -1;
    }

    int sock = connect_to_server();
    if (sock == -1) return -1;

    // Send the message to the server
    if (send(sock, message.c_str(), message.length(), 0) == -1) {
        std::cerr << "Error: Unable to send message\n";
        close(sock);
        return -1;
    }
    std::cout << "Message sent\n";
    shutdown(sock, SHUT_WR);

    // Wait for the server's integer response
    char buffer[MAX_MESSAGE_SIZE] = {};
    int total_bytes = 0;
    while (true) {
        int bytes_received = recv(sock, buffer + total_bytes, MAX_MESSAGE_SIZE - total_bytes, 0);
        if (bytes_received <= 0) break;
        total_bytes += bytes_received;
    }

    // Convert the response to an integer
    int response = atoi(buffer);
    if (response == 0) {
        std::cerr << "Error: Invalid response from server\n";
        close(sock);
        return -1;
    }

    std::cout << "Server response: " << response << "\n";

    // Close the connection
    close(sock);
    return response;
}
