#include "zk_compute.hpp"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>

namespace membra {
namespace zkcompute {

// ============================================================================
// ZKProver Implementation
// ============================================================================

ZKProver::ZKProver()
    : prover_endpoint_("http://localhost:8080"),
      crypto_(std::make_unique<Crypto>()) {
}

ZKComputeProof ZKProver::generate_proof(const ComputeTask& task,
                                      const ResourceCommitment& commitment) {
    ZKComputeProof proof;
    
    // Generate task ID if not provided
    proof.task_id = task.task_id.empty() ? 
        "task_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) :
        task.task_id;
    
    proof.prover_endpoint = prover_endpoint_;
    proof.compute_type = task.compute_type;
    
    // Hash input data
    proof.input_hash = crypto_->sha256(task.input_data);
    
    // Hash resource commitment
    std::vector<uint8_t> commitment_data;
    commitment_data.insert(commitment_data.end(), commitment.commitment_id.begin(), commitment.commitment_id.end());
    proof.resource_commitment = crypto_->sha256(commitment_data);
    
    // Generate mock proof (in production, call actual ZK prover)
    proof.proof_hash = generate_mock_proof(task);
    
    // Set circuit identifier
    proof.circuit_identifier = task.circuit_identifier;
    
    // Set initial status
    proof.verification_status = "pending";
    
    // Set timestamp
    auto now = std::chrono::system_clock::now();
    proof.created_at = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()
    ).count();
    
