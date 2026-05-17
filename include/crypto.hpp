#pragma once

#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <cstring>
#include <openssl/sha.h>
#include <mutex>

namespace membra {

using Hash = std::array<uint8_t, 32>;
using Address = std::array<uint8_t, 20>;

inline Hash sha256(const std::string& data) {
    Hash h{};
    SHA256(reinterpret_cast<const uint8_t*>(data.data()), data.size(), h.data());
    return h;
}

inline Hash sha256(const std::vector<uint8_t>& data) {
    Hash h{};
    SHA256(data.data(), data.size(), h.data());
    return h;
}

inline std::string hash_to_hex(const Hash& h) {
    std::string hex;
    hex.reserve(64);
    for (auto b : h) {
        char buf[3];
        snprintf(buf, 3, "%02x", b);
        hex += buf;
    }
    return hex;
}

inline Address hash_to_address(const Hash& h) {
    Address a{};
    std::memcpy(a.data(), h.data(), 20);
    return a;
}

} // namespace membra
