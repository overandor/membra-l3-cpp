#include "zero_balance_preservation.hpp"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>

namespace membra {
namespace preservation {

// ============================================================================
// WalletPreservationMonitor Implementation
// ============================================================================

WalletPreservationMonitor::WalletPreservationMonitor()
    : crypto_(std::make_unique<Crypto>()) {
    
    // Initialize default policy
    policy_ = PreservationPolicy();
}

PreservationAction WalletPreservationMonitor::check_wallet_status(const std::string& wallet_address, uint64_t current_balance) {
    // Check if wallet is exempt
    if (is_exempt(wallet_address)) {
        return PreservationAction::NONE;
    }
    
    // Check current balance against thresholds
    if (current_balance <= policy_.critical_threshold) {
        // Critical threshold - immediate preservation needed
        return PreservationAction::TRANSFER_FUNDS;
    } else if (current_balance <= policy_.preservation_threshold) {
        // Below preservation threshold - monitor and prepare for archival
        return PreservationAction::NOTIFY_USER;
    } else if (current_balance < policy_.preservation_threshold * 2) {
        // Approaching threshold - monitor
        return PreservationAction::NONE;
    }
    
    return PreservationAction::NONE;
}

WalletSnapshot WalletPreservationMonitor::create_snapshot(const std::string& wallet_address, uint64_t current_balance) {
    WalletSnapshot snapshot;
    snapshot.wallet_address = wallet_address;
    snapshot.balance = current_balance;
    snapshot.transaction_count = 0; // Would be fetched from blockchain
    snapshot.last_activity_timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    snapshot.snapshot_timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    
    // Generate state hash
    snapshot.state_hash = generate_state_hash(snapshot);
    
    // Store snapshot
    wallet_snapshots_[wallet_address] = snapshot;
    
    return snapshot;
}

void WalletPreservationMonitor::update_wallet_state(const std::string& wallet_address, uint64_t new_balance) {
    auto it = wallet_snapshots_.find(wallet_address);
    if (it != wallet_snapshots_.end()) {
        it->second.balance = new_balance;
        it->second.last_activity_timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    } else {
        // Create new snapshot if wallet not monitored
        create_snapshot(wallet_address, new_balance);
    }
    
    // Update status based on new balance
    PreservationAction action = check_wallet_status(wallet_address, new_balance);
    if (action != PreservationAction::NONE) {
        get_wallet_statuses()[wallet_address] = PreservationStatus::MONITORED;
    } else {
        get_wallet_statuses()[wallet_address] = PreservationStatus::ACTIVE;
    }
}

PreservationStatus WalletPreservationMonitor::get_wallet_status(const std::string& wallet_address) {
    auto it = get_wallet_statuses().find(wallet_address);
    if (it != get_wallet_statuses().end()) {
        return it->second;
    }
    
    return PreservationStatus::ACTIVE; // Default to active if unknown
}

bool WalletPreservationMonitor::needs_preservation(const std::string& wallet_address, uint64_t current_balance) {
    return current_balance <= policy_.preservation_threshold && !is_exempt(wallet_address);
}

bool WalletPreservationMonitor::is_exempt(const std::string& wallet_address) {
    return std::find(policy_.exempt_wallets.begin(), policy_.exempt_wallets.end(), wallet_address) 
           != policy_.exempt_wallets.end();
}

void WalletPreservationMonitor::set_policy(const PreservationPolicy& policy) {
    policy_ = policy;
}

PreservationPolicy WalletPreservationMonitor::get_policy() const {
    return policy_;
}

std::array<uint8_t, 32> WalletPreservationMonitor::generate_state_hash(const WalletSnapshot& snapshot) {
    std::vector<uint8_t> state_data;
    state_data.insert(state_data.end(), snapshot.wallet_address.begin(), snapshot.wallet_address.end());
    
    // Add balance to hash
    uint64_t balance = snapshot.balance;
    uint8_t* balance_ptr = reinterpret_cast<uint8_t*>(&balance);
    state_data.insert(state_data.end(), balance_ptr, balance_ptr + sizeof(balance));
    
    // Add timestamp
    int64_t timestamp = snapshot.snapshot_timestamp;
    uint8_t* timestamp_ptr = reinterpret_cast<uint8_t*>(&timestamp);
    state_data.insert(state_data.end(), timestamp_ptr, timestamp_ptr + sizeof(timestamp));
    
    return crypto_->sha256(state_data);
}

// ============================================================================
// WalletPreservationManager Implementation
// ============================================================================

WalletPreservationManager::WalletPreservationManager()
    : monitor_(std::make_shared<WalletPreservationMonitor>()),
      crypto_(std::make_unique<Crypto>()) {
    
    stats_ = {0, 0, 0, 0, 0, 0.0};
}

std::string WalletPreservationManager::preserve_wallet(const std::string& wallet_address, uint64_t current_balance) {
    std::string record_id = generate_record_id();
    
    // Create wallet snapshot
    WalletSnapshot snapshot = monitor_->create_snapshot(wallet_address, current_balance);
    
    // Archive snapshot
    std::string archive_location = archive_snapshot(snapshot);
    
    // Create preservation record
    PreservationRecord record;
    record.record_id = record_id;
    record.wallet_address = wallet_address;
    record.status = PreservationStatus::PRESERVED;
    record.action_taken = PreservationAction::ARCHIVE;
    record.balance_at_preservation = current_balance;
    record.preservation_amount = monitor_->get_policy().minimum_maintenance_amount;
    record.preservation_timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    record.archive_location = archive_location;
    
    preservation_records_[record_id] = record;
    
    // Update wallet status
    monitor_->update_wallet_state(wallet_address, current_balance);
    
    // Update statistics
    stats_.total_preservations++;
    stats_.active_preservations++;
    
    update_stats(record);
    
    return record_id;
}

bool WalletPreservationManager::restore_wallet(const std::string& record_id, const std::string& recovery_key) {
    auto it = preservation_records_.find(record_id);
    if (it == preservation_records_.end()) {
        return false;
    }
    
    auto& record = it->second;
    
    // Validate recovery key
    if (record.recovery_key != recovery_key) {
        return false;
    }
    
    // Update record
    record.status = PreservationStatus::RECOVERED;
    record.action_taken = PreservationAction::RESTORE;
    record.recovery_timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    
    // Update wallet status
    monitor_->update_wallet_state(record.wallet_address, record.balance_at_preservation);
    
    // Update statistics
    stats_.total_restorations++;
    stats_.active_preservations--;
    
    update_stats(record);
    
    return true;
}

bool WalletPreservationManager::transfer_maintenance_funds(const std::string& wallet_address, uint64_t amount) {
    // In a real implementation, this would interact with the gas reimbursement system
    // or directly transfer funds from a preservation pool
    
    // Update wallet state to reflect the transfer
    uint64_t current_balance = monitor_->create_snapshot(wallet_address, 0).balance;
    monitor_->update_wallet_state(wallet_address, current_balance + amount);
    
    stats_.total_funds_transferred += amount;
    
    return true;
}

bool WalletPreservationManager::freeze_wallet(const std::string& wallet_address, const std::string& reason) {
    auto it = preservation_records_.find(wallet_address);
    
    // Create preservation record for frozen wallet if not exists
    if (it == preservation_records_.end()) {
        std::string record_id = generate_record_id();
        
        PreservationRecord record;
        record.record_id = record_id;
        record.wallet_address = wallet_address;
        record.status = PreservationStatus::EMERGENCY_FROZEN;
        record.action_taken = PreservationAction::FREEZE;
        record.preservation_timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        record.metadata["freeze_reason"] = reason;
        
        preservation_records_[record_id] = record;
    } else {
        it->second.status = PreservationStatus::EMERGENCY_FROZEN;
        it->second.action_taken = PreservationAction::FREEZE;
        it->second.metadata["freeze_reason"] = reason;
    }
    
    // Update wallet status
    monitor_->get_wallet_statuses()[wallet_address] = PreservationStatus::EMERGENCY_FROZEN;
    
    stats_.total_frozen++;
    
    return true;
}

bool WalletPreservationManager::unfreeze_wallet(const std::string& wallet_address) {
    auto it = preservation_records_.find(wallet_address);
    if (it != preservation_records_.end()) {
        it->second.status = PreservationStatus::ACTIVE;
        it->second.action_taken = PreservationAction::UNFREEZE;
        
        // Update wallet status
        monitor_->get_wallet_statuses()[wallet_address] = PreservationStatus::ACTIVE;
        
        return true;
    }
    
    return false;
}

std::string WalletPreservationManager::request_governance_review(const std::string& wallet_address, const std::string& reason) {
    std::string request_id = "gov_review_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    
    // Create preservation record for governance review
    PreservationRecord record;
    record.record_id = request_id;
    record.wallet_address = wallet_address;
    record.status = PreservationStatus::GOVERNANCE_LOCKED;
    record.action_taken = PreservationAction::GOVERNANCE_REVIEW;
    record.preservation_timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    record.metadata["review_reason"] = reason;
    
    preservation_records_[request_id] = record;
    
    return request_id;
}

PreservationRecord WalletPreservationManager::get_preservation_record(const std::string& record_id) {
    auto it = preservation_records_.find(record_id);
    if (it != preservation_records_.end()) {
        return it->second;
    }
    
    PreservationRecord empty_record;
    empty_record.record_id = record_id;
    return empty_record;
}

std::vector<PreservationRecord> WalletPreservationManager::get_wallet_history(const std::string& wallet_address) {
    std::vector<PreservationRecord> history;
    
    for (const auto& [record_id, record] : preservation_records_) {
        if (record.wallet_address == wallet_address) {
            history.push_back(record);
        }
    }
    
    return history;
}

std::vector<PreservationRecord> WalletPreservationManager::get_all_preserved() {
    std::vector<PreservationRecord> preserved;
    
    for (const auto& [record_id, record] : preservation_records_) {
        if (record.status == PreservationStatus::PRESERVED) {
            preserved.push_back(record);
        }
    }
    
    return preserved;
}

WalletPreservationManager::PreservationStats WalletPreservationManager::get_stats() const {
    return stats_;
}

std::string WalletPreservationManager::generate_record_id() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    
    return "preserve_" + std::to_string(timestamp);
}

std::string WalletPreservationManager::archive_snapshot(const WalletSnapshot& snapshot) {
    // In a real implementation, this would upload to IPFS or other storage
    // For now, we return a mock hash
    
    std::vector<uint8_t> snapshot_data;
    snapshot_data.insert(snapshot_data.end(), snapshot.wallet_address.begin(), snapshot.wallet_address.end());
    
    uint64_t balance = snapshot.balance;
    uint8_t* balance_ptr = reinterpret_cast<uint8_t*>(&balance);
    snapshot_data.insert(snapshot_data.end(), balance_ptr, balance_ptr + sizeof(balance));
    
    std::array<uint8_t, 32> hash = crypto_->sha256(snapshot_data);
    
    char buf[65];
    for (size_t i = 0; i < 32; ++i) {
        snprintf(&buf[i * 2], 3, "%02x", hash[i]);
    }
    buf[64] = '\0';
    
    return std::string("ipfs://") + std::string(buf);
}

void WalletPreservationManager::update_stats(const PreservationRecord& /* record */) {
    // Statistics are updated in the individual methods
    // This is a placeholder for additional stat tracking
}

// ============================================================================
// WalletRecoveryService Implementation
// ============================================================================

WalletRecoveryService::WalletRecoveryService()
    : crypto_(std::make_unique<Crypto>()) {
}

std::string WalletRecoveryService::initiate_recovery(const std::string& record_id, const std::string& /* user_signature */) {
    std::string recovery_id = "recovery_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    
    RecoveryStatus status;
    status.recovery_id = recovery_id;
    status.record_id = record_id;
    status.status = "pending";
    status.initiated_at = std::chrono::system_clock::now().time_since_epoch().count();
    status.verification_method = "signature";
    
    recovery_requests_[recovery_id] = status;
    
    return recovery_id;
}

bool WalletRecoveryService::verify_recovery_request(const std::string& recovery_id) {
    auto it = recovery_requests_.find(recovery_id);
    if (it == recovery_requests_.end()) {
        return false;
    }
    
    auto& status = it->second;
    
    // Mock verification - in production would verify signature
    status.status = "verified";
    status.completed_at = std::chrono::system_clock::now().time_since_epoch().count();
    
    return true;
}

bool WalletRecoveryService::complete_recovery(const std::string& recovery_id, const std::string& recovery_key) {
    auto it = recovery_requests_.find(recovery_id);
    if (it == recovery_requests_.end()) {
        return false;
    }
    
    auto& status = it->second;
    
    if (status.status != "verified") {
        return false;
    }
    
    // Validate recovery key
    if (!validate_recovery_key(recovery_key)) {
        return false;
    }
    
    status.status = "completed";
    status.completed_at = std::chrono::system_clock::now().time_since_epoch().count();
    
    return true;
}

std::string WalletRecoveryService::generate_recovery_key(const std::string& wallet_address) {
    std::vector<uint8_t> key_data;
    key_data.insert(key_data.end(), wallet_address.begin(), wallet_address.end());
    
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    
    std::string timestamp_str = std::to_string(timestamp);
    key_data.insert(key_data.end(), timestamp_str.begin(), timestamp_str.end());
    
    std::array<uint8_t, 32> hash = crypto_->sha256(key_data);
    
    char buf[65];
    for (size_t i = 0; i < 32; ++i) {
        snprintf(&buf[i * 2], 3, "%02x", hash[i]);
    }
    buf[64] = '\0';
    
    return std::string(buf);
}

bool WalletRecoveryService::validate_recovery_key(const std::string& recovery_key) {
    // Basic validation - check length and format
    return recovery_key.length() == 64;
}

WalletRecoveryService::RecoveryStatus WalletRecoveryService::get_recovery_status(const std::string& recovery_id) {
    auto it = recovery_requests_.find(recovery_id);
    if (it != recovery_requests_.end()) {
        return it->second;
    }
    
    RecoveryStatus empty_status;
    empty_status.recovery_id = recovery_id;
    empty_status.status = "not_found";
    return empty_status;
}

// ============================================================================
// PreservationGovernance Implementation
// ============================================================================

PreservationGovernance::PreservationGovernance()
    : crypto_(std::make_unique<Crypto>()) {
}

std::string PreservationGovernance::request_approval(const std::string& wallet_address, PreservationAction action) {
    std::string request_id = "gov_approve_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    
    ApprovalStatus status;
    status.request_id = request_id;
    status.wallet_address = wallet_address;
    status.action = action;
    status.status = "pending";
    status.requested_at = std::chrono::system_clock::now().time_since_epoch().count();
    
    approval_requests_[request_id] = status;
    
    return request_id;
}

bool PreservationGovernance::submit_decision(const std::string& request_id, bool approved, const std::string& signature) {
    auto it = approval_requests_.find(request_id);
    if (it == approval_requests_.end()) {
        return false;
    }
    
    auto& status = it->second;
    status.status = approved ? "approved" : "rejected";
    status.governance_signature = signature;
    status.decided_at = std::chrono::system_clock::now().time_since_epoch().count();
    
    return true;
}

PreservationGovernance::ApprovalStatus PreservationGovernance::get_approval_status(const std::string& request_id) {
    auto it = approval_requests_.find(request_id);
    if (it != approval_requests_.end()) {
        return it->second;
    }
    
    ApprovalStatus empty_status;
    empty_status.request_id = request_id;
    empty_status.status = "not_found";
    return empty_status;
}

bool PreservationGovernance::requires_approval(const std::string& /* wallet_address */, PreservationAction action) {
    // Check if wallet is in priority list or requires governance
    // For now, we'll say governance approval is required for freezing actions
    return action == PreservationAction::FREEZE || action == PreservationAction::GOVERNANCE_REVIEW;
}

// ============================================================================
// Factory Function Implementation
// ============================================================================

PreservationStack create_preservation_stack() {
    PreservationStack stack;
    
    stack.monitor = std::make_shared<WalletPreservationMonitor>();
    stack.manager = std::make_shared<WalletPreservationManager>();
    stack.recovery_service = std::make_shared<WalletRecoveryService>();
    stack.governance = std::make_shared<PreservationGovernance>();
    
    return stack;
}

} // namespace preservation
} // namespace membra