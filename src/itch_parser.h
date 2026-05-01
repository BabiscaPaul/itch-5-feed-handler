#pragma once

#include <iostream>
#include <array>
#include <cstddef>
#include <cstdint>
#include <chrono>
#include <format>
#include "messages.h"

inline void parse_and_count(const uint8_t* data, size_t size) {
    std::array<uint64_t, 256> count{};
    size_t offset{0};
    auto start = std::chrono::steady_clock::now();

    while (offset < size) {
        uint16_t len = read_u16_be(data + offset);
        unsigned char type = data[offset + 2];
        count[type]++;
        offset += 2 + len;
    }

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    uint64_t total = 0;
    for (uint64_t c : count) total += c;

    for (int i = 0; i < 256; ++i) {
        if (count[i]) std::cout << static_cast<char>(i) << ": " << count[i] << '\n';
    }

    std::cout << std::format("\nTotal: {}\n", total);
    std::cout << std::format("Time: {:.2f}s ({:.1f}M msgs/sec)\n",
                            elapsed.count(),
                            total / elapsed.count() / 1e6);
}