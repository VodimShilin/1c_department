#pragma once

namespace data {
    struct InitTransferData {
        long size;
        long n;
        long each_size;
    };

    struct TransferData {
        long size;
        long offset;
    };

    enum {
        TransferDataSize = (long)sizeof(TransferData)
    };
}