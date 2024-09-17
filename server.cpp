#include "server.h"
#include "helpers.h"
#include <arpa/inet.h>    // htons()
#include <cstdio>         // printf()
#include <cstdlib>        // atoi()
#include <cstring>        // strlen()
#include <sys/socket.h>   // socket(), bind(), listen(), accept(), recv(), send()
#include <unistd.h>       // close()
#include <ctime>          // clock()

static const int MAX_MESSAGE_SIZE = 256;
static const int DATA_CHUNK_SIZE = 1000;  // 1000 bytes per chunk
static const char *RESPONSE_ACK = "ACK";

int Server::handle_connection(int connectionfd) {
    char buffer[DATA_CHUNK_SIZE] = {};
    int total_bytes_received = 0;
    clock_t start_time = clock();
    int bytes_received;

    // (1) Receive data in chunks of 1000 bytes
    while ((bytes_received = static_cast<int>(recv(connectionfd, buffer, DATA_CHUNK_SIZE, 0))) > 0) {
        total_bytes_received += bytes_received;
        printf("Received %d bytes\n", bytes_received);
    }

    // (2) Handle connection closure (FIN message)
    if (bytes_received == 0) {
        printf("Connection closed by client\n");
    } else if (bytes_received < 0) {
        perror("Error receiving data");
        close(connectionfd);
        return -1;
    }

    // (3) Send acknowledgment to client
    if (send(connectionfd, RESPONSE_ACK, strlen(RESPONSE_ACK), 0) == -1) {
        perror("Error sending acknowledgment");
        close(connectionfd);
        return -1;
    }

    // (4) Calculate the time taken and the transfer rate
    clock_t end_time = clock();
    double time_elapsed = double(end_time - start_time) / CLOCKS_PER_SEC;
    double rate_mbps = (total_bytes_received * 8.0) / (time_elapsed * 1000000.0); // bits per second to Mbps
    int total_kb = total_bytes_received / 1000;

    // (5) Print the summary
    printf("Received=%d KB, Rate=%.3f Mbps\n", total_kb, rate_mbps);

    // (6) Close the connection
    close(connectionfd);
    return 0;
}

int Server::run_server() {
    // (1) Create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Error creating socket");
        return -1;
    }

    // (2) Set the "reuse port" option
    int yes = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("Error setting socket options");
        close(sock);
        return -1;
    }

    // (3) Bind the socket to the port
    struct sockaddr_in server_addr;
    if (make_server_sockaddr(&server_addr, listen_port) == -1) {
        printf("Error: port number must be in the range of [1024, 65535]\n");
        close(sock);
        return -1;
    }
    if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding socket");
        close(sock);
        return -1;
    }

    // (4) Listen for incoming connections
    if (listen(sock, queue_size) == -1) {
        perror("Error listening on socket");
        close(sock);
        return -1;
    }
    printf("Listening on port %d...\n", listen_port);

    // (5) Accept and handle a single connection
    int connectionfd = accept(sock, NULL, NULL);
    if (connectionfd == -1) {
        perror("Error accepting connection");
        close(sock);
        return -1;
    }
    printf("Connection accepted\n");

    // (6) Handle the connection
    handle_connection(connectionfd);

    // (7) Close the server socket
    close(sock);
    return 0;
}
