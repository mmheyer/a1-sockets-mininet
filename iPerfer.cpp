#include <arpa/inet.h> // htons(), ntohs()
#include <netdb.h> // gethostbyname(), struct hostent
#include <netinet/in.h> // struct sockaddr_in
#include <stdio.h> // perror(), fprintf()
#include <string.h> // memcpy()
#include <sys/socket.h> // getsockname()
#include <unistd.h> // stderr
#include <time.h> // time(&time_t)
#include <iostream>
#include <getopt.h>

#include "Server.h"
#include "Client.h"

using namespace std;

int main(int argc, char* argv[]) {
    bool server_mode = false;
    bool client_mode = false;
    int port = -1;
    char* hostname = nullptr;
    int time = -1;

    // Parse command line options
    int opt;
    while ((opt = getopt(argc, argv, "s:p:h:t:")) != -1) {
        switch (opt) {
            case 's':  // Server mode flag
                server_mode = true;
                break;
            case 'c':  // Client mode flag
                client_mode = true;
                break;
            case 'p':  // Port number
                port = std::atoi(optarg);
                break;
            case 'h':  // Hostname (client mode)
                hostname = optarg;
                break;
            case 't':  // Time (client mode)
                time = std::atoi(optarg);
                break;
            default:
                std::cerr << "Usage: ./iPerfer -s -p <port>       # Server mode\n";
                std::cerr << "       ./iPerfer -c -h <hostname> -p <port> -t <time> # Client mode\n";
                return 1;
        }
    }

    // Validate arguments based on the mode
    if (server_mode && client_mode) {
        std::cerr << "Error: Cannot specify both server and client modes.\n";
        return 1;
    }

    if (server_mode) {
        // Server mode checks
        if (argc != 4 || strcmp(argv[1], "-s") != 0 || strcmp(argv[2], "-p") != 0) {
            printf("Error: missing or extra arguments\n");
            return 1;
        }
        if (port == -1) {
            std::cerr << "Error: Port number is required for server mode.\n";
            std::cerr << "Usage: ./iPerfer -s -p <port>\n";
            return 1;
        }
        if (port < 1024 || port > 65535) {
            printf("Error: port number must be in the range of [1024, 65535]\n");
            return 1;
        }
        std::cout << "Running in server mode on port " << port << "\n";
        // Initialize and run server
        Server server(port, 1); 
        return server.run_server();

    } else if (client_mode) {
        // Client mode checks
        if (argc != 7 || strcmp(argv[1], "-c") != 0 || strcmp(argv[3], "-p") != 0 || strcmp(argv[5], "-t") != 0) {
            printf("Error: missing or extra arguments\n");
            return 1;
        }
        if (hostname == nullptr || port == -1 || time == -1) {
            std::cerr << "Error: Hostname, port, and time are required for client mode.\n";
            std::cerr << "Usage: ./iPerfer -c -h <hostname> -p <port> -t <time>\n";
            return 1;
        }
        if (port < 1024 || port > 65535) {
            printf("Error: port number must be in the range of [1024, 65535]\n");
            return 1;
        }
        if (time <= 0) {
            printf("Error: time argument must be greater than 0\n");
            return 1;
        }
        std::cout << "Running in client mode\n";
        std::cout << "Server hostname: " << hostname << "\n";
        std::cout << "Server port: " << port << "\n";
        std::cout << "Time: " << time << " seconds\n";
        // Initialize and run client 
        Client client(hostname, port, time);
        return client.send_data();

    } else {
        std::cerr << "Error: You must specify either server or client mode.\n";
        std::cerr << "Usage: ./iPerfer -s -p <port>       # Server mode\n";
        std::cerr << "       ./iPerfer -c -h <hostname> -p <port> -t <time> # Client mode\n";
        return 1;
    }

    return 0;
}