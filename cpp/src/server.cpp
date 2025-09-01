#include "server.h"
#include "helpers.h"
#include <arpa/inet.h>    // htons()
#include <cstdio>         // printf()
#include <cstdlib>        // atoi()
#include <cstring>        // strlen()
#include <sys/socket.h>   // socket(), bind(), listen(), accept(), recv(), send()
#include <unistd.h>       // close()
#include <ctime>          // clock()
#include <iostream>
#include <chrono>        // for time measurement
#include "spdlog/spdlog.h"

static const int MAX_MESSAGE_SIZE = 256;
static const int DATA_CHUNK_SIZE = 80000;  // 80 KB per chunk
// static const char *RESPONSE_ACK = "ACK";
static const int NUM_PACKETS = 8;
static const char EXPECTED_CHAR = 'M';
static const char ACK_CHAR = 'A';
static char recv_buf[1];
static char ack_buf[1] = {ACK_CHAR};

int Server::handle_connection(int connectionfd) {
    std::chrono::high_resolution_clock::time_point send_time;
    std::chrono::high_resolution_clock::time_point recv_time;
    long long total_rtt = 0.0;

    // For each packet, send ACK and measure RTT on next packet
    for (int i = 0; i < NUM_PACKETS; ++i) {
        // receive packet from the client
        if (recv(connectionfd, recv_buf, 1, 0) != 1) {
            return -1;
        }
        if (recv_buf[0] != EXPECTED_CHAR) {
            return -1;
        }
        recv_time = std::chrono::high_resolution_clock::now();

        // compute RTT for packets 4-8
        if (i >= 4) {
            auto rtt = std::chrono::duration_cast<std::chrono::milliseconds>(recv_time - send_time).count();
            spdlog::debug("rtt for packet {}: {} ms", i + 1, rtt);
            total_rtt += rtt;
        }
        
        // Send ACK to client
        send_time = std::chrono::high_resolution_clock::now();
        if (send(connectionfd, ack_buf, 1, 0) != 1) {
            return -1;
        }
    }

    // Compute average RTT
    spdlog::debug("Total RTT for packets 4-8: {}", total_rtt);
    long long avg_rtt = total_rtt / 4;

    char buffer[DATA_CHUNK_SIZE] = {};
    long long total_bytes_received = 0;
    auto start_time = std::chrono::high_resolution_clock::now();
    long long bytes_received;

    // (1) Receive data in chunks of 80 KB
    while ((bytes_received = static_cast<long long>(recv(connectionfd, buffer, DATA_CHUNK_SIZE, 0))) > 0) {
        total_bytes_received += bytes_received;
        
        // send ACK for every packet received
        if (send(connectionfd, ack_buf, 1, 0) != 1) {
            spdlog::error("Error sending ACK to client");
            return -1;
        }
    }

    // (4) Calculate the time taken and the transfer rate
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time_elapsed = end_time - start_time;
    double rate_mbps = (static_cast<double>(total_bytes_received) * 8.0) / (time_elapsed.count() * 1000000.0); // bits per second to Mbps
    long long total_kb = total_bytes_received / 1000;

    // (5) Print the summary
    printf("Received=%lld KB, Rate=%.3f Mbps, RTT=%lldms\n", total_kb, rate_mbps, avg_rtt);

    // (6) Close the connection
    close(connectionfd);
    return 0;
}

int Server::run_server() {
    // (1) Create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        spdlog::error("Error creating socket");
        return -1;
    }

    // (2) Set the "reuse port" option
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        spdlog::error("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // (3) Bind the socket to the port
    struct sockaddr_in server_addr;
    if (make_server_sockaddr(&server_addr, listen_port) == -1) {
        printf("Error: port number must be in the range of [1024, 65535]\n");
        close(sock);
        return -1;
    }
    if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        spdlog::error("Error binding socket");
        close(sock);
        return -1;
    }

    // (4) Listen for incoming connections
    if (listen(sock, queue_size) == -1) {
        spdlog::error("Error listening on socket");
        close(sock);
        return -1;
    }
    spdlog::info("iPerfer server started");

    // (5) Accept and handle a single connection
    int connectionfd = accept(sock, NULL, NULL);
    if (connectionfd == -1) {
        spdlog::error("Error accepting connection");
        close(sock);
        return -1;
    }
    spdlog::info("Client connected");

    // (6) Handle the connection
    if (handle_connection(connectionfd) == -1) {
        spdlog::error("Error handling connection");
        close(sock);
        return -1;
    }

    // (7) Close the server socket
    close(sock);
    return 0;
}
