#include "zk_compute.hpp"
#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>

using namespace membra::zkcompute;

void test_zk_prover() {
    std::cout << "Testing ZKProver..." << std::endl;
    
    ZKProver prover;
    
    // Test proof generation
    ComputeTask task;
    task.task_id = "test_task_001";
    task.compute_type = ComputeType::LLM_INFERENCE;
    task.input_data = {1, 2, 3, 4, 5};
    task.circuit_identifier = "llm_inference_circuit_v1";
    task.required_compute_units = 5000;
    
    ResourceCommitment commitment;
    commitment.commitment_id = "commitment_001";
    commitment.node_id = "node_001";
    commitment.cpu_cores_committed = 4;
    commitment.memory_mb_committed = 8192;
    
    ZKComputeProof proof = prover.generate_proof(task, commitment);
    
    assert(proof.task_id == "test_task_001");
    assert(proof.compute_type == ComputeType::LLM_INFERENCE);
    assert(proof.verification_status == "pending");
    assert(proof.created_at > 0);
    assert(!proof.proof_hash.empty());
    assert(proof.proof_hash[0] != 0);
    
    std::cout << "  ✓ Proof generation successful" << std::endl;
    
    // Test proof verification
    VerificationResult verification = prover.verify_proof(proof);
    
    assert(verification.success == true);
    assert(verification.verification_status == "verified");
    assert(verification.verification_time_ms > 0);
    assert(verification.gas_used > 0);
    
    std::cout << "  ✓ Proof verification successful" << std::endl;
    
    // Test endpoint management
    std::string original_endpoint = prover.get_endpoint_info();
    prover.set_endpoint("http://localhost:9090");
    assert(prover.get_endpoint_info() == "http://localhost:9090");
    
    std::cout << "  ✓ Endpoint management successful" << std::endl;
    
    std::cout << "ZKProver tests passed!" << std::endl;
}

void test_zk_compute_manager() {
    std::cout << "\nTesting ZKComputeManager..." << std::endl;
    
    ZKComputeManager manager;
    
    // Test task submission
    ComputeTask task;
    task.task_id = "manager_task_001";
    task.compute_type = ComputeType::MERKLE_COMPUTATION;
    task.input_data = {10, 20, 30, 40, 50};
    task.circuit_identifier = "merkle_circuit_v1";
    task.required_compute_units = 2000;
    
    ResourceCommitment commitment;
    commitment.commitment_id = "manager_commitment_001";
    commitment.node_id = "node_001";
    commitment.cpu_cores_committed = 2;
    commitment.memory_mb_committed = 4096;
    
    std::string task_id = manager.submit_task(task, commitment);
    assert(!task_id.empty());
    
    std::cout << "  ✓ Task submission successful" << std::endl;
    
    // Test task status retrieval
    ZKComputeProof proof = manager.get_task_status(task_id);
    assert(proof.task_id == task_id);
    assert(proof.verification_status == "pending");
    
    std::cout << "  ✓ Task status retrieval successful" << std::endl;
    
    // Test task verification
    VerificationResult verification = manager.verify_task_completion(task_id);
    assert(verification.success == true);
    assert(verification.verification_status == "verified");
    
    std::cout << "  ✓ Task verification successful" << std::endl;
    
    // Test updated proof status
    ZKComputeProof verified_proof = manager.get_task_status(task_id);
    assert(verified_proof.verification_status == "verified");
    assert(verified_proof.reward_amount > 0);
    
    std::cout << "  ✓ Proof status updated successfully" << std::endl;
    
    // Test reward calculation
    uint64_t reward = manager.calculate_reward(task, verification);
    assert(reward > 0);
    assert(reward >= DEFAULT_REWARD_AMOUNT);
    
    std::cout << "  ✓ Reward calculation successful" << std::endl;
    
    // Test pending tasks
    ComputeTask task2;
    task2.task_id = "pending_task_001";
    task2.compute_type = ComputeType::MICROTASK_COMPLETION;
    task2.input_data = {100, 200};
    task2.required_compute_units = 1000;
    
    ResourceCommitment commitment2;
    commitment2.commitment_id = "pending_commitment_001";
    commitment2.node_id = "node_001";
    
    std::string task_id2 = manager.submit_task(task2, commitment2);
    
    std::vector<ZKComputeProof> pending = manager.get_pending_tasks();
    assert(pending.size() >= 1);
    
    std::cout << "  ✓ Pending tasks retrieval successful" << std::endl;
    
    // Test verified proofs
    std::vector<ZKComputeProof> verified = manager.get_verified_proofs();
    assert(verified.size() >= 1);
    
    std::cout << "  ✓ Verified proofs retrieval successful" << std::endl;
    
    // Test merkle root generation
    auto merkle_root = manager.generate_merkle_root();
    assert(!merkle_root.empty());
    assert(merkle_root[0] != 0);
    
    std::cout << "  ✓ Merkle root generation successful" << std::endl;
    
    // Test statistics
    auto stats = manager.get_stats();
    assert(stats.total_tasks_submitted >= 2);
    assert(stats.total_tasks_verified >= 1);
    assert(stats.total_rewards_paid > 0);
    assert(stats.total_compute_units >= 3000);
    
    std::cout << "  ✓ Statistics tracking successful" << std::endl;
    
    std::cout << "ZKComputeManager tests passed!" << std::endl;
}

