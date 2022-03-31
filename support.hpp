#pragma once

#include <sys/socket.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cassert>
#include <functional>
#include <cstring>
#include "tp/thread_pool.hpp"

namespace support {
    enum {
        PageSize = 4096l
    };

    void make_non_blocking(int fd) {
        int flags = fcntl(fd, F_GETFL);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    using Task = wheels::UniqueFunction<void()>;
}