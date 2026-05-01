#pragma once

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <string_view>

inline uint16_t read_big_endian(const uint8_t* p, std::integral_constant<int, 2>) {
    uint16_t raw;
    std::memcpy(&raw, p, 2);
    return __builtin_bswap16(raw);
}

inline uint32_t read_big_endian(const uint8_t* p, std::integral_constant<int, 4>) {
    uint32_t raw;
    std::memcpy(&raw, p, 4);
    return __builtin_bswap32(raw);
}

inline uint64_t read_big_endian(const uint8_t* p, std::integral_constant<int, 8>) {
    uint64_t raw;
    std::memcpy(&raw, p, 8);
    return __builtin_bswap64(raw);
}

inline uint64_t read_timestamp(const uint8_t* p) {
    uint64_t ts = 0;
    std::memcpy(reinterpret_cast<uint8_t*>(&ts) + 2, p, 6);
    return __builtin_bswap64(ts);
}

inline uint16_t read_u16_be(const uint8_t* p) {
    uint16_t raw;
    std::memcpy(&raw, p, 2);
    return __builtin_bswap16(raw);
}

inline std::string_view read_stock(const uint8_t* p) {
    const char* start{reinterpret_cast<const char*>(p)};
    size_t len{8};
    while (len > 0 && start[len - 1] == ' ') --len;
    return std::string_view{start, len};
}

struct MessageHeader {
    const uint8_t* data;

    char type()              const { return static_cast<char>(data[0]); }
    uint16_t stock_locate()  const { return read_big_endian(data + 1, std::integral_constant<int, 2>{}); }
    uint16_t tracking()      const { return read_big_endian(data + 3, std::integral_constant<int, 2>{}); }
    uint64_t timestamp()     const { return read_timestamp(data + 5); }
};

// Add Order — 'A' (36 bytes)
// A new order enters the book.
struct AddOrder {
    const uint8_t* data;

    uint16_t stock_locate()      const { return read_big_endian(data + 1, std::integral_constant<int, 2>{}); }
    uint64_t timestamp()         const { return read_timestamp(data + 5); }
    uint64_t order_ref()         const { return read_big_endian(data + 11, std::integral_constant<int, 8>{}); }
    char side()                  const { return static_cast<char>(data[19]); } // 'B' = buy, 'S' = sell
    uint32_t shares()            const { return read_big_endian(data + 20, std::integral_constant<int, 4>{}); }
    uint32_t price()             const { return read_big_endian(data + 32, std::integral_constant<int, 4>{}); }

    std::string_view stock()     const { return read_stock(data + 24); }
};

// Add Order with MPID — 'F' (40 bytes)
// Same as AddOrder but includes a 4-byte market participant ID at the end.
// We reuse the same offsets — the MPID is at bytes 36-39, which we ignore.
struct AddOrderMPID {
    const uint8_t* data;

    uint16_t stock_locate()      const { return read_big_endian(data + 1, std::integral_constant<int, 2>{}); }
    uint64_t timestamp()         const { return read_timestamp(data + 5); }
    uint64_t order_ref()         const { return read_big_endian(data + 11, std::integral_constant<int, 8>{}); }
    char side()                  const { return static_cast<char>(data[19]); }
    uint32_t shares()            const { return read_big_endian(data + 20, std::integral_constant<int, 4>{}); }
    uint32_t price()             const { return read_big_endian(data + 32, std::integral_constant<int, 4>{}); }

    std::string_view stock()     const { return read_stock(data + 24); }
};

// Order Executed — 'E' (31 bytes)
// Shares from an existing order were filled.
struct OrderExecuted {
    const uint8_t* data;

    uint16_t stock_locate()      const { return read_big_endian(data + 1, std::integral_constant<int, 2>{}); }
    uint64_t timestamp()         const { return read_timestamp(data + 5); }
    uint64_t order_ref()         const { return read_big_endian(data + 11, std::integral_constant<int, 8>{}); }
    uint32_t executed_shares()   const { return read_big_endian(data + 19, std::integral_constant<int, 4>{}); }
    uint64_t match_number()      const { return read_big_endian(data + 23, std::integral_constant<int, 8>{}); }
};

// Order Executed With Price — 'C' (36 bytes)
// Same as OrderExecuted but at a different price than the original order.
struct OrderExecutedWithPrice {
    const uint8_t* data;

    uint16_t stock_locate()      const { return read_big_endian(data + 1, std::integral_constant<int, 2>{}); }
    uint64_t timestamp()         const { return read_timestamp(data + 5); }
    uint64_t order_ref()         const { return read_big_endian(data + 11, std::integral_constant<int, 8>{}); }
    uint32_t executed_shares()   const { return read_big_endian(data + 19, std::integral_constant<int, 4>{}); }
    uint32_t price()             const { return read_big_endian(data + 32, std::integral_constant<int, 4>{}); }
};

// Order Cancel — 'X' (23 bytes)
// Reduce the quantity of an existing order.
struct OrderCancel {
    const uint8_t* data;

    uint16_t stock_locate()      const { return read_big_endian(data + 1, std::integral_constant<int, 2>{}); }
    uint64_t timestamp()         const { return read_timestamp(data + 5); }
    uint64_t order_ref()         const { return read_big_endian(data + 11, std::integral_constant<int, 8>{}); }
    uint32_t cancelled_shares()  const { return read_big_endian(data + 19, std::integral_constant<int, 4>{}); }
};

// Order Delete — 'D' (19 bytes)
// Remove an order from the book entirely.
struct OrderDelete {
    const uint8_t* data;

    uint16_t stock_locate()      const { return read_big_endian(data + 1, std::integral_constant<int, 2>{}); }
    uint64_t timestamp()         const { return read_timestamp(data + 5); }
    uint64_t order_ref()         const { return read_big_endian(data + 11, std::integral_constant<int, 8>{}); }
};

// Order Replace — 'U' (35 bytes)
// Delete old order, create new one with new ref, price, and quantity.
// Side and stock stay the same as the original.
struct OrderReplace {
    const uint8_t* data;

    uint16_t stock_locate()      const { return read_big_endian(data + 1, std::integral_constant<int, 2>{}); }
    uint64_t timestamp()         const { return read_timestamp(data + 5); }
    uint64_t old_order_ref()     const { return read_big_endian(data + 11, std::integral_constant<int, 8>{}); }
    uint64_t new_order_ref()     const { return read_big_endian(data + 19, std::integral_constant<int, 8>{}); }
    uint32_t shares()            const { return read_big_endian(data + 27, std::integral_constant<int, 4>{}); }
    uint32_t price()             const { return read_big_endian(data + 31, std::integral_constant<int, 4>{}); }
};

// Trade (Non-Cross) — 'P' (44 bytes)
// A trade involving a non-displayed order. Doesn't affect the visible book.
struct Trade {
    const uint8_t* data;

    uint16_t stock_locate()      const { return read_big_endian(data + 1, std::integral_constant<int, 2>{}); }
    uint64_t timestamp()         const { return read_timestamp(data + 5); }
    uint64_t order_ref()         const { return read_big_endian(data + 11, std::integral_constant<int, 8>{}); }
    char side()                  const { return static_cast<char>(data[19]); }
    uint32_t shares()            const { return read_big_endian(data + 20, std::integral_constant<int, 4>{}); }
    uint32_t price()             const { return read_big_endian(data + 32, std::integral_constant<int, 4>{}); }
    uint64_t match_number()      const { return read_big_endian(data + 36, std::integral_constant<int, 8>{}); }

    std::string_view stock()     const { return read_stock(data + 24); }
};
