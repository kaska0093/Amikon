//
//  server.cpp
//  amikon
//
//  Created by Nikita Shestakov on 28.03.2025.
//

#include "server.hpp"
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <cstring>
#include <sys/wait.h>
#include <csignal>
#include <sstream>

#define PORT_BASE 8080
#define WORKER_COUNT 5
#define SHM_NAME "/monitor_log"



void logToSharedMemory(const std::string& message) {
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) return;
    ftruncate(fd, sizeof(LogEntry) * 100);

    void* ptr = mmap(0, sizeof(LogEntry) * 100, PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) return;

    static int index = 0;
    LogEntry* log = static_cast<LogEntry*>(ptr);
    snprintf(log[index].entry, 256, "%s", message.c_str());

    index = (index + 1) % 100;
    munmap(ptr, sizeof(LogEntry) * 100);
    close(fd);
}

void workerProcess(int port) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (struct sockaddr*)&addr, sizeof(addr));

    while (true) {
        char buffer[64];
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);

        recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddr, &clientLen);
        buffer[63] = '\0';

        int pid = atoi(buffer);
        std::ostringstream logEntry;

        if (pid > 0) {
            std::ostringstream cmd;
            cmd << "ps -p " << pid << " -o %cpu | tail -1";
            FILE* pipe = popen(cmd.str().c_str(), "r");

            if (!pipe) {
                logEntry << "Error executing command";
            } else {
                char cpuUsage[16];
                fgets(cpuUsage, sizeof(cpuUsage), pipe);
                pclose(pipe);

                std::string response = cpuUsage;
                response.erase(response.find_last_not_of(" \n\r\t") + 1);
                sendto(sock, response.c_str(), response.size(), 0, (struct sockaddr*)&clientAddr, clientLen);

                logEntry << "PID " << pid << " CPU " << response;
            }
        } else {
            sendto(sock, "invalid", 7, 0, (struct sockaddr*)&clientAddr, clientLen);
            logEntry << "invalid request";
        }

        logToSharedMemory(logEntry.str());
    }
}

int main() {
    for (int i = 0; i < WORKER_COUNT; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            workerProcess(PORT_BASE + i);
            return 0;
        }
    }

    while (wait(nullptr) > 0);
    shm_unlink(SHM_NAME);
    return 0;
}

