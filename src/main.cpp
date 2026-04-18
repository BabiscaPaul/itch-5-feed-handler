#include "mmap_reader.h"
#include <cstdio>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::printf("Usage: %s <itch-file>\n", argv[0]);
        return 1;
    }
    MmapReader reader{argv[1]};
    if (!reader.is_valid()) {
        return 1;
    }

    std::printf("File size: %zu bytes (%.2f GB)\n",
                reader.size(), reader.size() / 1e9);

    return 0;
}