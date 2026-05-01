#include "mmap_reader.h"
#include <format>
#include <iostream>
#include "itch_parser.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << std::format("Usage: {} <itch-file>\n", argv[0]);
        return 1;
    }

    MmapReader reader{argv[1]};

    if (!reader.is_valid()) {
        return 1;
    }

    std::cout << std::format("File size: {} bytes ({:.2f} GB)\n\n",
                            reader.size(), reader.size() / 1e9);

    const uint8_t* data{reader.data()};
    size_t size{reader.size()};
    
    parse_and_count(data, size);

    return 0;
}