#include "crypto.hpp"
#include <openssl/sha.h>
#include <stdexcept>

namespace membra {

std::array<uint8_t, 32> Crypto::sha256(const std::vector<uint8_t>& data) const {
    std::array<uint8_t, 32> hash;
    SHA256(data.data(), data.size(), hash.data());
    return hash;
}

std::array<uint8_t, 32> Crypto::sha256(const std::string& data) const {
    std::array<uint8_t, 32> hash;
    SHA256(reinterpret_cast<const uint8_t*>(data.data()), data.size(), hash.data());
    return hash;
}

} // namespace membra