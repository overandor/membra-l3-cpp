#ifndef ZERO_BALANCE_PRESERVATION_HPP
#define ZERO_BALANCE_PRESERVATION_HPP

#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <memory>
#include <map>
#include <chrono>
#include "crypto.hpp"
#include "gas_reimbursement.hpp"

namespace membra {
namespace preservation {

// Constants
constexpr size_t WALLET_ADDRESS_SIZE = 32;
constexpr uint64_t PRESERVATION_THRESHOLD_LAMPORTS = 5000; // Preserve at 5000 lamports
constexpr uint64_t CRITICAL_THRESHOLD_LAMPORTS = 1000; // Critical threshold
constexpr uint64_t ARCHIVAL_COOLDOWN_SECONDS = 86400; // 24 hours between archival operations

/**
 * Preservation status types
 */
enum class PreservationStatus {
    ACTIVE,           // Wallet is actively used
    MONITORED,        // Balance is being monitored
    PRESERVED,        // Wallet has been preserved (archived)
    RECOVERED,        // Wallet has been recovered from archival
    GOVERNANCE_LOCKED, // Locked by governance decision
    EMERGENCY_FROZEN  // Emergency freeze
};

/**
 * Preservation action types
 */
enum class PreservationAction {
    NONE,             // No action needed
    ARCHIVE,          // Archive wallet state
    RESTORE,          // Restore from archive
    FREEZE,           // Freeze wallet operations
    UNFREEZE,         // Unfreeze wallet
    TRANSFER_FUNDS,   // Transfer minimal funds to maintain threshold
    NOTIFY_USER,      // Notify user of low balance
    GOVERNANCE_REVIEW // Request governance review
};

/**
 * Wallet state snapshot for archival
 */
struct WalletSnapshot {
    std::string wallet_address;
    uint64_t balance;
    uint64_t transaction_count;
    std::string last_transaction_hash;
    int64_t last_activity_timestamp;
    std::string associated_account; // Associated user account if any
    std::map<std::string, std::string> metadata;
    std::array<uint8_t, 32> state_hash;
    int64_t snapshot_timestamp;
    
    WalletSnapshot()
        : balance(0),
          transaction_count(0),
          last_activity_timestamp(0),
          snapshot_timestamp(0) {
        state_hash.fill(0);
    }
};

/**
 * Preservation record
 */
struct PreservationRecord {
    std::string record_id;
    std::string wallet_address;
    PreservationStatus status;
    PreservationAction action_taken;
    uint64_t balance_at_preservation;
    uint64_t preservation_amount; // Amount added to maintain threshold
    int64_t preservation_timestamp;
    int64_t recovery_timestamp;
    std::string governance_signature;
    std::string recovery_key; // Encrypted recovery key
    std::string archive_location; // IPFS hash or storage reference
    std::map<std::string, std::string> metadata;
    
    PreservationRecord()
        : status(PreservationStatus::ACTIVE),
          action_taken(PreservationAction::NONE),
          balance_at_preservation(0),
          preservation_amount(0),
          preservation_timestamp(0),
          recovery_timestamp(0) {}
};

/**
 * Preservation policy configuration
 */
struct PreservationPolicy {
    std::string policy_id;
    std::uint64_t preservation_threshold;
    std::uint64_t critical_threshold;
    std::uint64_t minimum_maintenance_amount;
    bool auto_preserve_enabled;
    bool auto_restore_enabled;
    bool governance_approval_required;
    std::uint64_t archival_cooldown_seconds;
    std::vector<std::string> exempt_wallets; // Wallets exempt from preservation
    std::vector<std::string> priority_wallets; // High-priority wallets for preservation
    
    PreservationPolicy()
        : policy_id("default"),
          preservation_threshold(PRESERVATION_THRESHOLD_LAMPORTS),
          critical_threshold(CRITICAL_THRESHOLD_LAMPORTS),
          minimum_maintenance_amount(1000),
          auto_preserve_enabled(true),
          auto_restore_enabled(false),
          governance_approval_required(false),
          archival_cooldown_seconds(ARCHIVAL_COOLDOWN_SECONDS) {}
};

/**
 * Wallet preservation monitor
 */
class WalletPreservationMonitor {
public:
    WalletPreservationMonitor();
    ~WalletPreservationMonitor() = default;
    
    // Monitor wallet balance
    PreservationAction check_wallet_status(const std::string& wallet_address, uint64_t current_balance);
    
    // Create wallet snapshot for archival
    WalletSnapshot create_snapshot(const std::string& wallet_address, uint64_t current_balance);
    
    // Update wallet state
    void update_wallet_state(const std::string& wallet_address, uint64_t new_balance);
    
    // Get wallet status
    PreservationStatus get_wallet_status(const std::string& wallet_address);
    
    // Check if wallet needs preservation
    bool needs_preservation(const std::string& wallet_address, uint64_t current_balance);
    
    // Check if wallet is exempt
    bool is_exempt(const std::string& wallet_address);
    
    // Set preservation policy
    void set_policy(const PreservationPolicy& policy);
    
