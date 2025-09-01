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
#include "spdlog/spdlog.h"
#include "cxxopts.hpp"

#include "server.h"
#include "client.h"

using namespace std;

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::debug); 

    cxxopts::Options options("iPerfer", "A simple network performance measurement tool");
    options.add_options()
        ("s, server", "Enable server", cxxopts::value<bool>())
        ("p, port", "Port number to use", cxxopts::value<int>())
        ("c, client", "Enable client", cxxopts::value<bool>()->default_value("false"))
        ("h, host", "Server hostname (required in client mode)", cxxopts::value<std::string>())
        ("t, time", "Duration in seconds (required in client mode)", cxxopts::value<int>()->default_value("10"))
        ("help", "Print help");

    auto result = options.parse(argc, argv);

    auto is_server = result["server"].as<bool>();
    auto port = result["port"].as<int>();

    spdlog::debug("About to check port number...");
    if (port < 1024 || port > 0xFFFF) {
      spdlog::error("Error: port number must be in the range of [1024, 65535]\n");
      exit(1);
    }

    spdlog::info("Setup complete! Server mode: {}. Listening/sending to port {}", is_server, port);

    if (is_server) {
        // Server mode checks
        if (argc != 4) {
            spdlog::error("Missing or extra arguments");
            exit(1);
        }
        // if (port < 1024 || port > 65535) {
        //     std::cout << "Error: port number must be in the range of [1024, 65535]\n";
        //     exit(1);
        // }
        // Initialize and run server
        Server server(port, 1);
        return server.run_server();

    } else {
        // // Client mode checks
        // if (argc != 8) {
        //     std::cout << "Error: missing or extra arguments\n";
        //     exit(1);
        // }
        // if (port < 1024 || port > 65535) {
        //     std::cout << "Error: port number must be in the range of [1024, 65535]\n";
        //     exit(1);
        // }
        // Get the server hostname
        if (!result.count("host")) {
            spdlog::error("Error: missing hostname\n");
            exit(1);
        }
        auto hostname = result["host"].as<std::string>();

        // Get the time duration
        auto time = result["time"].as<int>();
        if (time <= 0) {
            // std::cout << "Error: time argument must be greater than 0\n";
            spdlog::error("Error: time argument must be greater than 0\n");
            exit(1);
        }
        // Initialize and run client
        Client client(hostname, port, time);
        return client.send_data();

    }

    return 0;
}