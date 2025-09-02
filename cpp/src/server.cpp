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

int Server::handle_connection(int connectionfd) {
    char recv_buf[1024];
    char ack = 'A';  // single-byte ACK
    const int SMALL_PKTS = 8;
    long long total_rtt = 0;
    int rtt_measurements = 0;

    std::chrono::high_resolution_clock::time_point send_time, recv_time;

    // --- First phase: 8 small packets ---
    for (int i = 0; i < SMALL_PKTS; i++) {
        // Receive 1-byte packet
        int bytes = recv(connectionfd, recv_buf, 1, 0);
        if (bytes <= 0) {
            spdlog::error("Client disconnected early\n");
            close(connectionfd);
            return -1;
        }

        // for the last 4 packets, measure RTT
        if (i >= 4) {
            recv_time = std::chrono::high_resolution_clock::now();
            auto rtt = duration_cast<std::chrono::microseconds>(recv_time - send_time).count();
            total_rtt += rtt;
            rtt_measurements++;
            spdlog::debug("RTT measurement {}: {} us", rtt_measurements, rtt);
        }

        // Send ACK for this packet
        send(connectionfd, &ack, 1, 0);
        send_time = std::chrono::high_resolution_clock::now();  // mark send time for next RTT
    }

    // Compute average RTT
    // double avg_rtt = static_cast<double>(total_rtt) / rtt_measurements;
    long long avg_rtt = total_rtt / rtt_measurements / 1000; // convert to ms
    spdlog::debug("Average RTT (server-side, 4 measurements): {} us\n", avg_rtt);

    // --- Second phase: large 80KB packets ---
    // const size_t LARGE_PKT_SIZE = 80 * 1024;
    const size_t LARGE_PKT_SIZE = 80 * 1000;
    char large_buf[LARGE_PKT_SIZE];

    std::chrono::high_resolution_clock::time_point start_time = std::chrono::high_resolution_clock::now();
    long long total_bytes = 0;
    while (true) {
        int bytes = recv(connectionfd, large_buf, LARGE_PKT_SIZE, 0);
        if (bytes <= 0) {
            spdlog::debug("No more data from client. Closing connection.\n");
            break;
        }
        total_bytes += bytes;

        // ACK each large packet (1 byte is enough)
        send(connectionfd, &ack, 1, 0);
        // spdlog::debug("Received large packet of {} bytes\n", bytes);
    }
    std::chrono::high_resolution_clock::time_point end_time = std::chrono::high_resolution_clock::now();

    // (4) Calculate metrics
    std::chrono::duration<double> elapsed = end_time - start_time;
    double rate_mbps = (static_cast<double>(total_bytes) * 8.0) / (elapsed.count() * 1000000.0); // bits per second to Mbps
    long long total_kb = total_bytes / 1000;

    // (5) Print the summary
    spdlog::info("Received={} KB, Rate={:.3f} Mbps, RTT={}ms\n", total_kb, rate_mbps, avg_rtt);

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
