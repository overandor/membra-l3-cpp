#pragma once

#include "crypto.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace membra {

enum class InferenceType : uint8_t {
    PROOF_GENERATION = 0,
    VALIDATION = 1,
    CONSENSUS = 2,
    ZK_COMPUTATION = 3
};

struct InferenceReceipt {
    std::string receipt_id;
    std::string worker_address;
    std::string task_id;
    InferenceType inference_type;
    
    uint64_t input_hash;
    uint64_t output_hash;
    uint64_t compute_units_consumed;
    uint64_t cpu_cycles;
    uint64_t memory_bytes_used;
    double execution_time_sec;
    
    double timestamp;
    std::string signature;
    bool verified;
    
    // Reward information
    uint64_t reward_lamports;
    bool reward_claimed;
    double reward_claimed_at;
};

class ProofOfInference {
private:
    std::unordered_map<std::string, InferenceReceipt> receipts_;
    std::mutex mutex_;
    std::atomic<uint64_t> receipt_counter_;
    std::atomic<uint64_t> total_compute_units_;
    std::atomic<uint64_t> total_rewards_paid_;

public:
    ProofOfInference()
        : receipt_counter_(0), total_compute_units_(0), total_rewards_paid_(0) {}
    
    // Create a new inference receipt
    std::string create_receipt(
        const std::string& worker_address,
        const std::string& task_id,
        InferenceType type,
        uint64_t input_hash,
        uint64_t output_hash,
        uint64_t compute_units,
        uint64_t cpu_cycles,
        uint64_t memory_bytes,
        double execution_time
    ) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string receipt_id = "infer_" + std::to_string(receipt_counter_++);
        
        InferenceReceipt receipt;
        receipt.receipt_id = receipt_id;
        receipt.worker_address = worker_address;
        receipt.task_id = task_id;
        receipt.inference_type = type;
        receipt.input_hash = input_hash;
        receipt.output_hash = output_hash;
        receipt.compute_units_consumed = compute_units;
        receipt.cpu_cycles = cpu_cycles;
        receipt.memory_bytes_used = memory_bytes;
        receipt.execution_time_sec = execution_time;
        receipt.timestamp = current_timestamp();
        receipt.signature = sign_receipt(receipt);
        receipt.verified = false;
        receipt.reward_lamports = 0;
        receipt.reward_claimed = false;
        receipt.reward_claimed_at = 0;
        
        receipts_[receipt_id] = std::move(receipt);
        total_compute_units_ += compute_units;
        
        return receipt_id;
    }
    
    // Verify a receipt's signature and hash chain
    bool verify_receipt(const std::string& receipt_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = receipts_.find(receipt_id);
        if (it == receipts_.end()) return false;
        
        InferenceReceipt& receipt = it->second;
        
        // Verify signature (simplified - would use actual crypto in production)
        receipt.verified = verify_signature(receipt);
        return receipt.verified;
    }
    
    // Claim reward for a verified receipt
    bool claim_reward(const std::string& receipt_id, uint64_t reward_lamports) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = receipts_.find(receipt_id);
        if (it == receipts_.end()) return false;
        if (!it->second.verified) return false;
        if (it->second.reward_claimed) return false;
        
        it->second.reward_lamports = reward_lamports;
        it->second.reward_claimed = true;
        it->second.reward_claimed_at = current_timestamp();
        
        total_rewards_paid_ += reward_lamports;
        return true;
    }
    
    // Check if receipt is replayed (already used)
    bool is_replay(const std::string& receipt_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = receipts_.find(receipt_id);
        return it != receipts_.end() && it->second.reward_claimed;
    }
    
    // Get receipt by ID
    InferenceReceipt* get_receipt(const std::string& receipt_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = receipts_.find(receipt_id);
        return it == receipts_.end() ? nullptr : &it->second;
    }
    
    // Get all receipts for a worker
    std::vector<InferenceReceipt> get_worker_receipts(const std::string& worker_address) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<InferenceReceipt> result;
        for (const auto& pair : receipts_) {
            if (pair.second.worker_address == worker_address) {
                result.push_back(pair.second);
            }
        }
        return result;
    }
    
    uint64_t total_compute_units() const { return total_compute_units_.load(); }
    uint64_t total_rewards_paid() const { return total_rewards_paid_.load(); }
    size_t receipt_count() {
        std::lock_guard<std::mutex> lock(mutex_);
        return receipts_.size();
    }

private:
    static std::string sign_receipt(const InferenceReceipt& receipt) {
        // Simplified signature generation
        std::string data = receipt.receipt_id + receipt.worker_address + 
                         std::to_string(receipt.input_hash) + 
                         std::to_string(receipt.timestamp);
        return hash_to_hex(sha256(data));
    }
    
    static bool verify_signature(const InferenceReceipt& receipt) {
        // Simplified verification - would use actual crypto in production
        std::string data = receipt.receipt_id + receipt.worker_address + 
                         std::to_string(receipt.input_hash) + 
                         std::to_string(receipt.timestamp);
        return hash_to_hex(sha256(data)) == receipt.signature;
    }
    
    static double current_timestamp() {
        return std::chrono::duration<double>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

} // namespace membra
