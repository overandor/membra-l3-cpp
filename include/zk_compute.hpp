#ifndef ZK_COMPUTE_HPP
#define ZK_COMPUTE_HPP

#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <memory>
#include <map>
#include <chrono>
#include "crypto.hpp"
#include "merkle_provenance.hpp"

namespace membra {
namespace zkcompute {

// Constants
constexpr size_t HASH_SIZE = 32;
constexpr size_t TASK_ID_SIZE = 32;
constexpr uint64_t DEFAULT_REWARD_AMOUNT = 1000000; // 0.001 SOL in lamports

/**
 * Compute types for ZK proofs
 */
enum class ComputeType {
    LLM_INFERENCE,
    MERKLE_COMPUTATION,
    MICROTASK_COMPLETION,
    COLLATERAL_LOCKING,
    GAS_REIMBURSEMENT,
    IDO_APPRAISAL,
    CIRCUIT_COMPILATION,
    PROOF_VERIFICATION
};

/**
 * ZK compute proof structure
 */
struct ZKComputeProof {
    std::string task_id;
    std::string prover_endpoint;
    ComputeType compute_type;
    std::array<uint8_t, HASH_SIZE> input_hash;
    std::array<uint8_t, HASH_SIZE> output_hash;
    std::array<uint8_t, HASH_SIZE> resource_commitment;
    std::array<uint8_t, HASH_SIZE> proof_hash;
    std::string verification_status; // "pending", "verified", "failed"
    uint64_t reward_amount;
    int64_t created_at;
    int64_t verified_at;
    std::string circuit_identifier;
    std::map<std::string, std::string> metadata;
    
    ZKComputeProof()
        : compute_type(ComputeType::LLM_INFERENCE),
          verification_status("pending"),
          reward_amount(DEFAULT_REWARD_AMOUNT),
          created_at(0),
          verified_at(0) {
        input_hash.fill(0);
        output_hash.fill(0);
        resource_commitment.fill(0);
        proof_hash.fill(0);
    }
};

/**
 * Compute task specification
 */
struct ComputeTask {
    std::string task_id;
    ComputeType compute_type;
    std::vector<uint8_t> input_data;
    std::string circuit_identifier;
    uint64_t required_compute_units;
    uint64_t timeout_seconds;
    std::string requester_wallet;
    int64_t created_at;
    
    ComputeTask()
        : compute_type(ComputeType::LLM_INFERENCE),
          required_compute_units(1000),
          timeout_seconds(300),
          created_at(0) {}
};

/**
 * Resource commitment for compute
 */
struct ResourceCommitment {
    std::string commitment_id;
    std::string node_id;
    uint64_t cpu_cores_committed;
    uint64_t memory_mb_committed;
    bool gpu_committed;
    uint64_t duration_seconds;
    std::array<uint8_t, HASH_SIZE> commitment_hash;
    int64_t expires_at;
    
    ResourceCommitment()
        : cpu_cores_committed(0),
          memory_mb_committed(0),
          gpu_committed(false),
          duration_seconds(0),
          expires_at(0) {
        commitment_hash.fill(0);
    }
};

/**
 * ZK proof verification result
 */
struct VerificationResult {
    bool success;
    std::string verification_status;
    std::string error_message;
    uint64_t gas_used;
    uint64_t verification_time_ms;
    std::array<uint8_t, HASH_SIZE> verified_hash;
    
    VerificationResult()
        : success(false),
          verification_status("failed"),
          gas_used(0),
          verification_time_ms(0) {
        verified_hash.fill(0);
    }
};

/**
 * ZK compute prover interface
 */
class ZKProver {
public:
    ZKProver();
    ~ZKProver() = default;
    
    // Generate proof for compute task
    ZKComputeProof generate_proof(const ComputeTask& task,
                                  const ResourceCommitment& commitment);
    
    // Verify proof
    VerificationResult verify_proof(const ZKComputeProof& proof);
    
    // Get prover endpoint info
    std::string get_endpoint_info() const;
    
    // Set prover endpoint
    void set_endpoint(const std::string& endpoint);
    
private:
    std::string prover_endpoint_;
    std::unique_ptr<Crypto> crypto_;
    
