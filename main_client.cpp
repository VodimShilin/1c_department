#include <iostream>
#include <sys/mman.h>
#include <sys/wait.h>
#include "server.cpp"
#include "client.cpp"

enum {
    PageSize = 4096
};

void validating(const std::string& path, const std::string& addr) {}

int main(int argc, char* argv[]) {
    std::string path = argv[1];
    const int thread_count = (int) strtol(argv[2], NULL, 10);
    const int port = argc >= 4 ? (int) strtol(argv[3], NULL, 10) : 1234;// default
    std::string addr = argc >= 5 ? argv[4] : "127.0.0.1";
    validating(path, addr);
    Client client = Client(port, addr, path, thread_count);
    client.Start();
}