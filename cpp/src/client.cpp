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
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
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
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        spdlog::error("Error: failed to connect to the server");
        close(sockfd);
        return -1;
    }

    char send_byte = '\0';                     // 1-byte packet for RTT estimation
    char ack_byte;                             // buffer for 1-byte ACK
    const std::size_t CHUNK_SIZE = 80 * 1000;  // 80 KB
    char data_buf[CHUNK_SIZE];
    std::memset(data_buf, send_byte, CHUNK_SIZE);      // all zero bytes

    // --- RTT ESTIMATION PHASE ---
    long long total_rtt = 0;
    int rtt_measurements = 0;
    std::chrono::high_resolution_clock::time_point send_time;
    std::chrono::high_resolution_clock::time_point recv_time;

    for (int i = 0; i < 8; i++) {
        // send 1-byte packet
        send(sockfd, &send_byte, 1, 0);
        send_time = std::chrono::high_resolution_clock::now();

        // wait for 1-byte ACK
        int bytes = recv(sockfd, &ack_byte, 1, 0);
        if (bytes <= 0) {
            spdlog::error("Server closed connection during RTT estimation.");
            close(sockfd);
            return -1;
        }
        recv_time = std::chrono::high_resolution_clock::now();

        // RTT measured only for the last 4 packets
        if (i >= 4) {
            auto rtt = std::chrono::duration_cast<std::chrono::milliseconds>(recv_time - send_time).count();
            total_rtt += rtt;
            rtt_measurements++;
        }
    }

    long long avg_rtt = (rtt_measurements > 0)
                        ? total_rtt / rtt_measurements
                        : 0;

    // --- DATA TRANSFER PHASE ---
    long long total_bytes_sent = 0;
    auto start_time = std::chrono::high_resolution_clock::now();
    auto end_time = start_time + std::chrono::seconds(duration);

    int num_acks = 0;
    while (std::chrono::high_resolution_clock::now() < end_time) {
        auto start_time = std::chrono::high_resolution_clock::now();
        int bytes_sent = 0;
        while (bytes_sent < CHUNK_SIZE) {
            // send 80KB data chunk
            int sent = send(sockfd, data_buf + bytes_sent, CHUNK_SIZE - bytes_sent, 0);
            if (sent <= 0) {
                spdlog::error("Error sending data to server.");
                close(sockfd);
                return -1;
            }
            bytes_sent += sent;
        }
        total_bytes_sent += bytes_sent;

        // wait for 1-byte ACK before sending next chunk
        int bytes = recv(sockfd, &ack_byte, 1, 0);
        num_acks++;
        if (bytes <= 0) {
            spdlog::error("Server closed connection during data transfer.");
            break;
        }
    }

    // --- METRICS CALCULATION ---
    auto transfer_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = transfer_end - start_time;
    spdlog::debug("Data transfer time: {:.6f} seconds", elapsed.count());
    std::chrono::duration<double> transmission_delay = elapsed - std::chrono::duration<double>(num_acks * avg_rtt / 1000.0); // avg_rtt is in ms, convert to seconds
    spdlog::debug("Transmission delay (excluding RTT): {:.6f} seconds", transmission_delay.count());
    long long kb_sent = total_bytes_sent / 1000;
    double rate_mbps = (static_cast<double>(total_bytes_sent) * 8) / (1000000.0 * transmission_delay.count());

    // --- FINAL OUTPUT ---
    spdlog::info("Sent={} KB, Rate={:.3f} Mbps, RTT={}ms",
                 kb_sent, rate_mbps, avg_rtt);

    // (9) Close the socket
    close(sockfd);
    return 0;
}
