#pragma once

#include "crypto.hpp"
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <chrono>

namespace membra {

enum class ProofType : uint8_t {
    VOLATILITY = 0,
    DEVELOPMENT = 1,
    ZK_COMPUTE = 2,
    GOVERNANCE = 3,
    FEE_CREDIT = 4,
    ROUTE_VALIDATION = 5,
    ORACLE_UPDATE = 6,
    TREASURY = 7,
    IDENTITY_LINK = 8
};

struct ProofEntry {
    ProofType type;
    std::string data;  // JSON string for simplicity
    double timestamp;
    std::string prev_hash;
    std::string entry_hash;

    std::string compute_hash() const {
        std::string payload = std::to_string(static_cast<int>(type)) + data +
                             std::to_string(timestamp) + prev_hash;
        return hash_to_hex(sha256(payload));
    }

    void seal(const std::string& prev) {
        prev_hash = prev;
        entry_hash = compute_hash();
    }
};

class ProofBook {
private:
    std::vector<ProofEntry> entries_;
    std::string last_hash_;
    std::mutex mutex_;

public:
    ProofBook() : last_hash_(64, '0') {}  // genesis hash

    ProofEntry append(ProofType type, const std::string& data) {
        std::lock_guard<std::mutex> lock(mutex_);
        ProofEntry entry{type, data, current_timestamp(), "", ""};
        entry.seal(last_hash_);
        entries_.push_back(std::move(entry));
        last_hash_ = entries_.back().entry_hash;
        return entries_.back();
    }

    std::vector<ProofEntry> get_entries(ProofType type, size_t limit = 100) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<ProofEntry> result;
        size_t count = 0;
        for (auto it = entries_.rbegin(); it != entries_.rend() && count < limit; ++it) {
            if (it->type == type) {
                result.push_back(*it);
                count++;
            }
        }
        return result;
    }

    bool verify_chain() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string expected(64, '0');
        for (const auto& entry : entries_) {
            if (entry.prev_hash != expected) return false;
            std::string computed = entry.compute_hash();
            if (entry.entry_hash != computed) return false;
            expected = computed;
        }
        return true;
    }

    const std::string& last_hash() const { return last_hash_; }
    size_t entry_count() const { return entries_.size(); }

private:
    static double current_timestamp() {
        return std::chrono::duration<double>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

} // namespace membra
