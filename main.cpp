#include <iostream>
#include <sys/mman.h>
#include <sys/wait.h>
#include "server.cpp"
#include "client.cpp"


void validating(const std::string& path, const std::string& addr) {}

int main(int argc, char* argv[]) {
    std::string path = argv[1];
    const int thread_count = (int)strtol(argv[2], NULL, 10);
    const int port = argc >= 4 ? (int)strtol(argv[3], NULL, 10) : 1234; // default
    std::string addr = argc >= 5 ? argv[4] : "127.0.0.1";
    validating(path, addr);

    char* global_memory = (char*)mmap(NULL, support::PageSize, PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    global_memory += sizeof(Server);

    auto server_ptr = new (global_memory) Server(port, addr, path);
//    Server server = {1234, "127.0.0.1", socket_fd};
    if (!server_ptr->Impl()) {
        std::cerr << "server not started\n";
    }

    int pid_server = fork();
    if (pid_server == 0) {
        server_ptr->Start();
    }

    int pid_client = fork();
    if (pid_server == 0) {
        Client client = Client(port, addr, path, thread_count);
    }
    waitpid(pid_client, NULL, NULL);

    waitpid(pid_server, NULL, NULL);
    return 0;
}
