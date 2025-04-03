//
//  logger.cpp
//  amikon
//
//  Created by Nikita Shestakov on 28.03.2025.
//

#include "logger.hpp"
#include "server.hpp"
#include <iostream>
#include <fstream>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#define SHM_NAME "/monitor_log"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <output_file>" << endl;
        return 1;
    }

    int fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (fd == -1) {
        cerr << "Error: cannot access shared memory" << endl;
        return 1;
    }

    LogEntry* log = (LogEntry*)mmap(0, sizeof(LogEntry) * 100, PROT_READ, MAP_SHARED, fd, 0);
    if (log == MAP_FAILED) return 1;

    ofstream outFile(argv[1]);
    
    for (int i = 0; i < 100; i++) {
        outFile << log[i].entry << endl;
    }

    outFile.close();
    munmap(log, sizeof(LogEntry) * 100);
    close(fd);

    return 0;
}
