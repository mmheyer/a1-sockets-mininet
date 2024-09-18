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

#include "server.h"
#include "client.h"

using namespace std;

int main(int argc, char* argv[]) {
    bool server_mode = false;
    bool client_mode = false;
    int port = -1;
    char* hostname = nullptr;
    int time = -1;

    // check # command line args
    if (argc != 4 && argc != 8) {
        std::cout << "Error: missing or extra arguments\n";
        exit(1);
    }

    // Parse command line options
    int opt;
    while ((opt = getopt(argc, argv, "scp:h:t:")) != -1) {
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

    if (server_mode) {
        // Server mode checks
        if (argc != 4) {
            std::cout << "Error: missing or extra arguments\n";
            exit(1);
        }
        if (port < 1024 || port > 65535) {
            std::cout << "Error: port number must be in the range of [1024, 65535]\n";
            exit(1);
        }
        // Initialize and run server
        Server server(port, 1); 
        return server.run_server();

    } else if (client_mode) {
        // Client mode checks
        if (argc != 8) {
            std::cout << "Error: missing or extra arguments\n";
            exit(1);
        }
        if (port < 1024 || port > 65535) {
            std::cout << "Error: port number must be in the range of [1024, 65535]\n";
            exit(1);
        }
        if (time <= 0) {
            std::cout << "Error: time argument must be greater than 0\n";
            exit(1);
        }
        // Initialize and run client 
        Client client(hostname, port, time);
        return client.send_data();

    }

    return 0;
}