    return proof;
}

VerificationResult ZKProver::verify_proof(const ZKComputeProof& proof) {
    VerificationResult result;
    
    // Mock verification (in production, call actual ZK verifier)
    result.success = true;
    result.verification_status = "verified";
    result.verification_time_ms = 100 + (rand() % 200); // 100-300ms
    result.gas_used = 50000 + (rand() % 100000); // 50k-150k gas
    
    // Hash the proof for verification
    std::vector<uint8_t> proof_data;
    proof_data.insert(proof_data.end(), proof.task_id.begin(), proof.task_id.end());
    proof_data.insert(proof_data.end(), proof.proof_hash.begin(), proof.proof_hash.end());
    result.verified_hash = crypto_->sha256(proof_data);
    
    // Check proof hash consistency
    if (proof.proof_hash[0] == 0) {
        result.success = false;
        result.verification_status = "invalid_proof";
        result.error_message = "Proof hash is empty";
    }
    
    return result;
}

std::string ZKProver::get_endpoint_info() const {
    return prover_endpoint_;
}

void ZKProver::set_endpoint(const std::string& endpoint) {
    prover_endpoint_ = endpoint;
}

std::array<uint8_t, HASH_SIZE> ZKProver::generate_mock_proof(const ComputeTask& task) {
    // Generate mock proof hash based on task data
    std::vector<uint8_t> proof_data;
    proof_data.insert(proof_data.end(), task.task_id.begin(), task.task_id.end());
    proof_data.insert(proof_data.end(), task.input_data.begin(), task.input_data.end());
    proof_data.push_back(static_cast<uint8_t>(task.compute_type));
    
    return crypto_->sha256(proof_data);
}

// ============================================================================
// ZKComputeManager Implementation
// ============================================================================

ZKComputeManager::ZKComputeManager()
    : prover_(std::make_shared<ZKProver>()),
      merkle_builder_(std::make_unique<tokenomics::MerkleTreeBuilder>()),
      crypto_(std::make_unique<Crypto>()) {
    
    stats_ = {0, 0, 0, 0, 0.0};
}

std::string ZKComputeManager::submit_task(const ComputeTask& task,
                                        const ResourceCommitment& commitment) {
    // Generate proof
    ZKComputeProof proof = prover_->generate_proof(task, commitment);
    
    // Store proof
    proofs_[proof.task_id] = proof;
    
    // Update statistics
    stats_.total_tasks_submitted++;
    stats_.total_compute_units += task.required_compute_units;
    
    return proof.task_id;
}

ZKComputeProof ZKComputeManager::get_task_status(const std::string& task_id) {
    auto it = proofs_.find(task_id);
    if (it != proofs_.end()) {
        return it->second;
    }
    
    ZKComputeProof empty_proof;
    empty_proof.task_id = task_id;
    empty_proof.verification_status = "not_found";
    return empty_proof;
}

VerificationResult ZKComputeManager::verify_task_completion(const std::string& task_id) {
    auto it = proofs_.find(task_id);
    if (it == proofs_.end()) {
        VerificationResult result;
        result.success = false;
        result.verification_status = "task_not_found";
        return result;
    }
    
    ZKComputeProof& proof = it->second;
    
    // Verify the proof
    VerificationResult verification = prover_->verify_proof(proof);
    
    if (verification.success) {
        proof.verification_status = verification.verification_status;
        proof.verified_at = std::chrono::system_clock::now().time_since_epoch().count();
        proof.output_hash = verification.verified_hash;
        
        // Calculate reward
        ComputeTask mock_task; // Would retrieve actual task
        mock_task.compute_type = proof.compute_type;
        proof.reward_amount = calculate_reward(mock_task, verification);
        
        // Update statistics
        stats_.total_tasks_verified++;
        stats_.total_rewards_paid += proof.reward_amount;
        
        double total_time = stats_.average_verification_time_ms * (stats_.total_tasks_verified - 1) + 
                           verification.verification_time_ms;
        stats_.average_verification_time_ms = total_time / stats_.total_tasks_verified;
    }
    
    return verification;
}

uint64_t ZKComputeManager::calculate_reward(const ComputeTask& task,
                                          const VerificationResult& verification) {
    if (!verification.success) {
        return 0;
    }
    
    // Base reward based on compute type
    uint64_t base_reward = DEFAULT_REWARD_AMOUNT;
    
    switch (task.compute_type) {
        case ComputeType::LLM_INFERENCE:
            base_reward = 2000000; // 0.002 SOL
            break;
        case ComputeType::MERKLE_COMPUTATION:
            base_reward = 1500000; // 0.0015 SOL
            break;
        case ComputeType::MICROTASK_COMPLETION:
            base_reward = 500000; // 0.0005 SOL
            break;
        case ComputeType::COLLATERAL_LOCKING:
            base_reward = 1000000; // 0.001 SOL
            break;
        case ComputeType::GAS_REIMBURSEMENT:
            base_reward = 750000; // 0.00075 SOL
            break;
        case ComputeType::IDO_APPRAISAL:
            base_reward = 3000000; // 0.003 SOL
            break;
        case ComputeType::CIRCUIT_COMPILATION:
            base_reward = 2500000; // 0.0025 SOL
            break;
        case ComputeType::PROOF_VERIFICATION:
            base_reward = 500000; // 0.0005 SOL
            break;
    }
    
    // Adjust based on compute complexity
    uint64_t complexity_multiplier = (task.required_compute_units / 1000);
    uint64_t reward = base_reward * complexity_multiplier;
    
    // Adjust based on verification speed (faster = higher reward)
    double speed_bonus = std::max(1.0, 1000.0 / verification.verification_time_ms);
    reward = static_cast<uint64_t>(reward * speed_bonus);
    
    return reward;
}

std::vector<ZKComputeProof> ZKComputeManager::get_pending_tasks() {
    std::vector<ZKComputeProof> pending;
    
    for (const auto& [task_id, proof] : proofs_) {
        if (proof.verification_status == "pending") {
            pending.push_back(proof);
        }
    }
    
    return pending;
}

std::vector<ZKComputeProof> ZKComputeManager::get_verified_proofs() {
    std::vector<ZKComputeProof> verified;
    
    for (const auto& [task_id, proof] : proofs_) {
        if (proof.verification_status == "verified") {
            verified.push_back(proof);
        }
    }
    
    return verified;
}

std::array<uint8_t, HASH_SIZE> ZKComputeManager::generate_merkle_root() {
    std::vector<ZKComputeProof> verified = get_verified_proofs();
    
    if (verified.empty()) {
        std::array<uint8_t, HASH_SIZE> empty_hash;
        empty_hash.fill(0);
        return empty_hash;
    }
    
    // Add verified proofs to merkle tree
    for (const auto& proof : verified) {
        std::vector<uint8_t> proof_data;
        proof_data.insert(proof_data.end(), proof.task_id.begin(), proof.task_id.end());
        proof_data.insert(proof_data.end(), proof.proof_hash.begin(), proof.proof_hash.end());
        merkle_builder_->add_leaf(proof_data);
    }
    
    return merkle_builder_->build_root();
}

ZKComputeManager::ComputeStats ZKComputeManager::get_stats() const {
    return stats_;
}

std::string ZKComputeManager::generate_task_id() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    
    std::array<uint8_t, HASH_SIZE> hash = crypto_->sha256(
        std::vector<uint8_t>(std::to_string(timestamp).begin(), std::to_string(timestamp).end())
    );
    
