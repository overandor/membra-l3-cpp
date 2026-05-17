#pragma once

#include "compute_staking.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace membra {

// Compute-collateralized gas model: gas subsidized by staked CPU/compute
// Neither sender nor receiver pays gas directly
// Gas is paid from staked compute rewards + protocol reserves

class GasVault {
private:
    std::atomic<uint64_t> sol_reserves_;  // in lamports (backup)
    std::atomic<uint64_t> total_gas_paid_;
    std::atomic<uint64_t> gas_paid_by_compute_;  // gas subsidized by staked compute
    std::mutex mutex_;
    std::unordered_map<std::string, uint64_t> user_tx_counts_;  // for rate limiting
    ComputeStaking* compute_staking_;  // pointer to compute staking system

public:
    GasVault(uint64_t initial_sol_lamports = 0, ComputeStaking* staking = nullptr)
        : sol_reserves_(initial_sol_lamports), total_gas_paid_(0),
          gas_paid_by_compute_(0), compute_staking_(staking) {}

    void set_compute_staking(ComputeStaking* staking) {
        compute_staking_ = staking;
    }

    void deposit_sol(uint64_t amount_lamports, const std::string& source = "treasury") {
        sol_reserves_ += amount_lamports;
    }

    bool can_cover_gas(uint64_t gas_lamports) const {
        // Check if staked compute can subsidize
        if (compute_staking_) {
            uint64_t compute_rate = compute_staking_->current_gas_credit_rate();
            if (compute_rate > 0) {
                return true;  // staked compute can subsidize
            }
        }
        // Fallback to reserves
        return sol_reserves_.load() >= gas_lamports;
    }

    // Protocol pays gas using user's staked compute allowance first, then reserves
    bool pay_gas(const std::string& user_address, uint64_t gas_lamports) {
        // Rate limiting check
        std::lock_guard<std::mutex> lock(mutex_);
        if (user_tx_counts_[user_address] > 1000) {  // max 1000 txs per user
            return false;
        }

        // Priority: use user's gas allowance from their staked compute
        if (compute_staking_) {
            if (compute_staking_->consume_gas_allowance(user_address, gas_lamports)) {
                // User's gas allowance covers this transaction
                gas_paid_by_compute_ += gas_lamports;
                total_gas_paid_ += gas_lamports;
                user_tx_counts_[user_address]++;
                return true;
            }
        }

        // Fallback to protocol reserves
        if (sol_reserves_ < gas_lamports) {
            return false;
        }
        sol_reserves_ -= gas_lamports;
        total_gas_paid_ += gas_lamports;
        user_tx_counts_[user_address]++;
        return true;
    }

    uint64_t sol_reserves() const { return sol_reserves_.load(); }
    uint64_t total_gas_paid() const { return total_gas_paid_.load(); }
    uint64_t gas_paid_by_compute() const { return gas_paid_by_compute_.load(); }
    double sol_reserves_sol() const { return sol_reserves_ / 1e9; }

    bool is_healthy(double min_reserves_sol = 10.0) const {
        return sol_reserves_sol() >= min_reserves_sol;
    }

    double compute_coverage_ratio() const {
        if (total_gas_paid_ == 0) return 0.0;
        return static_cast<double>(gas_paid_by_compute_) / total_gas_paid_;
    }
};

} // namespace membra
