#include "client.h"
#include "helpers.h"
#include <sys/socket.h>  // socket(), connect(), send(), recv(), shutdown()
#include <unistd.h>      // close()
#include <chrono>        // for time measurement
#include <cstring>       // memset()
#include <iostream>
#include <cassert>
#include <ctime>
#include <iomanip>  // for std::put_time
#include "spdlog/spdlog.h"

// Constant size for data chunks
static const int CHUNK_SIZE = 80 * 1000; // 80 KB
static const char FIN_MESSAGE[] = "FIN";
static const char ACK_MESSAGE[] = "ACK";

// Constructor for the Client class
Client::Client(const std::string& hostname, int port, int duration)
    : hostname(hostname), port(port), duration(duration) {}

// Function to send data to the server
int Client::send_data() {
    // (1) Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        spdlog::error("Error: failed to create socket");
        return -1;
    }

    // (2) Make sockaddr for the client connection
    struct sockaddr_in server_addr;
    if (make_client_sockaddr(&server_addr, hostname.c_str(), port) == -1) {
        printf("Error: Unable to make server sockaddr\n");
        return -1;
    }

    // (3) Connect to the server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        spdlog::error("Error: failed to connect to the server");
        close(sock);
        return -1;
    }

    // Send eight 1-byte packets to measure RTT, waiting for an ACK after each
    const int RTT_PACKETS = 8;
    char ping_data[1] = {'M'};
    char ack_buf[1];
    long long total_rtt = 0;
    for (int i = 0; i < RTT_PACKETS; ++i) {
        auto rtt_start = std::chrono::high_resolution_clock::now();
        if (send(sock, ping_data, 1, 0) != 1) {
            spdlog::error("Error: failed to send ping data");
            close(sock);
            return -1;
        }
        if (recv(sock, ack_buf, 1, 0) != 1) {
            spdlog::error("Error: failed to receive ACK");
            close(sock);
            return -1;
        }
        auto rtt_end = std::chrono::high_resolution_clock::now();
        if (i >= 4) {
            auto rtt = std::chrono::duration_cast<std::chrono::milliseconds>(rtt_end - rtt_start).count();
            total_rtt += rtt;
        }
    }
    spdlog::debug("Total RTT (last 4 packets): {} ms", total_rtt);
    long long avg_rtt = total_rtt / 4;
    spdlog::debug("Average RTT (last 4 packets): {} ms", avg_rtt);

    // (4) Start sending data
    char data[CHUNK_SIZE] = {};  // All zeroes
    long long total_bytes_sent = 0;

    // Send data for the specified duration
    auto start_time = std::chrono::high_resolution_clock::now();
    auto end_time = start_time + std::chrono::seconds(duration);

    while (std::chrono::high_resolution_clock::now() < end_time) {
        // Send 80 KB chunks as fast as possible
        long long bytes_sent = static_cast<long long>(send(sock, data, CHUNK_SIZE, 0));
        if (bytes_sent == -1) {
            spdlog::error("Error: failed to send data");
            close(sock);
            return -1;
        }
        total_bytes_sent += bytes_sent;

        // Wait for ACK from server
        if (recv(sock, ack_buf, 1, 0) != 1) {
            spdlog::error("Error: failed to receive ACK");
            close(sock);
            return -1;
        }
    }

    // (7) Calculate the elapsed time
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start_time;

    // (8) Print summary in the format: Sent=X KB, Rate=Y Mbps
    long long kb_sent = total_bytes_sent / 1000;
    double rate_mbps = (static_cast<double>(total_bytes_sent) * 8) / (1000000.0 * elapsed.count());

    printf("Sent=%lld KB, Rate=%.3f Mbps, RTT=%lldms\n", kb_sent, rate_mbps, avg_rtt);

    // (9) Close the socket
    close(sock);
    return 0;
}
