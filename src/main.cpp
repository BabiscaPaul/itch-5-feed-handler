#include "mmap_reader.h"
#include "messages.h"
#include <format>
#include <iostream>

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
    size_t offset{0};
    int add_orders_printed{0};

    while (offset < size && add_orders_printed < 5) {
        uint16_t msg_len = read_u16_be(data + offset);
        offset += 2;

        MessageHeader header{data + offset};

        if (header.type() == 'A') {
            AddOrder msg{data + offset};

            Timestamp ts{msg.timestamp()};
            uint64_t total_secs{ts / 1'000'000'000};
            uint64_t nanos{ts % 1'000'000'000};
            uint64_t hours{total_secs / 3600};
            uint64_t mins{(total_secs % 3600) / 60};
            uint64_t secs{total_secs % 60};

            std::cout << std::format("Add Order #{}:\n", add_orders_printed + 1);
            std::cout << std::format("  Stock:     {}\n", msg.stock());
            std::cout << std::format("  Side:      {} ({})\n", msg.side(), msg.side() == 'B' ? "Buy" : "Sell");
            std::cout << std::format("  Shares:    {}\n", msg.shares());
            std::cout << std::format("  Price:     ${:.4f}\n", msg.price() / 10000.0);
            std::cout << std::format("  Order Ref: {}\n", msg.order_ref());
            std::cout << std::format("  Time:      {:02}:{:02}:{:02}.{:09}\n", hours, mins, secs, nanos);
            std::cout << std::format("  Locate:    {}\n\n", msg.stock_locate());

            add_orders_printed++;
        }

        offset += msg_len;
    }

    return 0;
}