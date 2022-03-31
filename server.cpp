#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include "data.hpp"
#include "support.hpp"


enum {
        MaxBacklog = 50
};


class Server {
public:
    Server(int port, const std::string& addr, const std::string& path): port_(port), addr_(addr), path_(path) {}

    bool Impl() {
        return InitImpl();
    }

    void Start() {
        StartImpl();
    }

    bool Started() {
        return started_;
    }

private:
    bool InitImpl() {
        socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in sockaddr_ = {AF_INET, htons(port_), inet_addr(addr_.c_str())};
        if (0 != bind(socket_fd_,
            (struct sockaddr*)&(sockaddr_),
            sizeof(struct sockaddr_in))) {
            std::cerr << "bad binding\n";
            return started_ = false;
        }
        if (0 != listen(socket_fd_, MaxBacklog)) {
            std::cerr << "bad listening\n";
            return started_ = false;
        }
        epoll_fd_ = epoll_create1(0);
        {
            struct epoll_event event = {
                    .events = EPOLLIN | EPOLLERR | EPOLLHUP,
                    .data = {.fd = socket_fd_}
            };
            if (0 != epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket_fd_, &event)) {
                std::cerr << "bad epoll_ctl setting\n";
                return started_ = false;
            }
        }
        return started_ = true;
    }

    void StartImpl() {
        int count;
        InitConnection();
        InitData();

        while (true) {
            struct epoll_event event;

            int timeout_tries = 100;
            while ((epoll_wait(epoll_fd_, &event, 1, 1000) <= 0) && timeout_tries > 0) {
                --timeout_tries;
            }
            if (timeout_tries == 0) {
                std::cerr << "timeout connection\n";
                assert(false);
            }
            if (event.data.fd == socket_fd_) {
                if (event.events & EPOLLIN) {
                    int tries_attempts = 10;
                    while (AcceptNewClient() != 0 && tries_attempts > 0) {
                        --tries_attempts;
                    }
                    if (tries_attempts == 0) {
                        std::cerr << "error in adding client\n";
                        assert(false);
                    }
                }
            } else {
                if (event.events & EPOLLIN) {
                    int tries_attempts = 10;
                    while (ProcessClient(event.data.fd) <= 0 && tries_attempts > 0) {
                        --tries_attempts;
                    }
                    if (tries_attempts == 0) {
                        std::cerr << "error in ProcessInitConnection\n";
                        assert(false);
                    }
                }
            }
        }
    }

    void InitConnection() {
        struct epoll_event event;

        int timeout_tries = 100;
        while ((epoll_wait(epoll_fd_, &event, 1, 1000) <= 0) && timeout_tries > 0) {
            --timeout_tries;
        }
        if (timeout_tries == 0) {
            std::cerr << "timeout connection\n";
            assert(false);
        }
        if (event.data.fd == socket_fd_) {
            int tries_attempts = 10;
            while (AcceptNewClient() != 0 && tries_attempts > 0) {
                --tries_attempts;
            }
            if (tries_attempts == 0) {
                std::cerr << "error in adding client\n";
                assert(false);
            }
        }
    }

    void InitData() {
        struct epoll_event event;
        int tries_attempts = 10;
        while (event.data.fd == socket_fd_ && tries_attempts > 0) {
            int timeout_tries = 100;
            while ((epoll_wait(epoll_fd_, &event, 1, 1000) <= 0) && timeout_tries > 0) {
                --timeout_tries;
            }
            if (timeout_tries == 0) {
                std::cerr << "timeout connection\n";
                assert(false);
            }
            --tries_attempts;
        }
        tries_attempts = 10;
        if (event.events & EPOLLIN) {
            while (ProcessInitConnection(event.data.fd) != 0 && tries_attempts > 0) {
                --tries_attempts;
            }
            if (tries_attempts == 0) {
                std::cerr << "error in adding client\n";
                assert(false);
            }
        }
    }

    int AcceptNewClient() {
        int fd = accept(socket_fd_, NULL, NULL);
//        support::make_non_blocking(fd);
        struct epoll_event new_event = {
                .events = EPOLLIN | EPOLLERR | EPOLLHUP,
                .data = {.fd = fd}
        };
        return epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &new_event);
    }

    int ProcessInitConnection(int fd) {
        if (read(fd, &transfer_data_, sizeof(transfer_data_)) <= 0) {
            return -1;
        }
        file_fd_ = open(path_.c_str(), O_CREAT | O_RDWR, 0664);
        ftruncate(file_fd_, transfer_data_.size);
        struct stat file_stat;
        if (0 != fstat(file_fd_, &file_stat)) {
            std::cerr << "bad file\n";
        }
        content_ = (char*)mmap(nullptr, file_stat.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, file_fd_, 0);
        return 0;
    }

    bool ProcessClient(int fd) {
        ssize_t remaining_size = transfer_data_.each_size;
        ssize_t read_size;
        char buf[support::PageSize];
        auto data_ptr = (data::TransferData*)buf;
        while (remaining_size > 0) {
            int try_attempts = 10;
            while (0 > (read_size = read(fd, buf, support::PageSize))) {
                --try_attempts;
            }
            if (try_attempts == 0) {
                std::cerr << "bad writing to server\n";
                assert(false);
            }
            std::cout  << data_ptr->offset << " " << data_ptr->size << " " << data::TransferDataSize << "\n";
            write(file_fd_, buf + data::TransferDataSize, data_ptr->size - data::TransferDataSize);
//            memcpy(content_ + data_ptr->offset, buf + data::TransferDataSize, data_ptr->size - data::TransferDataSize);
            remaining_size -= read_size - data::TransferDataSize;
        }
        assert(false);
    }

private:
    bool started_ = false;
    int port_;
    int socket_fd_;
    int epoll_fd_;
    std::string addr_;
    std::string path_;
    data::InitTransferData transfer_data_;
    char* content_;
    int file_fd_;
};