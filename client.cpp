//
//  client.cpp
//  amikon
//
//  Created by Nikita Shestakov on 28.03.2025.
//

#include "client.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <arpa/inet.h>

using namespace std;

void sendRequest(const string& ip, int port, int pid) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    string request = to_string(pid);
    sendto(sock, request.c_str(), request.size(), 0, (struct sockaddr*)&addr, sizeof(addr));

    char buffer[64];
    recv(sock, buffer, sizeof(buffer), 0);
    buffer[63] = '\0';

    cout << "Response: " << buffer << endl;
    close(sock);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <server_ip> <port> <pid>" << endl;
        return 1;
    }

    sendRequest(argv[1], atoi(argv[2]), atoi(argv[3]));
    return 0;
}
