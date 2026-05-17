#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace membra {

enum class GasPaymentSource : uint8_t {
    SENDER = 0,
    RECEIVER = 1,
    PROTOCOL = 2,
    STAKED_COMPUTE = 3
};

struct GasReceipt {
    std::string receipt_id;
    std::string tx_id;
    std::string receiver_wallet;
    std::string sender_wallet;
    
    uint64_t gas_paid_lamports;
    uint64_t priority_fee_lamports;
    uint64_t total_fee_lamports;
    
    GasPaymentSource payment_source;
    std::string fee_payer;
    
    uint64_t slot;
    double block_time;
    std::string tx_signature;
    
    bool used_for_compensation;
    double created_at;
};

struct GasOracleState {
    std::string oracle_id;
    std::string chain;
    std::string cluster;
    
    uint64_t gas_twap_lamports;
    uint64_t priority_fee_twap_lamports;
    double sol_usd_twap;
    double membra_usd_twap;
    
    double last_updated_at;
    double stale_after_seconds;
    
    bool is_stale() const {
        double now = std::chrono::duration<double>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        return (now - last_updated_at) > stale_after_seconds;
    }
};

class ReceiverPaidGas {
private:
    std::unordered_map<std::string, GasReceipt> receipts_;
    std::mutex mutex_;
    std::atomic<uint64_t> receipt_counter_;
    
    GasOracleState oracle_state_;
    std::mutex oracle_mutex_;
    
    // Compensation configuration
    bool compensation_enabled_;
    uint64_t max_compensation_per_tx_;
    uint64_t max_compensation_per_wallet_per_day_;
    uint64_t max_compensation_per_epoch_;
    
    static constexpr double ORACLE_STALE_SECONDS = 300.0;  // 5 minutes

public:
    ReceiverPaidGas()
        : receipt_counter_(0),
          compensation_enabled_(false),  // Disabled by default
          max_compensation_per_tx_(1000000000),  // 1 SOL
          max_compensation_per_wallet_per_day_(10000000000),  // 10 SOL
          max_compensation_per_epoch_(100000000000)  // 100 SOL
    {
        // Initialize oracle with defaults
        oracle_state_.oracle_id = "gas_oracle_1";
        oracle_state_.chain = "solana";
        oracle_state_.cluster = "devnet";
        oracle_state_.gas_twap_lamports = 5000;
        oracle_state_.priority_fee_twap_lamports = 1000;
        oracle_state_.sol_usd_twap = 150.0;
        oracle_state_.membra_usd_twap = 0.10;
        oracle_state_.last_updated_at = current_timestamp();
        oracle_state_.stale_after_seconds = ORACLE_STALE_SECONDS;
    }
    
    // Create a gas receipt (receiver paid the gas)
    std::string create_receipt(
        const std::string& tx_id,
        const std::string& receiver_wallet,
        const std::string& sender_wallet,
        uint64_t gas_paid_lamports,
        uint64_t priority_fee_lamports,
        uint64_t slot,
        const std::string& tx_signature
    ) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string receipt_id = "gas_" + std::to_string(receipt_counter_++);
        
        GasReceipt receipt;
        receipt.receipt_id = receipt_id;
        receipt.tx_id = tx_id;
        receipt.receiver_wallet = receiver_wallet;
        receipt.sender_wallet = sender_wallet;
        receipt.gas_paid_lamports = gas_paid_lamports;
        receipt.priority_fee_lamports = priority_fee_lamports;
        receipt.total_fee_lamports = gas_paid_lamports + priority_fee_lamports;
        receipt.payment_source = GasPaymentSource::RECEIVER;
        receipt.fee_payer = receiver_wallet;
        receipt.slot = slot;
        receipt.block_time = current_timestamp();
        receipt.tx_signature = tx_signature;
        receipt.used_for_compensation = false;
        receipt.created_at = current_timestamp();
        
        receipts_[receipt_id] = std::move(receipt);
        return receipt_id;
    }
    
    // Update gas oracle state
    void update_oracle(
        uint64_t gas_twap,
        uint64_t priority_fee_twap,
        double sol_usd,
        double membra_usd
    ) {
        std::lock_guard<std::mutex> lock(oracle_mutex_);
        
        oracle_state_.gas_twap_lamports = gas_twap;
        oracle_state_.priority_fee_twap_lamports = priority_fee_twap;
        oracle_state_.sol_usd_twap = sol_usd;
        oracle_state_.membra_usd_twap = membra_usd;
        oracle_state_.last_updated_at = current_timestamp();
    }
    
    // Calculate token compensation for a gas receipt
    uint64_t calculate_compensation(const std::string& receipt_id, bool& can_compensate) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = receipts_.find(receipt_id);
        if (it == receipts_.end()) {
            can_compensate = false;
            return 0;
        }
        
        GasReceipt& receipt = it->second;
        
        // Check if compensation is enabled
        if (!compensation_enabled_) {
            can_compensate = false;
            return 0;
        }
        
        // Check if oracle is stale
        {
            std::lock_guard<std::mutex> oracle_lock(oracle_mutex_);
            if (oracle_state_.is_stale()) {
                can_compensate = false;
                return 0;
            }
        }
        
        // Check if receipt already used
        if (receipt.used_for_compensation) {
            can_compensate = false;
            return 0;
        }
        
        // Check daily cap
        uint64_t wallet_daily = get_wallet_daily_compensation(receipt.receiver_wallet);
        if (wallet_daily + receipt.total_fee_lamports > max_compensation_per_wallet_per_day_) {
            can_compensate = false;
            return 0;
        }
        
        // Calculate raw compensation
        // formula: gas_paid * sol_usd * compensation_rate / membra_usd
        double gas_usd = receipt.total_fee_lamports / 1e9 * oracle_state_.sol_usd_twap;
        double compensation_rate = 0.5;  // 50% compensation rate
        double compensation_usd = gas_usd * compensation_rate;
        double compensation_tokens = compensation_usd / oracle_state_.membra_usd_twap;
        
        // Apply caps
        uint64_t raw_compensation = static_cast<uint64_t>(compensation_tokens * 1e9);  // Convert to lamports
        uint64_t capped_compensation = std::min(raw_compensation, max_compensation_per_tx_);
        
        can_compensate = true;
        return capped_compensation;
    }
    
    // Mark receipt as used for compensation
    bool mark_compensated(const std::string& receipt_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = receipts_.find(receipt_id);
        if (it == receipts_.end()) return false;
        
        it->second.used_for_compensation = true;
        return true;
    }
    
    // Enable/disable compensation
    void set_compensation_enabled(bool enabled) {
        compensation_enabled_ = enabled;
    }
    
    // Get oracle state
    GasOracleState get_oracle_state() {
        std::lock_guard<std::mutex> lock(oracle_mutex_);
        return oracle_state_;
    }
    
    // Get receipt
    GasReceipt* get_receipt(const std::string& receipt_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = receipts_.find(receipt_id);
        return it == receipts_.end() ? nullptr : &it->second;
    }

private:
    uint64_t get_wallet_daily_compensation(const std::string& wallet_address) {
        double now = current_timestamp();
        double day_start = now - 86400.0;
        
        uint64_t total = 0;
        for (const auto& pair : receipts_) {
            const GasReceipt& receipt = pair.second;
            if (receipt.receiver_wallet == wallet_address && 
                receipt.used_for_compensation && 
                receipt.created_at >= day_start) {
                total += receipt.total_fee_lamports;
            }
        }
        return total;
    }
    
    static double current_timestamp() {
        return std::chrono::duration<double>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

} // namespace membra