    // Get current policy
    PreservationPolicy get_policy() const;
    
private:
    std::map<std::string, WalletSnapshot> wallet_snapshots_;
    std::map<std::string, PreservationStatus> wallet_statuses_;
    PreservationPolicy policy_;
    std::unique_ptr<Crypto> crypto_;
    
public:
    // Direct access for testing (in production would use proper getters/setters)
    std::map<std::string, PreservationStatus>& get_wallet_statuses() { return wallet_statuses_; }
    
private:
    
    // Generate state hash
    std::array<uint8_t, 32> generate_state_hash(const WalletSnapshot& snapshot);
};

/**
 * Wallet preservation manager
 */
class WalletPreservationManager {
public:
    WalletPreservationManager();
    ~WalletPreservationManager() = default;
    
    // Preserve wallet (archive)
    std::string preserve_wallet(const std::string& wallet_address, uint64_t current_balance);
    
    // Restore wallet from archival
    bool restore_wallet(const std::string& record_id, const std::string& recovery_key);
    
    // Transfer minimal funds to maintain threshold
    bool transfer_maintenance_funds(const std::string& wallet_address, uint64_t amount);
    
    // Freeze wallet operations
    bool freeze_wallet(const std::string& wallet_address, const std::string& reason);
    
    // Unfreeze wallet operations
    bool unfreeze_wallet(const std::string& wallet_address);
    
    // Request governance review
    std::string request_governance_review(const std::string& wallet_address, const std::string& reason);
    
    // Get preservation record
    PreservationRecord get_preservation_record(const std::string& record_id);
    
    // Get wallet preservation history
    std::vector<PreservationRecord> get_wallet_history(const std::string& wallet_address);
    
    // Get all preserved wallets
    std::vector<PreservationRecord> get_all_preserved();
    
    // Statistics
    struct PreservationStats {
        uint64_t total_preservations;
        uint64_t total_restorations;
        uint64_t total_funds_transferred;
        uint64_t total_frozen;
        uint64_t active_preservations;
        double average_preservation_time_hours;
    };
    PreservationStats get_stats() const;
    
private:
    std::map<std::string, PreservationRecord> preservation_records_;
    std::shared_ptr<WalletPreservationMonitor> monitor_;
    std::unique_ptr<Crypto> crypto_;
    
    PreservationStats stats_;
    
    // Generate record ID
    std::string generate_record_id();
    
    // Archive wallet snapshot to storage
    std::string archive_snapshot(const WalletSnapshot& snapshot);
    
    // Update statistics
    void update_stats(const PreservationRecord& record);
};

/**
 * Recovery service for preserved wallets
 */
class WalletRecoveryService {
public:
    WalletRecoveryService();
    ~WalletRecoveryService() = default;
    
    // Initiate recovery process
    std::string initiate_recovery(const std::string& record_id, const std::string& user_signature);
    
    // Verify recovery request
    bool verify_recovery_request(const std::string& recovery_id);
    
    // Complete recovery
    bool complete_recovery(const std::string& recovery_id, const std::string& recovery_key);
    
    // Generate recovery key
    std::string generate_recovery_key(const std::string& wallet_address);
    
    // Validate recovery key
    bool validate_recovery_key(const std::string& recovery_key);
    
    // Get recovery status
    struct RecoveryStatus {
        std::string recovery_id;
        std::string record_id;
        std::string status; // "pending", "verified", "completed", "failed"
        int64_t initiated_at;
        int64_t completed_at;
        std::string verification_method;
    };
    RecoveryStatus get_recovery_status(const std::string& recovery_id);
    
private:
    std::map<std::string, RecoveryStatus> recovery_requests_;
    std::unique_ptr<Crypto> crypto_;
};

/**
 * Governance integration for preservation decisions
 */
class PreservationGovernance {
public:
    PreservationGovernance();
    ~PreservationGovernance() = default;
    
    // Request approval for preservation action
    std::string request_approval(const std::string& wallet_address, PreservationAction action);
    
    // Submit governance decision
    bool submit_decision(const std::string& request_id, bool approved, const std::string& signature);
    
    // Get approval status
    struct ApprovalStatus {
        std::string request_id;
        std::string wallet_address;
        PreservationAction action;
        std::string status; // "pending", "approved", "rejected"
        std::string governance_signature;
        int64_t requested_at;
        int64_t decided_at;
    };
    ApprovalStatus get_approval_status(const std::string& request_id);
    
    // Check if governance approval is required
    bool requires_approval(const std::string& wallet_address, PreservationAction action);
    
private:
    std::map<std::string, ApprovalStatus> approval_requests_;
    std::unique_ptr<Crypto> crypto_;
};

/**
 * Zero balance preservation factory
 */
struct PreservationStack {
    std::shared_ptr<WalletPreservationMonitor> monitor;
    std::shared_ptr<WalletPreservationManager> manager;
    std::shared_ptr<WalletRecoveryService> recovery_service;
    std::shared_ptr<PreservationGovernance> governance;
};

PreservationStack create_preservation_stack();

} // namespace preservation
} // namespace membra

#endif // ZERO_BALANCE_PRESERVATION_HPP