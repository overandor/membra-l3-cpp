#ifndef GAS_REIMBURSEMENT_HPP
#define GAS_REIMBURSEMENT_HPP

#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <memory>
#include <map>
#include <chrono>
#include "crypto.hpp"
#include "zk_compute.hpp"

namespace membra {
namespace gas {

// Constants
constexpr size_t WALLET_ADDRESS_SIZE = 32;
constexpr size_t TRANSACTION_HASH_SIZE = 32;
constexpr uint64_t MIN_BALANCE_LAMPORTS = 1000; // Minimum balance to prevent zero balance
constexpr uint64_t DEFAULT_REIMBURSEMENT_RATE_BPS = 10000; // 100% reimbursement by default
constexpr uint64_t MAX_REIMBURSEMENT_MULTIPLIER = 2; // Maximum 2x gas reimbursement

/**
 * Reimbursement types
 */
enum class ReimbursementType {
    PRE_TRANSACTION,  // Gas estimation before transaction
    POST_TRANSACTION, // Actual gas reimbursement after transaction
    BONUS,           // Bonus for successful transactions
    EMERGENCY,       // Emergency reimbursement for failed transactions
    ZK_COMPUTE_BACKED // Reimbursement backed by ZK compute proofs
};

/**
 * Transaction gas information
 */
struct TransactionGasInfo {
    std::string transaction_id;
    std::string wallet_address;
    uint64_t estimated_gas_units;
    uint64_t actual_gas_used;
    uint64_t gas_price_lamports;
    uint64_t total_gas_cost;
    std::string transaction_type; // "transfer", "swap", "stake", etc.
    int64_t timestamp;
    bool success;
    std::string error_message;
    
    TransactionGasInfo()
        : estimated_gas_units(0),
          actual_gas_used(0),
          gas_price_lamports(0),
          total_gas_cost(0),
          timestamp(0),
          success(false) {}
};

/**
 * Reimbursement request
 */
struct ReimbursementRequest {
    std::string request_id;
    std::string transaction_id;
    std::string wallet_address;
    ReimbursementType reimbursement_type;
    uint64_t gas_cost;
    uint64_t requested_amount;
    std::string justification;
    std::array<uint8_t, TRANSACTION_HASH_SIZE> transaction_hash;
    int64_t created_at;
    
    ReimbursementRequest()
        : reimbursement_type(ReimbursementType::PRE_TRANSACTION),
          gas_cost(0),
          requested_amount(0),
          created_at(0) {
        transaction_hash.fill(0);
    }
};

/**
 * Reimbursement record
 */
struct ReimbursementRecord {
    std::string request_id;
    std::string transaction_id;
    std::string wallet_address;
    ReimbursementType reimbursement_type;
    uint64_t gas_cost;
    uint64_t reimbursed_amount;
    uint64_t bonus_amount;
    uint64_t total_amount;
    std::string status; // "pending", "approved", "rejected", "paid"
    int64_t processed_at;
    std::string approval_signature;
    std::map<std::string, std::string> metadata;
    
    ReimbursementRecord()
        : reimbursement_type(ReimbursementType::PRE_TRANSACTION),
          gas_cost(0),
          reimbursed_amount(0),
          bonus_amount(0),
          total_amount(0),
          status("pending"),
          processed_at(0) {}
};

/**
 * Gas pool configuration
 */
struct GasPoolConfig {
    std::string pool_id;
    uint64_t total_pool_balance;
    uint64_t available_balance;
    uint64_t reimbursement_rate_bps;
    uint64_t bonus_rate_bps;
    uint64_t max_reimbursement_per_tx;
    uint64_t min_balance_threshold;
    bool auto_approve_enabled;
    bool zk_compute_backing_enabled;
    
    GasPoolConfig()
        : total_pool_balance(0),
          available_balance(0),
          reimbursement_rate_bps(DEFAULT_REIMBURSEMENT_RATE_BPS),
          bonus_rate_bps(0),
          max_reimbursement_per_tx(10000000), // 0.01 SOL max per tx
          min_balance_threshold(MIN_BALANCE_LAMPORTS),
          auto_approve_enabled(false),
          zk_compute_backing_enabled(false) {}
};

/**
 * Wallet balance state
 */
struct WalletBalanceState {
    std::string wallet_address;
    uint64_t current_balance;
    uint64_t min_balance_threshold;
    bool is_below_threshold;
    uint64_t pending_reimbursements;
    uint64_t total_reimbursed;
    int64_t last_updated;
    
    WalletBalanceState()
        : current_balance(0),
          min_balance_threshold(MIN_BALANCE_LAMPORTS),
          is_below_threshold(false),
          pending_reimbursements(0),
          total_reimbursed(0),
          last_updated(0) {}
};

/**
 * Gas estimator
 */
class GasEstimator {
public:
    GasEstimator();
    ~GasEstimator() = default;
    
    // Estimate gas for transaction
    uint64_t estimate_gas(const std::string& transaction_type,
                         const std::vector<uint8_t>& transaction_data);
    
    // Get current gas price
    uint64_t get_gas_price();
    
    // Calculate total gas cost
    uint64_t calculate_total_cost(uint64_t gas_units, uint64_t gas_price);
    