    char buf[33];
    for (size_t i = 0; i < 16; ++i) {
        snprintf(&buf[i * 2], 3, "%02x", hash[i]);
    }
    buf[32] = '\0';
    
    return std::string(buf) + "_zk_task";
}

void ZKComputeManager::update_stats(const ZKComputeProof& /* proof */) {
    // Statistics are updated in verify_task_completion
    // This is a placeholder for additional stat tracking
}

// ============================================================================
// ComputeResourceAllocator Implementation
// ============================================================================

ComputeResourceAllocator::ComputeResourceAllocator()
    : crypto_(std::make_unique<Crypto>()) {
    
    // Initialize with a mock node status
    NodeStatus mock_status;
    mock_status.node_id = "node_001";
    mock_status.available_cpu_cores = 8;
    mock_status.available_memory_mb = 16384;
    mock_status.gpu_available = true;
    mock_status.active_tasks = 0;
    mock_status.utilization_ratio = 0.0;
    
    node_statuses_[mock_status.node_id] = mock_status;
}

ResourceCommitment ComputeResourceAllocator::allocate_resources(const ComputeTask& task,
                                                                const std::string& node_id) {
    ResourceCommitment commitment;
    
    // Check availability
    if (!check_availability(task, node_id)) {
        commitment.commitment_id = "unavailable";
        return commitment;
    }
    
    // Generate commitment ID
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    
    std::string combined = node_id + std::to_string(timestamp);
    std::array<uint8_t, HASH_SIZE> hash = crypto_->sha256(
        std::vector<uint8_t>(combined.begin(), combined.end())
    );
    
    char buf[33];
    for (size_t i = 0; i < 16; ++i) {
        snprintf(&buf[i * 2], 3, "%02x", hash[i]);
    }
    buf[32] = '\0';
    
    commitment.commitment_id = std::string(buf);
    commitment.node_id = node_id;
    commitment.cpu_cores_committed = std::min(
        task.required_compute_units / 1000,
        node_statuses_[node_id].available_cpu_cores
    );
    commitment.memory_mb_committed = std::min(
        task.required_compute_units / 10,
        node_statuses_[node_id].available_memory_mb
    );
    commitment.gpu_committed = node_statuses_[node_id].gpu_available;
    commitment.duration_seconds = task.timeout_seconds;
    commitment.expires_at = std::chrono::system_clock::now().time_since_epoch().count() + 
                          task.timeout_seconds;
    
    // Hash commitment
    std::vector<uint8_t> commitment_data;
    commitment_data.insert(commitment_data.end(), commitment.commitment_id.begin(), commitment.commitment_id.end());
    
    uint8_t* cpu_ptr = reinterpret_cast<uint8_t*>(&commitment.cpu_cores_committed);
    commitment_data.insert(commitment_data.end(), cpu_ptr, cpu_ptr + sizeof(commitment.cpu_cores_committed));
    
    uint8_t* mem_ptr = reinterpret_cast<uint8_t*>(&commitment.memory_mb_committed);
    commitment_data.insert(commitment_data.end(), mem_ptr, mem_ptr + sizeof(commitment.memory_mb_committed));
    
    commitment.commitment_hash = crypto_->sha256(commitment_data);
    
    // Store commitment
    commitments_[commitment.commitment_id] = commitment;
    
    // Update node status
    node_statuses_[node_id].available_cpu_cores -= commitment.cpu_cores_committed;
    node_statuses_[node_id].available_memory_mb -= commitment.memory_mb_committed;
    node_statuses_[node_id].active_tasks++;
    node_statuses_[node_id].utilization_ratio = 
        static_cast<double>(node_statuses_[node_id].active_tasks) / 8.0;
    
    return commitment;
}

