#include "client.h"
#include "helpers.h"
#include <sys/socket.h>  // socket(), connect(), send(), recv(), shutdown()
#include <unistd.h>      // close()
#include <stdio.h>       // printf(), perror()
#include <chrono>        // for time measurement
#include <cstring>       // memset()
#include <iostream>
#include <cassert>
#include <ctime>

// Constant size for data chunks
static const int CHUNK_SIZE = 1000;
static const char FIN_MESSAGE[] = "FIN";
static const char ACK_MESSAGE[] = "ACK";

// Constructor for the Client class
Client::Client(const std::string& hostname, int port, int duration)
    : hostname(hostname), port(port), duration(duration) {
        std::cout << "DEBUG: Called constructor for client.\n";
    }

// Function to send data to the server
int Client::send_data() {
    // (1) Create socket
    std::cout << "DEBUG: (1) Create socket\n";
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Error: failed to create socket");
        return -1;
    }

    // (2) Make sockaddr for the client connection
    std::cout << "DEBUG: (2) Make sockaddr for the client connection\n";
    struct sockaddr_in server_addr;
    if (make_client_sockaddr(&server_addr, hostname.c_str(), port) == -1) {
        printf("Error: Unable to make server sockaddr\n");
        return -1;
    }

    // (3) Connect to the server
    std::cout << "DEBUG: (3) Connect to the server\n";
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error: failed to connect to the server");
        close(sock);
        return -1;
    }

    // (4) Start sending data
    std::cout << "DEBUG: (4) Start sending data\n";
    char data[CHUNK_SIZE] = {};  // All zeroes
    int total_bytes_sent = 0;

    // Record the start time
    auto start = std::chrono::high_resolution_clock::now();

    // Send data for the specified duration
    //auto end_time = std::chrono::high_resolution_clock::now() + std::chrono::seconds(duration);
    time_t start_time = time(nullptr);  // Get current time in seconds
    // std::cout << "DEBUG: duration=" << duration << std::endl;
    // assert(start_time < end_time);
    while (difftime(time(nullptr), start_time) < duration) {
        time_t now = time(nullptr);  // Get current time
        double elapsed = difftime(now, start_time);  // Calculate elapsed time

        // Print the elapsed time in seconds
        std::cout << "DEBUG: Elapsed time: " << elapsed << " seconds" << std::endl;

        // Send 1000-byte chunks as fast as possible
        int bytes_sent = static_cast<int>(send(sock, data, CHUNK_SIZE, 0));
        if (bytes_sent == -1) {
            std::cout << "Error: failed to send data.\n";
            perror("Error: failed to send data");
            close(sock);
            return -1;
        }
        total_bytes_sent += bytes_sent;
        
        // Sleep for a short duration to prevent busy-waiting
        sleep(1);  // Sleep for 1 second between iterations
    }
    std::cout << "DEBUG: Finished timing loop." << std::endl;

    // (5) Send the FIN message and shutdown transmission
    std::cout << "DEBUG: (5) Send the FIN message and shutdown transmission\n";
    send(sock, FIN_MESSAGE, strlen(FIN_MESSAGE), 0);
    shutdown(sock, SHUT_WR);  // Stop sending data

    // (6) Await acknowledgment from the server
    std::cout << "DEBUG: (6) Await acknowledgment from the server\n";
    char buffer[256] = {};
    if (recv(sock, buffer, sizeof(buffer), 0) <= 0 || strcmp(buffer, ACK_MESSAGE) != 0) {
        printf("Error: failed to receive acknowledgment from server\n");
        close(sock);
        return -1;
    }

    // (7) Calculate the elapsed time
    std::cout << "DEBUG: (7) Calculate the elapsed time\n";
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    // (8) Print summary in the format: Sent=X KB, Rate=Y Mbps
    std::cout << "DEBUG: (8) Print summary in the format: Sent=X KB, Rate=Y Mbps\n";
    int kb_sent = total_bytes_sent / 1000;
    double rate_mbps = (total_bytes_sent * 8) / (1000000.0 * elapsed.count());

    printf("Sent=%d KB, Rate=%.3f Mbps\n", kb_sent, rate_mbps);

    // (9) Close the socket
    std::cout << "DEBUG: (9) Close the socket\n";
    close(sock);
    return 0;
}
