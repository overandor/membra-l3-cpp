#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace membra {

// Protocol-subsidized gas model: neither sender nor receiver pays
// Protocol pays all gas fees from its own reserves
// No reimbursement needed - protocol bears the cost

class GasVault {
private:
    std::atomic<uint64_t> sol_reserves_;  // in lamports
    std::atomic<uint64_t> total_gas_paid_;
    std::mutex mutex_;
    std::unordered_map<std::string, uint64_t> user_tx_counts_;  // for rate limiting

public:
    GasVault(uint64_t initial_sol_lamports = 0)
        : sol_reserves_(initial_sol_lamports), total_gas_paid_(0) {}

    void deposit_sol(uint64_t amount_lamports, const std::string& source = "treasury") {
        sol_reserves_ += amount_lamports;
    }

    bool can_cover_gas(uint64_t gas_lamports) const {
        return sol_reserves_.load() >= gas_lamports;
    }

    // Protocol pays gas - no user credits needed
    bool pay_gas(const std::string& user_address, uint64_t gas_lamports) {
        // Check reserves
        if (!can_cover_gas(gas_lamports)) {
            return false;
        }

        // Rate limiting check (optional)
        std::lock_guard<std::mutex> lock(mutex_);
        if (user_tx_counts_[user_address] > 1000) {  // max 1000 txs per user
            return false;
        }

        // Protocol pays
        sol_reserves_ -= gas_lamports;
        total_gas_paid_ += gas_lamports;
        user_tx_counts_[user_address]++;
        return true;
    }

    uint64_t sol_reserves() const { return sol_reserves_.load(); }
    uint64_t total_gas_paid() const { return total_gas_paid_.load(); }
    double sol_reserves_sol() const { return sol_reserves_ / 1e9; }

    bool is_healthy(double min_reserves_sol = 10.0) const {
        return sol_reserves_sol() >= min_reserves_sol;
    }
};

} // namespace membra