void ComputeResourceAllocator::release_resources(const std::string& commitment_id) {
    auto it = commitments_.find(commitment_id);
    if (it == commitments_.end()) {
        return;
    }
    
    ResourceCommitment& commitment = it->second;
    
    // Release resources back to node
    auto& node_status = node_statuses_[commitment.node_id];
    node_status.available_cpu_cores += commitment.cpu_cores_committed;
    node_status.available_memory_mb += commitment.memory_mb_committed;
    node_status.active_tasks--;
    node_status.utilization_ratio = 
        static_cast<double>(node_status.active_tasks) / 8.0;
    
    // Remove commitment
    commitments_.erase(it);
}

bool ComputeResourceAllocator::check_availability(const ComputeTask& task,
                                               const std::string& node_id) {
    auto it = node_statuses_.find(node_id);
    if (it == node_statuses_.end()) {
        return false;
    }
    
    const NodeStatus& status = it->second;
    
    // Check if sufficient resources are available
    uint64_t required_cpu = task.required_compute_units / 1000;
    uint64_t required_memory = task.required_compute_units / 10;
    
    return status.available_cpu_cores >= required_cpu &&
           status.available_memory_mb >= required_memory &&
           (!task.circuit_identifier.empty() || status.gpu_available);
}

ComputeResourceAllocator::NodeStatus ComputeResourceAllocator::get_node_status(const std::string& node_id) {
    auto it = node_statuses_.find(node_id);
    if (it != node_statuses_.end()) {
        return it->second;
    }
    
    NodeStatus empty_status;
    empty_status.node_id = node_id;
    return empty_status;
}

// ============================================================================
// RewardDistributor Implementation
// ============================================================================

RewardDistributor::RewardDistributor()
    : total_rewards_distributed_(0),
      crypto_(std::make_unique<Crypto>()) {
}

bool RewardDistributor::distribute_reward(const ZKComputeProof& proof,
                                        const std::string& recipient_wallet) {
    if (proof.verification_status != "verified") {
        return false;
    }
    
    // Add reward to recipient's balance
    reward_balances_[recipient_wallet] += proof.reward_amount;
    total_rewards_distributed_ += proof.reward_amount;
    
    return true;
}

uint64_t RewardDistributor::calculate_reward_amount(const ComputeTask& task,
                                                const VerificationResult& verification) {
    if (!verification.success) {
        return 0;
    }
    
    // Base reward based on compute type
    uint64_t base_reward = DEFAULT_REWARD_AMOUNT;
    
    switch (task.compute_type) {
        case ComputeType::LLM_INFERENCE:
            base_reward = 2000000;
            break;
        case ComputeType::MERKLE_COMPUTATION:
            base_reward = 1500000;
            break;
        case ComputeType::MICROTASK_COMPLETION:
            base_reward = 500000;
            break;
        case ComputeType::COLLATERAL_LOCKING:
            base_reward = 1000000;
            break;
        case ComputeType::GAS_REIMBURSEMENT:
            base_reward = 750000;
            break;
        case ComputeType::IDO_APPRAISAL:
            base_reward = 3000000;
            break;
        case ComputeType::CIRCUIT_COMPILATION:
            base_reward = 2500000;
            break;
        case ComputeType::PROOF_VERIFICATION:
            base_reward = 500000;
            break;
    }
    
    // Adjust for compute complexity
    uint64_t complexity_multiplier = (task.required_compute_units / 1000);
    return base_reward * complexity_multiplier;
}

std::map<std::string, uint64_t> RewardDistributor::get_reward_history(const std::string& wallet) {
    std::map<std::string, uint64_t> history;
    
    if (!wallet.empty()) {
        // Return all rewards
        return reward_balances_;
    }
    
    // Return rewards for specific wallet
    auto it = reward_balances_.find(wallet);
    if (it != reward_balances_.end()) {
        history[wallet] = it->second;
    }
    
    return history;
}

uint64_t RewardDistributor::get_total_rewards_distributed() const {
    return total_rewards_distributed_;
}

// ============================================================================
// Factory Function Implementation
// ============================================================================

ZKComputeStack create_zk_compute_stack() {
    ZKComputeStack stack;
    
    stack.prover = std::make_shared<ZKProver>();
    stack.manager = std::make_shared<ZKComputeManager>();
    stack.allocator = std::make_shared<ComputeResourceAllocator>();
    stack.distributor = std::make_shared<RewardDistributor>();
    
    return stack;
}

} // namespace zkcompute
} // namespace membra