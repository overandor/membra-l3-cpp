#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace membra {

struct RewardVaultEntry {
    std::string entry_id;
    std::string worker_address;
    std::string receipt_id;
    
    uint64_t amount_lamports;
    std::string source;  // "inference", "staking", "futures"
    
    double timestamp;
    bool claimed;
    double claimed_at;
    
    // Accounting fields
    std::string pool_id;
    uint64_t pool_balance_before;
    uint64_t pool_balance_after;
    
    // Funding verification
    bool payout_possible;
    std::string funding_source;
};

class RewardVault {
private:
    std::unordered_map<std::string, RewardVaultEntry> entries_;
    std::mutex mutex_;
    
    std::atomic<uint64_t> total_rewards_available_;
    std::atomic<uint64_t> total_rewards_claimed_;
    std::atomic<uint64_t> pool_balance_;
    
    // Caps and limits
    static constexpr uint64_t MAX_REWARD_PER_RECEIPT = 1000000000;  // 1 SOL
    static constexpr uint64_t MAX_REWARD_PER_WORKER_PER_DAY = 10000000000;  // 10 SOL
    static constexpr uint64_t MAX_POOL_BALANCE = 1000000000000;  // 1000 SOL
    
public:
    RewardVault(uint64_t initial_pool_balance = 0)
        : total_rewards_available_(initial_pool_balance),
          total_rewards_claimed_(0),
          pool_balance_(initial_pool_balance) {}
    
    // Deposit funds to the reward pool
    bool deposit_to_pool(uint64_t amount_lamports, const std::string& source = "treasury") {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (pool_balance_ + amount_lamports > MAX_POOL_BALANCE) {
            return false;  // Would exceed max pool balance
        }
        
        pool_balance_ += amount_lamports;
        total_rewards_available_ += amount_lamports;
        return true;
    }
    
    // Create a reward entry (doesn't claim yet)
    std::string create_reward_entry(
        const std::string& worker_address,
        const std::string& receipt_id,
        uint64_t amount_lamports,
        const std::string& source
    ) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check if vault is funded
        if (pool_balance_.load() == 0) {
            return "";  // Vault not funded - no rewards possible
        }
        
        // Check caps
        if (amount_lamports > MAX_REWARD_PER_RECEIPT) {
            return "";  // Exceeds per-receipt cap
        }
        
        // Check daily cap for worker
        uint64_t worker_daily_claimed = get_worker_daily_claimed(worker_address);
        if (worker_daily_claimed + amount_lamports > MAX_REWARD_PER_WORKER_PER_DAY) {
            return "";  // Exceeds daily cap
        }
        
        // Check pool balance
        if (amount_lamports > pool_balance_.load()) {
            return "";  // Insufficient pool balance
        }
        
        std::string entry_id = "reward_" + worker_address + "_" + 
                              std::to_string(current_timestamp());
        
        RewardVaultEntry entry;
        entry.entry_id = entry_id;
        entry.worker_address = worker_address;
        entry.receipt_id = receipt_id;
        entry.amount_lamports = amount_lamports;
        entry.source = source;
        entry.timestamp = current_timestamp();
        entry.claimed = false;
        entry.claimed_at = 0;
        entry.pool_id = "main_pool";
        entry.pool_balance_before = pool_balance_.load();
        entry.payout_possible = true;
        entry.funding_source = "treasury_or_api_usage";
        
        entries_[entry_id] = std::move(entry);
        return entry_id;
    }
    
    // Claim a reward
    bool claim_reward(const std::string& entry_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = entries_.find(entry_id);
        if (it == entries_.end()) return false;
        if (it->second.claimed) return false;
        
        RewardVaultEntry& entry = it->second;
        
        // Check if vault is funded before paying
        if (pool_balance_.load() == 0) {
            return false;  // Vault not funded - cannot pay
        }
        
        // Double-check pool balance
        if (entry.amount_lamports > pool_balance_.load()) {
            return false;
        }
        
        // Deduct from pool
        pool_balance_ -= entry.amount_lamports;
        total_rewards_claimed_ += entry.amount_lamports;
        
        // Mark as claimed
        entry.claimed = true;
        entry.claimed_at = current_timestamp();
        entry.pool_balance_after = pool_balance_.load();
        
        return true;
    }
    
    // Get pool balance
    uint64_t pool_balance() const { return pool_balance_.load(); }
    uint64_t total_rewards_available() const { return total_rewards_available_.load(); }
    uint64_t total_rewards_claimed() const { return total_rewards_claimed_.load(); }
    uint64_t remaining_rewards() const { 
        return total_rewards_available_.load() - total_rewards_claimed_.load(); 
    }
    
    // Get worker's claimed rewards today
    uint64_t get_worker_daily_claimed(const std::string& worker_address) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        double now = current_timestamp();
        double day_start = now - 86400.0;  // 24 hours ago
        
        uint64_t total = 0;
        for (const auto& pair : entries_) {
            const RewardVaultEntry& entry = pair.second;
            if (entry.worker_address == worker_address && 
                entry.claimed && 
                entry.claimed_at >= day_start) {
                total += entry.amount_lamports;
            }
        }
        return total;
    }
    
    // Get reward entry
    RewardVaultEntry* get_entry(const std::string& entry_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = entries_.find(entry_id);
        return it == entries_.end() ? nullptr : &it->second;
    }
    
    // Get all entries for a worker
    std::vector<RewardVaultEntry> get_worker_entries(const std::string& worker_address) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<RewardVaultEntry> result;
        for (const auto& pair : entries_) {
            if (pair.second.worker_address == worker_address) {
                result.push_back(pair.second);
            }
        }
        return result;
    }

private:
    static double current_timestamp() {
        return std::chrono::duration<double>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

} // namespace membra
