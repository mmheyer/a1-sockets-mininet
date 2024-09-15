#ifndef CLIENT_H
#define CLIENT_H

#include <string>

class Client {
public:
    Client(const std::string& hostname, int port, int duration);
    int send_data();

private:
    std::string hostname;
    int port;
    int duration;
};

#endif
