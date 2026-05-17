#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <chrono>

namespace membra {

struct ComputeStake {
    std::string stake_id;
    std::string staker_address;
    uint64_t cpu_cores;          // number of CPU cores staked
    uint64_t ram_gb;             // RAM in GB staked
    uint64_t compute_units;      // compute units (normalized)
    double staked_at;
    double unstaked_at;          // 0 if still staked
    bool active;                 // true if staked
    double reward_rate;          // reward credits per second per unit
    uint64_t instant_credit_grant;  // instant MEMBRA gas credit grant (not SOL)
    uint64_t total_credit_grants_issued;  // total credit grants
    uint64_t gas_credit_allowance;  // remaining MEMBRA gas credit allowance (not SOL)
    bool redeemable;            // true if credits can be redeemed from funded vault
};

class ComputeStaking {
private:
    std::unordered_map<std::string, ComputeStake> stakes_;
    std::mutex mutex_;
    std::atomic<uint64_t> total_staked_compute_;
    std::atomic<uint64_t> total_cpu_cores_;
    std::atomic<uint64_t> total_credit_grants_issued_;

    // Configuration
    static constexpr double GAS_CREDITS_PER_CORE_PER_SEC = 1000.0;  // 1000 lamports/sec/core
    static constexpr double MIN_STAKE_DURATION_SEC = 3600.0;  // 1 hour minimum
    static constexpr double REWARD_MULTIPLIER = 1.5;  // bonus for longer stakes
    static constexpr uint64_t CREDIT_GRANT_PER_CORE = 10000000000;  // 10 SOL-equivalent gas credit units per core
    static constexpr uint64_t GAS_CREDIT_ALLOWANCE_PER_CORE = 10000000000;  // 10 SOL-equivalent gas credit allowance per core

public:
    ComputeStaking()
        : total_staked_compute_(0), total_cpu_cores_(0), total_credit_grants_issued_(0) {}

    // Stake CPU/RAM resources - MEMBRA issues MEMBRA gas credits (not SOL)
    std::string stake_compute(
        const std::string& staker_address,
        uint64_t cpu_cores,
        uint64_t ram_gb,
        uint64_t compute_units,
        uint64_t& credit_grant
    ) {
        std::lock_guard<std::mutex> lock(mutex_);

        std::string stake_id = "stake_" + staker_address + "_" +
                              std::to_string(std::chrono::duration<double>(
                                  std::chrono::system_clock::now().time_since_epoch()).count());

        // Issue MEMBRA gas credit grant (not SOL)
        credit_grant = cpu_cores * CREDIT_GRANT_PER_CORE;
        total_credit_grants_issued_ += credit_grant;

        // Calculate gas credit allowance for free txs (not SOL)
        uint64_t gas_credit_allowance = cpu_cores * GAS_CREDIT_ALLOWANCE_PER_CORE;

        ComputeStake stake;
        stake.stake_id = stake_id;
        stake.staker_address = staker_address;
        stake.cpu_cores = cpu_cores;
        stake.ram_gb = ram_gb;
        stake.compute_units = compute_units;
        stake.staked_at = current_timestamp();
        stake.unstaked_at = 0;
        stake.active = true;
        stake.reward_rate = GAS_CREDITS_PER_CORE_PER_SEC * REWARD_MULTIPLIER;
        stake.instant_credit_grant = credit_grant;
        stake.total_credit_grants_issued = credit_grant;
        stake.gas_credit_allowance = gas_credit_allowance;
        stake.redeemable = false;  // Not redeemable until vault funded

        stakes_[stake_id] = std::move(stake);

        total_staked_compute_ += compute_units;
        total_cpu_cores_ += cpu_cores;

        return stake_id;
    }

    // Unstake and claim accumulated gas credits
    bool unstake_compute(const std::string& stake_id, uint64_t& additional_credits) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = stakes_.find(stake_id);
        if (it == stakes_.end()) return false;
        if (!it->second.active) return false;

        ComputeStake& stake = it->second;

        // Check minimum stake duration
        double duration = current_timestamp() - stake.staked_at;
        if (duration < MIN_STAKE_DURATION_SEC) {
            return false;  // too early to unstake
        }

        // Calculate additional earned MEMBRA gas credits (time-based)
        additional_credits = calculate_time_rewards(stake, duration);
        stake.total_credit_grants_issued += additional_credits;

        stake.active = false;
        stake.unstaked_at = current_timestamp();

        total_staked_compute_ -= stake.compute_units;
        total_cpu_cores_ -= stake.cpu_cores;

        return true;
    }

    // Get current gas credit rate from staked compute
    uint64_t current_gas_credit_rate() const {
        uint64_t cores = total_cpu_cores_.load();
        return static_cast<uint64_t>(cores * GAS_CREDITS_PER_CORE_PER_SEC);
    }

    // Get total staked compute
    uint64_t total_staked_compute() const { return total_staked_compute_.load(); }
    uint64_t total_cpu_cores() const { return total_cpu_cores_.load(); }
    uint64_t total_credit_grants_issued() const { return total_credit_grants_issued_.load(); }

    // Get stake info
    ComputeStake* get_stake(const std::string& stake_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = stakes_.find(stake_id);
        return it == stakes_.end() ? nullptr : &it->second;
    }

    // Get all stakes for a staker
    std::vector<ComputeStake> get_staker_stakes(const std::string& staker_address) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<ComputeStake> result;
        for (const auto& pair : stakes_) {
            if (pair.second.staker_address == staker_address) {
                result.push_back(pair.second);
            }
        }
        return result;
    }

    // Consume gas credit allowance for free transaction
    bool consume_gas_allowance(const std::string& staker_address, uint64_t gas_cost) {
        std::lock_guard<std::mutex> lock(mutex_);

        // Find active stake for this staker
        for (auto& pair : stakes_) {
            ComputeStake& stake = pair.second;
            if (stake.staker_address == staker_address && stake.active) {
                if (stake.gas_credit_allowance >= gas_cost) {
                    stake.gas_credit_allowance -= gas_cost;
                    return true;
                }
            }
        }
        return false;
    }

    // Get remaining gas credit allowance for a staker
    uint64_t get_gas_allowance(const std::string& staker_address) {
        std::lock_guard<std::mutex> lock(mutex_);

        for (const auto& pair : stakes_) {
            const ComputeStake& stake = pair.second;
            if (stake.staker_address == staker_address && stake.active) {
                return stake.gas_credit_allowance;
            }
        }
        return 0;
    }

private:
    static uint64_t calculate_time_rewards(const ComputeStake& stake, double duration) {
        // Time-based rewards = compute_units * reward_rate * duration
        double reward = stake.compute_units * stake.reward_rate * duration;
        return static_cast<uint64_t>(reward);
    }

    static double current_timestamp() {
        return std::chrono::duration<double>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

} // namespace membra