    // Mock proof generation (in production, call actual ZK prover)
    std::array<uint8_t, HASH_SIZE> generate_mock_proof(const ComputeTask& task);
};

/**
 * ZK compute manager
 */
class ZKComputeManager {
public:
    ZKComputeManager();
    ~ZKComputeManager() = default;
    
    // Submit compute task
    std::string submit_task(const ComputeTask& task,
                           const ResourceCommitment& commitment);
    
    // Get task status
    ZKComputeProof get_task_status(const std::string& task_id);
    
    // Verify task completion
    VerificationResult verify_task_completion(const std::string& task_id);
    
    // Calculate reward based on compute type and complexity
    uint64_t calculate_reward(const ComputeTask& task,
                               const VerificationResult& verification);
    
    // Get pending tasks
    std::vector<ZKComputeProof> get_pending_tasks();
    
    // Get verified proofs
    std::vector<ZKComputeProof> get_verified_proofs();
    
    // Generate merkle root of verified proofs
    std::array<uint8_t, HASH_SIZE> generate_merkle_root();
    
    // Statistics
    struct ComputeStats {
        uint64_t total_tasks_submitted;
        uint64_t total_tasks_verified;
        uint64_t total_rewards_paid;
        uint64_t total_compute_units;
        double average_verification_time_ms;
    };
    ComputeStats get_stats() const;
    
private:
    std::map<std::string, ZKComputeProof> proofs_;
    std::shared_ptr<ZKProver> prover_;
    std::unique_ptr<tokenomics::MerkleTreeBuilder> merkle_builder_;
    std::unique_ptr<Crypto> crypto_;
    
    ComputeStats stats_;
    
    // Generate task ID
    std::string generate_task_id();
    
    // Update statistics
    void update_stats(const ZKComputeProof& proof);
};

/**
 * Compute resource allocator
 */
class ComputeResourceAllocator {
public:
    ComputeResourceAllocator();
    ~ComputeResourceAllocator() = default;
    
    // Allocate resources for task
    ResourceCommitment allocate_resources(const ComputeTask& task,
                                          const std::string& node_id);
    
    // Release resources
    void release_resources(const std::string& commitment_id);
    
    // Check resource availability
    bool check_availability(const ComputeTask& task,
                             const std::string& node_id);
    
    // Get node status
    struct NodeStatus {
        std::string node_id;
        uint64_t available_cpu_cores;
        uint64_t available_memory_mb;
        bool gpu_available;
        uint64_t active_tasks;
        double utilization_ratio;
    };
    NodeStatus get_node_status(const std::string& node_id);
    
private:
    std::map<std::string, ResourceCommitment> commitments_;
    std::map<std::string, NodeStatus> node_statuses_;
    std::unique_ptr<Crypto> crypto_;
};

/**
 * Reward distributor
 */
class RewardDistributor {
public:
    RewardDistributor();
    ~RewardDistributor() = default;
    
    // Distribute reward for verified proof
    bool distribute_reward(const ZKComputeProof& proof,
                          const std::string& recipient_wallet);
    
    // Calculate reward amount
    uint64_t calculate_reward_amount(const ComputeTask& task,
                                    const VerificationResult& verification);
    
    // Get reward distribution history
    std::map<std::string, uint64_t> get_reward_history(const std::string& wallet);
    
    // Get total rewards distributed
    uint64_t get_total_rewards_distributed() const;
    
private:
    std::map<std::string, uint64_t> reward_balances_;
    uint64_t total_rewards_distributed_;
    std::unique_ptr<Crypto> crypto_;
};

/**
 * ZK compute integration factory
 */
struct ZKComputeStack {
    std::shared_ptr<ZKProver> prover;
    std::shared_ptr<ZKComputeManager> manager;
    std::shared_ptr<ComputeResourceAllocator> allocator;
    std::shared_ptr<RewardDistributor> distributor;
};

ZKComputeStack create_zk_compute_stack();

} // namespace zkcompute
} // namespace membra

#endif // ZK_COMPUTE_HPP