void test_compute_resource_allocator() {
    std::cout << "\nTesting ComputeResourceAllocator..." << std::endl;
    
    ComputeResourceAllocator allocator;
    
    // Test resource allocation
    ComputeTask task;
    task.task_id = "allocation_task_001";
    task.compute_type = ComputeType::LLM_INFERENCE;
    task.required_compute_units = 4000;
    task.timeout_seconds = 120;
    
    std::string node_id = "node_001";
    
    ResourceCommitment commitment = allocator.allocate_resources(task, node_id);
    assert(!commitment.commitment_id.empty());
    assert(commitment.node_id == node_id);
    assert(commitment.cpu_cores_committed > 0);
    assert(commitment.memory_mb_committed > 0);
    assert(commitment.expires_at > 0);
    
    std::cout << "  ✓ Resource allocation successful" << std::endl;
    
    // Test availability check
    bool available = allocator.check_availability(task, node_id);
    assert(available == true);
    
    std::cout << "  ✓ Availability check successful" << std::endl;
    
    // Test node status retrieval
    auto status = allocator.get_node_status(node_id);
    assert(status.node_id == node_id);
    assert(status.available_cpu_cores >= 0);
    assert(status.available_memory_mb >= 0);
    assert(status.active_tasks >= 1);
    
    std::cout << "  ✓ Node status retrieval successful" << std::endl;
    
    // Test resource release
    allocator.release_resources(commitment.commitment_id);
    
    auto status_after_release = allocator.get_node_status(node_id);
    assert(status_after_release.active_tasks < status.active_tasks);
    
    std::cout << "  ✓ Resource release successful" << std::endl;
    
    // Test overallocation (should fail)
    ComputeTask large_task;
    large_task.task_id = "large_task_001";
    large_task.compute_type = ComputeType::CIRCUIT_COMPILATION;
    large_task.required_compute_units = 1000000; // Very large
    large_task.timeout_seconds = 600;
    
    ResourceCommitment large_commitment = allocator.allocate_resources(large_task, node_id);
    assert(large_commitment.commitment_id == "unavailable");
    
    std::cout << "  ✓ Overallocation prevention successful" << std::endl;
    
    std::cout << "ComputeResourceAllocator tests passed!" << std::endl;
}

void test_reward_distributor() {
    std::cout << "\nTesting RewardDistributor..." << std::endl;
    
    RewardDistributor distributor;
    
    // Create a verified proof
    ZKComputeProof proof;
    proof.task_id = "reward_task_001";
    proof.compute_type = ComputeType::LLM_INFERENCE;
    proof.verification_status = "verified";
    proof.reward_amount = 2000000;
    
    // Test reward distribution
    std::string recipient_wallet = "recipient_wallet_001";
    bool distributed = distributor.distribute_reward(proof, recipient_wallet);
    assert(distributed == true);
    
    std::cout << "  ✓ Reward distribution successful" << std::endl;
    
    // Test reward history
    auto history = distributor.get_reward_history(recipient_wallet);
    assert(history.size() >= 1);
    assert(history[recipient_wallet] == proof.reward_amount);
    
    std::cout << "  ✓ Reward history retrieval successful" << std::endl;
    
    // Test total rewards
    uint64_t total = distributor.get_total_rewards_distributed();
    assert(total == proof.reward_amount);
    
    std::cout << "  ✓ Total rewards tracking successful" << std::endl;
    
    // Test reward amount calculation
    ComputeTask task;
    task.compute_type = ComputeType::IDO_APPRAISAL;
    task.required_compute_units = 3000;
    
    VerificationResult verification;
    verification.success = true;
    verification.verification_time_ms = 150;
    
    uint64_t calculated_reward = distributor.calculate_reward_amount(task, verification);
    assert(calculated_reward > 0);
    assert(calculated_reward >= DEFAULT_REWARD_AMOUNT);
    
    std::cout << "  ✓ Reward amount calculation successful" << std::endl;
    
    // Test failed verification (no reward)
    VerificationResult failed_verification;
    failed_verification.success = false;
    
    uint64_t failed_reward = distributor.calculate_reward_amount(task, failed_verification);
    assert(failed_reward == 0);
    
    std::cout << "  ✓ Failed verification handling successful" << std::endl;
    
    // Test unverified proof distribution
    ZKComputeProof unverified_proof;
    unverified_proof.task_id = "unverified_task_001";
    unverified_proof.verification_status = "pending";
    
    bool unverified_distributed = distributor.distribute_reward(unverified_proof, recipient_wallet);
    assert(unverified_distributed == false);
    
    std::cout << "  ✓ Unverified proof rejection successful" << std::endl;
    
    std::cout << "RewardDistributor tests passed!" << std::endl;
}