    // Update gas price oracle
    void update_gas_price(uint64_t new_price);
    
private:
    uint64_t current_gas_price_;
    std::map<std::string, uint64_t> base_gas_costs_;
    std::unique_ptr<Crypto> crypto_;
    
    uint64_t get_base_gas_cost(const std::string& transaction_type);
};

/**
 * Gas reimbursement manager
 */
class GasReimbursementManager {
public:
    GasReimbursementManager();
    ~GasReimbursementManager() = default;
    
    // Submit reimbursement request
    std::string submit_request(const ReimbursementRequest& request);
    
    // Process reimbursement request
    ReimbursementRecord process_request(const std::string& request_id);
    
    // Approve reimbursement
    bool approve_reimbursement(const std::string& request_id,
                              const std::string& approver_signature);
    
    // Reject reimbursement
    bool reject_reimbursement(const std::string& request_id,
                             const std::string& reason);
    
    // Calculate reimbursement amount
    uint64_t calculate_reimbursement(const TransactionGasInfo& gas_info,
                                    const GasPoolConfig& config);
    
    // Calculate bonus amount
    uint64_t calculate_bonus(const TransactionGasInfo& gas_info,
                            const GasPoolConfig& config);
    
    // Check wallet balance (never-zero enforcement)
    bool check_wallet_balance(const std::string& wallet_address,
                             uint64_t required_amount);
    
    // Get reimbursement status
    ReimbursementRecord get_reimbursement_status(const std::string& request_id);
    
    // Get pending reimbursements
    std::vector<ReimbursementRecord> get_pending_reimbursements();
    
    // Statistics
    struct ReimbursementStats {
        uint64_t total_requests;
        uint64_t total_approved;
        uint64_t total_rejected;
        uint64_t total_reimbursed;
        uint64_t total_bonus_paid;
        double average_reimbursement_time_ms;
    };
    ReimbursementStats get_stats() const;
    
private:
    std::map<std::string, ReimbursementRequest> requests_;
    std::map<std::string, ReimbursementRecord> records_;
    std::map<std::string, WalletBalanceState> wallet_states_;
    std::shared_ptr<GasEstimator> estimator_;
    std::unique_ptr<Crypto> crypto_;
    
    ReimbursementStats stats_;
    
    // Generate request ID
    std::string generate_request_id();
    
    // Update wallet balance state
    void update_wallet_state(const std::string& wallet_address, uint64_t amount);
    
    // Update statistics
    void update_stats(const ReimbursementRecord& record);
};

/**
 * Gas pool manager
 */
class GasPoolManager {
public:
    GasPoolManager();
    ~GasPoolManager() = default;
    
    // Initialize gas pool
    bool initialize_pool(const GasPoolConfig& config);
    
    // Get pool configuration
    GasPoolConfig get_pool_config(const std::string& pool_id);
    
    // Update pool balance
    bool update_pool_balance(const std::string& pool_id, uint64_t amount);
    
    // Check pool availability
    bool check_availability(const std::string& pool_id, uint64_t required_amount);
    
    // Reserve funds for reimbursement
    bool reserve_funds(const std::string& pool_id, uint64_t amount);
    
    // Release reserved funds
    void release_funds(const std::string& pool_id, uint64_t amount);
    
    // Get pool statistics
    struct PoolStats {
        uint64_t total_deposited;
        uint64_t total_withdrawn;
        uint64_t current_balance;
        uint64_t reserved_amount;
        uint64_t total_reimbursements;
        double utilization_ratio;
    };
    PoolStats get_pool_stats(const std::string& pool_id);
    
private:
    std::map<std::string, GasPoolConfig> pool_configs_;
    std::map<std::string, uint64_t> reserved_amounts_;
    PoolStats stats_;
    std::unique_ptr<Crypto> crypto_;
};

/**
 * ZK-backed gas reimbursement
 */
class ZKGasReimbursement {
public:
    ZKGasReimbursement();
    ~ZKGasReimbursement() = default;
    
    // Request ZK-backed reimbursement
    std::string request_zk_reimbursement(const TransactionGasInfo& gas_info,
                                        const zkcompute::ZKComputeProof& proof);
    
    // Verify ZK proof for reimbursement
    bool verify_zk_reimbursement(const std::string& request_id,
                                const zkcompute::ZKComputeProof& proof);
    
    // Calculate ZK-backed amount
    uint64_t calculate_zk_amount(const TransactionGasInfo& gas_info,
                                const zkcompute::VerificationResult& verification);
    
    // Get ZK reimbursement status
    ReimbursementRecord get_zk_status(const std::string& request_id);
    
private:
    std::map<std::string, ReimbursementRecord> zk_records_;
    std::shared_ptr<zkcompute::ZKComputeManager> zk_manager_;
    std::unique_ptr<Crypto> crypto_;
};

/**
 * Gas reimbursement factory
 */
struct GasReimbursementStack {
    std::shared_ptr<GasEstimator> estimator;
    std::shared_ptr<GasReimbursementManager> manager;
    std::shared_ptr<GasPoolManager> pool_manager;
    std::shared_ptr<ZKGasReimbursement> zk_reimbursement;
};

GasReimbursementStack create_gas_reimbursement_stack();

} // namespace gas
} // namespace membra

#endif // GAS_REIMBURSEMENT_HPP