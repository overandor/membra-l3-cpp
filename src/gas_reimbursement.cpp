#include "gas_reimbursement.hpp"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>

namespace membra {
namespace gas {

// ============================================================================
// GasEstimator Implementation
// ============================================================================

GasEstimator::GasEstimator()
    : current_gas_price_(1000), // Default 1000 lamports per gas unit
      crypto_(std::make_unique<Crypto>()) {
    
    // Initialize base gas costs for different transaction types
    base_gas_costs_["transfer"] = 5000;
    base_gas_costs_["swap"] = 150000;
    base_gas_costs_["stake"] = 100000;
    base_gas_costs_["unstake"] = 100000;
    base_gas_costs_["mint"] = 200000;
    base_gas_costs_["burn"] = 100000;
    base_gas_costs_["approve"] = 50000;
    base_gas_costs_["revoke"] = 50000;
    base_gas_costs_["custom"] = 100000;
}

uint64_t GasEstimator::estimate_gas(const std::string& transaction_type,
                                   const std::vector<uint8_t>& transaction_data) {
    uint64_t base_cost = get_base_gas_cost(transaction_type);
    
    // Adjust based on transaction data size
    uint64_t data_cost = transaction_data.size() * 10; // 10 gas units per byte
    
    // Add random variance to simulate network conditions
    uint64_t variance = (base_cost / 10) * (rand() % 3); // 0-20% variance
    
    return base_cost + data_cost + variance;
}

uint64_t GasEstimator::get_gas_price() {
    return current_gas_price_;
}

uint64_t GasEstimator::calculate_total_cost(uint64_t gas_units, uint64_t gas_price) {
    return gas_units * gas_price;
}

void GasEstimator::update_gas_price(uint64_t new_price) {
    current_gas_price_ = new_price;
}

uint64_t GasEstimator::get_base_gas_cost(const std::string& transaction_type) {
    auto it = base_gas_costs_.find(transaction_type);
    if (it != base_gas_costs_.end()) {
        return it->second;
    }
    return base_gas_costs_["custom"]; // Default to custom transaction cost
}

// ============================================================================
// GasReimbursementManager Implementation
// ============================================================================

GasReimbursementManager::GasReimbursementManager()
    : estimator_(std::make_shared<GasEstimator>()),
      crypto_(std::make_unique<Crypto>()) {
    
    stats_ = {0, 0, 0, 0, 0, 0.0};
}

std::string GasReimbursementManager::submit_request(const ReimbursementRequest& request) {
    std::string request_id = request.request_id.empty() ? generate_request_id() : request.request_id;
    
    // Store request
    requests_[request_id] = request;
    
    // Create initial record
    ReimbursementRecord record;
    record.request_id = request_id;
    record.transaction_id = request.transaction_id;
    record.wallet_address = request.wallet_address;
    record.reimbursement_type = request.reimbursement_type;
    record.gas_cost = request.gas_cost;
    record.status = "pending";
    record.processed_at = 0;
    
    records_[request_id] = record;
    
    // Update statistics
    stats_.total_requests++;
    
    // Update wallet state
    update_wallet_state(request.wallet_address, request.gas_cost);
    
    return request_id;
}

ReimbursementRecord GasReimbursementManager::process_request(const std::string& request_id) {
    auto it = requests_.find(request_id);
    if (it == requests_.end()) {
        ReimbursementRecord empty_record;
        empty_record.request_id = request_id;
        empty_record.status = "not_found";
        return empty_record;
    }
    
    auto& record = records_[request_id];
    auto& request = it->second;
    
    // Use default pool config for calculation
    GasPoolConfig default_config;
    
    // Create gas info for calculation
    TransactionGasInfo gas_info;
    gas_info.transaction_id = request.transaction_id;
    gas_info.wallet_address = request.wallet_address;
    gas_info.total_gas_cost = request.gas_cost;
    gas_info.transaction_type = "custom";
    gas_info.success = true;
    
    // Calculate reimbursement
    uint64_t reimbursed_amount = calculate_reimbursement(gas_info, default_config);
    uint64_t bonus_amount = calculate_bonus(gas_info, default_config);
    
    record.reimbursed_amount = reimbursed_amount;
    record.bonus_amount = bonus_amount;
    record.total_amount = reimbursed_amount + bonus_amount;
    record.processed_at = std::chrono::system_clock::now().time_since_epoch().count();
    
    // Auto-approve if enabled
    if (default_config.auto_approve_enabled) {
        record.status = "approved";
        stats_.total_approved++;
        stats_.total_reimbursed += record.total_amount;
        if (bonus_amount > 0) {
            stats_.total_bonus_paid += bonus_amount;
        }
    } else {
        record.status = "pending_approval";
    }
    
    update_stats(record);
    
    return record;
}

bool GasReimbursementManager::approve_reimbursement(const std::string& request_id,
                                                    const std::string& approver_signature) {
    auto it = records_.find(request_id);
    if (it == records_.end()) {
        return false;
    }
    
    auto& record = it->second;
    if (record.status != "pending" && record.status != "pending_approval") {
        return false;
    }
    
    record.status = "approved";
    record.approval_signature = approver_signature;
    record.processed_at = std::chrono::system_clock::now().time_since_epoch().count();
    
    stats_.total_approved++;
    stats_.total_reimbursed += record.total_amount;
    if (record.bonus_amount > 0) {
        stats_.total_bonus_paid += record.bonus_amount;
    }
    
    return true;
}

bool GasReimbursementManager::reject_reimbursement(const std::string& request_id,
                                                   const std::string& reason) {
    auto it = records_.find(request_id);
    if (it == records_.end()) {
        return false;
    }
    
    auto& record = it->second;
    if (record.status == "approved" || record.status == "paid") {
        return false;
    }
    
    record.status = "rejected";
    record.metadata["rejection_reason"] = reason;
    record.processed_at = std::chrono::system_clock::now().time_since_epoch().count();
    
    stats_.total_rejected++;
    
    return true;
}

uint64_t GasReimbursementManager::calculate_reimbursement(const TransactionGasInfo& gas_info,
                                                         const GasPoolConfig& config) {
    // Base reimbursement is gas cost * rate
    uint64_t base_amount = (gas_info.total_gas_cost * config.reimbursement_rate_bps) / 10000;
    
    // Cap at maximum per transaction
    uint64_t capped_amount = std::min(base_amount, config.max_reimbursement_per_tx);
    
    // Apply multiplier based on transaction success
    uint64_t multiplier = gas_info.success ? 1 : 0;
    
    return capped_amount * multiplier;
}

uint64_t GasReimbursementManager::calculate_bonus(const TransactionGasInfo& gas_info,
                                                  const GasPoolConfig& config) {
    if (!gas_info.success || config.bonus_rate_bps == 0) {
        return 0;
    }
    
    // Bonus is calculated on gas cost
    uint64_t bonus_amount = (gas_info.total_gas_cost * config.bonus_rate_bps) / 10000;
    
    // Cap bonus at 50% of gas cost
    uint64_t max_bonus = gas_info.total_gas_cost / 2;
    return std::min(bonus_amount, max_bonus);
}

bool GasReimbursementManager::check_wallet_balance(const std::string& wallet_address,
                                                  uint64_t required_amount) {
    auto it = wallet_states_.find(wallet_address);
    if (it == wallet_states_.end()) {
        // Assume wallet has sufficient balance if unknown
        return true;
    }
    
    const auto& state = it->second;
    uint64_t available_balance = state.current_balance - state.pending_reimbursements;
    
    return available_balance >= (required_amount + state.min_balance_threshold);
}

ReimbursementRecord GasReimbursementManager::get_reimbursement_status(const std::string& request_id) {
    auto it = records_.find(request_id);
    if (it != records_.end()) {
        return it->second;
    }
    
    ReimbursementRecord empty_record;
    empty_record.request_id = request_id;
    empty_record.status = "not_found";
    return empty_record;
}

std::vector<ReimbursementRecord> GasReimbursementManager::get_pending_reimbursements() {
    std::vector<ReimbursementRecord> pending;
    
    for (const auto& [request_id, record] : records_) {
        if (record.status == "pending" || record.status == "pending_approval") {
            pending.push_back(record);
        }
    }
    
    return pending;
}

GasReimbursementManager::ReimbursementStats GasReimbursementManager::get_stats() const {
    return stats_;
}

std::string GasReimbursementManager::generate_request_id() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    
    // Simplified ID generation to avoid potential crypto issues
    return "gas_req_" + std::to_string(timestamp);
}

void GasReimbursementManager::update_wallet_state(const std::string& wallet_address, uint64_t amount) {
    auto& state = wallet_states_[wallet_address];
    state.wallet_address = wallet_address;
    state.pending_reimbursements += amount;
    state.last_updated = std::chrono::system_clock::now().time_since_epoch().count();
    
    // Check if below threshold
    if (state.current_balance < state.min_balance_threshold) {
        state.is_below_threshold = true;
    }
}

void GasReimbursementManager::update_stats(const ReimbursementRecord& /* record */) {
    // Statistics are updated in process_request and approve_reimbursement
    // This is a placeholder for additional stat tracking
}

// ============================================================================
// GasPoolManager Implementation
// ============================================================================

GasPoolManager::GasPoolManager()
    : crypto_(std::make_unique<Crypto>()) {
    
    stats_ = {0, 0, 0, 0, 0, 0.0};
}

bool GasPoolManager::initialize_pool(const GasPoolConfig& config) {
    pool_configs_[config.pool_id] = config;
    
    // Initialize stats
    stats_.total_deposited = config.total_pool_balance;
    stats_.current_balance = config.available_balance;
    
    return true;
}

GasPoolConfig GasPoolManager::get_pool_config(const std::string& pool_id) {
    auto it = pool_configs_.find(pool_id);
    if (it != pool_configs_.end()) {
        return it->second;
    }
    
    return GasPoolConfig(); // Return default config if not found
}

bool GasPoolManager::update_pool_balance(const std::string& pool_id, uint64_t amount) {
    auto it = pool_configs_.find(pool_id);
    if (it == pool_configs_.end()) {
        return false;
    }
    
    auto& config = it->second;
    config.total_pool_balance += amount;
    config.available_balance += amount;
    
    stats_.current_balance = config.available_balance;
    
    return true;
}

bool GasPoolManager::check_availability(const std::string& pool_id, uint64_t required_amount) {
    auto it = pool_configs_.find(pool_id);
    if (it == pool_configs_.end()) {
        return false;
    }
    
    const auto& config = it->second;
    uint64_t available = config.available_balance - reserved_amounts_[pool_id];
    
    return available >= required_amount;
}

bool GasPoolManager::reserve_funds(const std::string& pool_id, uint64_t amount) {
    if (!check_availability(pool_id, amount)) {
        return false;
    }
    
    reserved_amounts_[pool_id] += amount;
    stats_.reserved_amount = reserved_amounts_[pool_id];
    
    // Update utilization ratio
    auto& config = pool_configs_[pool_id];
    stats_.utilization_ratio = static_cast<double>(reserved_amounts_[pool_id]) / 
                             static_cast<double>(config.total_pool_balance);
    
    return true;
}

void GasPoolManager::release_funds(const std::string& pool_id, uint64_t amount) {
    if (reserved_amounts_[pool_id] >= amount) {
        reserved_amounts_[pool_id] -= amount;
    } else {
        reserved_amounts_[pool_id] = 0;
    }
    
    stats_.reserved_amount = reserved_amounts_[pool_id];
    
    // Update utilization ratio
    auto it = pool_configs_.find(pool_id);
    if (it != pool_configs_.end()) {
        const auto& config = it->second;
        stats_.utilization_ratio = static_cast<double>(reserved_amounts_[pool_id]) / 
                                 static_cast<double>(config.total_pool_balance);
    }
}

GasPoolManager::PoolStats GasPoolManager::get_pool_stats(const std::string& pool_id) {
    auto it = pool_configs_.find(pool_id);
    if (it != pool_configs_.end()) {
        const auto& config = it->second;
        PoolStats pool_stats;
        pool_stats.total_deposited = config.total_pool_balance;
        pool_stats.current_balance = config.available_balance;
        pool_stats.reserved_amount = reserved_amounts_[pool_id];
        pool_stats.total_reimbursements = stats_.total_reimbursements;
        pool_stats.utilization_ratio = stats_.utilization_ratio;
        pool_stats.total_withdrawn = pool_stats.total_deposited - pool_stats.current_balance;
        return pool_stats;
    }
    
    return PoolStats();
}

// ============================================================================
// ZKGasReimbursement Implementation
// ============================================================================

ZKGasReimbursement::ZKGasReimbursement()
    : zk_manager_(std::make_shared<zkcompute::ZKComputeManager>()),
      crypto_(std::make_unique<Crypto>()) {
}

std::string ZKGasReimbursement::request_zk_reimbursement(const TransactionGasInfo& gas_info,
                                                         const zkcompute::ZKComputeProof& proof) {
    std::string request_id = "zk_gas_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    
    // Create reimbursement record
    ReimbursementRecord record;
    record.request_id = request_id;
    record.transaction_id = gas_info.transaction_id;
    record.wallet_address = gas_info.wallet_address;
    record.reimbursement_type = ReimbursementType::ZK_COMPUTE_BACKED;
    record.gas_cost = gas_info.total_gas_cost;
    record.status = "pending_zk_verification";
    
    zk_records_[request_id] = record;
    
    return request_id;
}

bool ZKGasReimbursement::verify_zk_reimbursement(const std::string& request_id,
                                                 const zkcompute::ZKComputeProof& proof) {
    auto it = zk_records_.find(request_id);
    if (it == zk_records_.end()) {
        return false;
    }
    
    auto& record = it->second;
    
    // Verify the ZK proof
    zkcompute::VerificationResult verification;
    auto task_status = zk_manager_->get_task_status(proof.task_id);
    if (task_status.verification_status == "verified") {
        verification.success = true;
        verification.verification_status = "verified";
    } else {
        verification.success = false;
        verification.verification_status = "failed";
        verification.error_message = "Proof not verified";
    }
    
    if (verification.success) {
        // Calculate ZK-backed amount
        TransactionGasInfo gas_info;
        gas_info.transaction_id = record.transaction_id;
        gas_info.wallet_address = record.wallet_address;
        gas_info.total_gas_cost = record.gas_cost;
        gas_info.success = true;
        
        uint64_t zk_amount = calculate_zk_amount(gas_info, verification);
        record.reimbursed_amount = zk_amount;
        record.total_amount = zk_amount;
        record.status = "zk_verified";
        record.processed_at = std::chrono::system_clock::now().time_since_epoch().count();
        
        return true;
    }
    
    record.status = "zk_verification_failed";
    return false;
}

uint64_t ZKGasReimbursement::calculate_zk_amount(const TransactionGasInfo& gas_info,
                                                 const zkcompute::VerificationResult& verification) {
    // ZK-backed reimbursement can be higher due to proof of compute
    uint64_t base_amount = gas_info.total_gas_cost;
    
    // Apply ZK multiplier based on verification speed
    double zk_multiplier = std::min(2.0, 1000.0 / std::max(1.0, static_cast<double>(verification.verification_time_ms)));
    
    return static_cast<uint64_t>(base_amount * zk_multiplier);
}

ReimbursementRecord ZKGasReimbursement::get_zk_status(const std::string& request_id) {
    auto it = zk_records_.find(request_id);
    if (it != zk_records_.end()) {
        return it->second;
    }
    
    ReimbursementRecord empty_record;
    empty_record.request_id = request_id;
    empty_record.status = "not_found";
    return empty_record;
}

// ============================================================================
// Factory Function Implementation
// ============================================================================

GasReimbursementStack create_gas_reimbursement_stack() {
    GasReimbursementStack stack;
    
    stack.estimator = std::make_shared<GasEstimator>();
    stack.manager = std::make_shared<GasReimbursementManager>();
    stack.pool_manager = std::make_shared<GasPoolManager>();
    stack.zk_reimbursement = std::make_shared<ZKGasReimbursement>();
    
    return stack;
}

} // namespace gas
} // namespace membra