void test_zk_compute_stack() {
    std::cout << "\nTesting ZKComputeStack..." << std::endl;
    
    // Create stack
    ZKComputeStack stack = create_zk_compute_stack();
    
    assert(stack.prover != nullptr);
    assert(stack.manager != nullptr);
    assert(stack.allocator != nullptr);
    assert(stack.distributor != nullptr);
    
    std::cout << "  ✓ Stack creation successful" << std::endl;
    
    // Test end-to-end workflow
    ComputeTask task;
    task.task_id = "workflow_task_001";
    task.compute_type = ComputeType::GAS_REIMBURSEMENT;
    task.input_data = {50, 60, 70};
    task.circuit_identifier = "gas_reimbursement_circuit_v1";
    task.required_compute_units = 1500;
    
    // Allocate resources
    ResourceCommitment commitment = stack.allocator->allocate_resources(task, "node_001");
    assert(!commitment.commitment_id.empty());
    
    std::cout << "  ✓ Resource allocation in stack successful" << std::endl;
    
    // Submit task
    std::string task_id = stack.manager->submit_task(task, commitment);
    assert(!task_id.empty());
    
    std::cout << "  ✓ Task submission in stack successful" << std::endl;
    
    // Verify task
    VerificationResult verification = stack.manager->verify_task_completion(task_id);
    assert(verification.success == true);
    
    std::cout << "  ✓ Task verification in stack successful" << std::endl;
    
    // Distribute reward
    ZKComputeProof proof = stack.manager->get_task_status(task_id);
    bool rewarded = stack.distributor->distribute_reward(proof, "wallet_001");
    assert(rewarded == true);
    
    std::cout << "  ✓ Reward distribution in stack successful" << std::endl;
    
    // Release resources
    stack.allocator->release_resources(commitment.commitment_id);
    
    std::cout << "  ✓ Resource release in stack successful" << std::endl;
    
    std::cout << "ZKComputeStack tests passed!" << std::endl;
}

void test_compute_types() {
    std::cout << "\nTesting ComputeType enum..." << std::endl;
    
    // Test all compute types
    std::vector<ComputeType> types = {
        ComputeType::LLM_INFERENCE,
        ComputeType::MERKLE_COMPUTATION,
        ComputeType::MICROTASK_COMPLETION,
        ComputeType::COLLATERAL_LOCKING,
        ComputeType::GAS_REIMBURSEMENT,
        ComputeType::IDO_APPRAISAL,
        ComputeType::CIRCUIT_COMPILATION,
        ComputeType::PROOF_VERIFICATION
    };
    
    for (const auto& type : types) {
        ComputeTask task;
        task.compute_type = type;
        task.required_compute_units = 1000;
        
        ZKProver prover;
        ResourceCommitment commitment;
        ZKComputeProof proof = prover.generate_proof(task, commitment);
        
        assert(proof.compute_type == type);
    }
    
    std::cout << "  ✓ All compute types handled correctly" << std::endl;
    std::cout << "ComputeType tests passed!" << std::endl;
}

void test_concurrent_operations() {
    std::cout << "\nTesting concurrent operations..." << std::endl;
    
    ZKComputeManager manager;
    std::vector<std::string> task_ids;
    
    // Submit multiple tasks concurrently
    for (int i = 0; i < 10; ++i) {
        ComputeTask task;
        task.task_id = "concurrent_task_" + std::to_string(i);
        task.compute_type = ComputeType::MICROTASK_COMPLETION;
        task.input_data = {static_cast<uint8_t>(i)};
        task.required_compute_units = 500 + (i * 100);
        
        ResourceCommitment commitment;
        commitment.commitment_id = "concurrent_commitment_" + std::to_string(i);
        commitment.node_id = "node_001";
        
        std::string task_id = manager.submit_task(task, commitment);
        task_ids.push_back(task_id);
    }
    
    assert(task_ids.size() == 10);
    
    std::cout << "  ✓ Concurrent task submission successful" << std::endl;
    
    // Verify all tasks
    for (const auto& task_id : task_ids) {
        VerificationResult verification = manager.verify_task_completion(task_id);
        assert(verification.success == true);
    }
    
    std::cout << "  ✓ Concurrent task verification successful" << std::endl;
    
    // Check statistics
    auto stats = manager.get_stats();
    assert(stats.total_tasks_submitted == 10);
    assert(stats.total_tasks_verified == 10);
    
    std::cout << "  ✓ Statistics consistency verified" << std::endl;
    
    std::cout << "Concurrent operations tests passed!" << std::endl;
}

int main() {
    std::cout << "=== ZK Compute Integration Test Suite ===" << std::endl;
    
    try {
        test_zk_prover();
        test_zk_compute_manager();
        test_compute_resource_allocator();
        test_reward_distributor();
        test_zk_compute_stack();
        test_compute_types();
        test_concurrent_operations();
        
        std::cout << "\n✅ All ZK Compute tests passed successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}