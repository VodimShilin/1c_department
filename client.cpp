#include "data.hpp"
#include "support.hpp"

#include <string>
//#include <thread>
#include <vector>

class Client {
public:
    Client(int port, const std::string& addr, const std::string& path, int thread_count): port_(port), thread_count_(thread_count), addr_(addr), path_(path) {}

    void Start() {
        StartImpl();
    }

private:
    void StartImpl() {
        int fd = open(path_.c_str(), O_RDONLY);
        struct stat file_stat;
        if (0 != fstat(fd, &file_stat)) {
            std::cerr << "error with fstat\n";
            assert(false);
        }
        transfer_data_ = {file_stat.st_size, thread_count_, (file_stat.st_size - 1) / thread_count_ + 1};

        content_ = (char*)mmap(nullptr, file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

        int socket_fd = InitConnection();
//        TransferConnection(content_ + 0 * transfer_data_.each_size);
        exe::tp::ThreadPool pool(thread_count_);
        for (int i = 0; i < thread_count_; ++i) {
            pool.Submit([&]() {
                TransferConnection(content_ + i * transfer_data_.each_size);
            });
        }
        pool.WaitIdle();
        pool.Stop();
    }

    int InitConnection() {
        int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
//        support::make_non_blocking(socket_fd);
        struct sockaddr_in sockaddr_ = {AF_INET, htons(port_), inet_addr(addr_.c_str())};
        if (0 != connect(socket_fd, (struct sockaddr*)&(sockaddr_), sizeof(struct sockaddr_in))) {
            std::cerr << "error with connect in InitConnection\n";
            assert(false);
        }

        write(socket_fd, &transfer_data_, sizeof(transfer_data_));
        return socket_fd;
    }

    void TransferConnection(char* content) {
        int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
//        support::make_non_blocking(socket_fd);
        struct sockaddr_in sockaddr_ = {AF_INET, htons(port_), inet_addr(addr_.c_str())};
        if (0 != connect(socket_fd, (sockaddr*)&(sockaddr_), sizeof(struct sockaddr_in))) {
            std::cerr << "error with connect in InitConnection\n";
            assert(false);
        }
        ssize_t remaining_size = transfer_data_.each_size;
        ssize_t read_size;
        char buf[support::PageSize];
        while (remaining_size > 0) {
            int try_attempts = 10;
            auto cur = new (buf) data::TransferData({remaining_size + sizeof(data::TransferData) >
                                                                     support::PageSize ? support::PageSize : remaining_size + data::TransferDataSize,
                                                     content - content_});
            memcpy(buf + data::TransferDataSize, content, cur->size - data::TransferDataSize);
            while (0 > (read_size = write(socket_fd, buf, cur->size))) {
                --try_attempts;
            }
            if (try_attempts == 0) {
                std::cerr << "bad writing to server\n";
                assert(false);
            }
            remaining_size -= read_size - data::TransferDataSize;
            content += read_size - data::TransferDataSize;
        }
    }

private:
    int port_;
    int thread_count_;
    data::InitTransferData transfer_data_;
    std::string addr_;
    std::string path_;
    char* content_;
};